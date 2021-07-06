#pragma once

#include <eosio.token/eosio.token.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/symbol.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>

using namespace eosio;
using namespace std;
#define DEBUG

CONTRACT bridge: public contract {
 public:
  using contract::contract;

  //
  // TABLES
  //
  // Global Settings
  TABLE settings_row {
    name admin_account;
    name current_chain_name;
    bool enabled;
    eosio::microseconds expire_after = days(7);  // duration after which transfers and reports expire and can be evicted from RAM
    uint32_t weight_threshold;                   // cumulative weight threshold needed for a report to be confirmed
    uint64_t unclaimed_points = 0;
  };
  using settings_singleton = eosio::singleton<"settings"_n, settings_row>;

  // Multichain Channels
  TABLE channels_row {
    name channel_name;
    name remote_contract;
    uint64_t next_transfer_id = 0;
    bool enabled = true;

    uint64_t primary_key() const { return channel_name.value; }
  };
  using channels_table = eosio::multi_index<"channels"_n, channels_row>;

  // IBC Tokens
  TABLE tokens_row {
    name channel;
    extended_symbol token_info;    // stores info about the token that needs to be issued or received
    bool do_issue;                 // whether tokens should be issued or taken from the contract balance
    asset min_quantity;            // minimum transfer quantity
    extended_symbol remote_token;  // contract where token is issued on this chain
    bool enabled;                  // disabled tokens can't be deposited for IBC transfers
    float fee_pct;                 // percentage of deposit to be taken as a fee
    asset fee_flat;                // a flat fee taken from each IBC deposit
    asset total_claimed_fees;      // fees collected and also claimed by reporters
    asset unclaimed_fees;          // fees collected but not yet claimed
    uint64_t primary_key() const { return token_info.get_symbol().code().raw(); }
    uint64_t by_remote() const { return remote_token.get_symbol().code().raw(); }
  };
  using tokens_table = eosio::multi_index<"tokens"_n, tokens_row,
                                          indexed_by<"byremote"_n, const_mem_fun<tokens_row, uint64_t, &tokens_row::by_remote>>>;

  // pending transfers
  TABLE transfers_row {
    uint64_t id;  // tokens_table.next_transfer_id
    checksum256 transaction_id;
    name from_blockchain;
    name to_blockchain;
    name from_account;
    name to_account;
    asset quantity;
    string memo;
    time_point_sec transaction_time;
    time_point_sec expires_at;  // secondary index
    bool is_refund = false;     // refunds are implemented as separate transfers

    uint64_t primary_key() const { return id; }
    uint64_t by_expiry() const { return _by_expiry(*this); }
    static uint64_t _by_expiry(const transfers_row& t) { return t.expires_at.sec_since_epoch(); }

    bool operator==(const transfers_row& b) const {
      auto a = *this;
      return a.id == b.id && a.transaction_id == b.transaction_id &&
             a.from_blockchain == b.from_blockchain &&
             a.to_blockchain == b.to_blockchain &&
             a.from_account == b.from_account && a.to_account == b.to_account &&
             a.quantity == b.quantity &&
             //                  a.memo == b.memo && // ignore memo for now
             a.transaction_time == b.transaction_time &&
             a.expires_at == b.expires_at && a.is_refund == b.is_refund;
    }
  };
  using transfers_table = eosio::multi_index<"transfers"_n, transfers_row,
                                             indexed_by<"byexpiry"_n, const_mem_fun<transfers_row, uint64_t, &transfers_row::by_expiry>>>;
  // reports from reporters
  TABLE reports_row {
    uint64_t id;
    transfers_row transfer;
    bool confirmed = false;
    std::vector<name> confirmed_by;
    uint32_t confirmation_weight = 0;
    uint32_t failed_weight = 0;
    bool executed = false;
    bool failed = false;
    std::vector<name> failed_by;

    uint64_t primary_key() const { return id; }
    uint128_t by_transfer_id() const { return _by_transfer_id(transfer); }
    static uint128_t _by_transfer_id(const transfers_row& t) {
      return static_cast<uint128_t>(t.from_blockchain.value) << 64 | t.id;
    }
    uint64_t by_expiry() const { return transfers_row::_by_expiry(transfer); }
  };
  using reports_table = eosio::multi_index<"reports"_n, reports_row,
                                           indexed_by<"bytransferid"_n, const_mem_fun<reports_row, uint128_t, &reports_row::by_transfer_id>>,
                                           indexed_by<"byexpiry"_n, const_mem_fun<reports_row, uint64_t, &reports_row::by_expiry>>>;
  // keeps track of unprocessed reports that expired for manual review
  using expiredreports_table = eosio::multi_index<"reports.expr"_n, reports_row>;

  // registered reporters
  TABLE reporters_row {
    name account;
    uint64_t unclaimed_points = 0;
    uint64_t total_claimed_points = 0;
    time_point_sec last_claim_time = current_time_point();
    uint8_t weight = 1;
    vector<extended_asset> unclaimed_fees;
    uint64_t primary_key() const { return account.value; }
  };
  using reporters_table = eosio::multi_index<"reporters"_n, reporters_row>;

  ACTION claimpoints(const name& reporter);
  ACTION claimfees(const name& reporter, const symbol_code& token);

  //
  // ACTIONS
  //

  /**
   * Called once to initialize the global settings singleton
   * @param admin_account admin has permission to do perform most actions
   * @param current_chain_name the name of the chain where this contract is deployed
   * @param expire_after_seconds pending transfers will stay in the transfers table for this long before they will fail
   * @param threshold how many reporters are required to approve a transfer for it to be validated
   */
  ACTION
  init(const name& admin_account, const name& current_chain_name, const uint32_t& expire_after_seconds, const uint32_t& weight_threshold);

  ACTION addchannel(const name& channel_name, const name& remote_contract);

  /**
   * register an IBC token
   * @param channel
   * @param token_symbol
   * @param do_issue issue tokens on this chain?
   * @param min_quantity minimum quantity of this token that can be sent over the IBC channel
   * @param remote_token external token contract info
   * @param enabled channel must be enabled to process transactions
   */
  ACTION addtoken(const name& channel, const extended_symbol& token_symbol, const bool& do_issue, const asset& min_quantity, const extended_symbol& remote_token, const float& fee_pct, const asset& fee_flat, const bool& enabled);

  /**
   * update token parameters
   * @param channel channel name
   * @param token_symbol
   * @param min_quantity minimum quantity of this token that can be sent over the IBC channel
   * @param enabled channel must be enabled to process transactions
   */
  ACTION updatetoken(const name& channel, const extended_symbol& token_symbol, const asset& min_quantity, const bool& enabled);

  /**
   *
   * @param channel
   * @param expire_after_seconds
   * @param threshold
   */
  ACTION update(const name& channel, const uint32_t& expire_after_seconds, const uint32_t& weight_threshold);

  /**
   *
   * @param enable
   */
  ACTION enable(const bool& enable);

  /**
   *
   * @param reporter
   */
  ACTION addreporter(const name& reporter, const uint8_t& weight);

  /**
   *
   * @param reporter
   */
  ACTION rmreporter(const name& reporter);

  /**
   *
   * @param channel_name
   * @param ids
   */
  [[eosio::action("clear.trans")]] void cleartransfers(const name& channel_name, const std::vector<uint64_t>& ids);

  /**
   *
   * @param channel
   * @param ids
   */
  [[eosio::action("clear.rep")]] void clearreports(const name& channel, const std::vector<uint64_t>& ids);

  /**
   *
   * @param channel
   * @param count
   */
  [[eosio::action("clear.exp")]] void clearexpired(const name& channel, const uint64_t& count);

  /**
   *
   * @param reporter reporter must be registered
   * @param channel
   * @param transfer
   */
  ACTION report(const name& reporter, const name& channel, const transfers_row& transfer);

  /**
   *
   * @param reporter
   * @param channel
   * @param report_id
   */
  ACTION exec(const name& reporter, const name& channel, const uint64_t& report_id);

  /**
   *
   * @param reporter
   * @param channel
   * @param report_id
   */
  ACTION execfailed(const name& reporter, const name& channel, const uint64_t& report_id);

  /**
   *
   * @param from
   * @param to
   * @param quantity
   * @param memo
   */
  [[eosio::on_notify("*::transfer")]] void on_transfer(const name& from,
                                                       const name& to,
                                                       const asset& quantity,
                                                       const string& memo);

#if defined(DEBUG)
  ACTION clrreporters();
  ACTION clrtransfers(name channel);
  ACTION clrtokens(name channel);
  ACTION clrreports(name channel);
  ACTION clrchannels();

  template <typename T>
  void cleanTable(name code, uint64_t account, const uint32_t batchSize) {
    T db(code, account);
    uint32_t counter = 0;
    auto itr = db.begin();
    while(itr != db.end() && counter++ < batchSize) {
      itr = db.erase(itr);
    }
  }
#endif

 private:
  settings_row get_settings();

  bool channel_exists(name channel_name);

  void register_transfer(const name& channel_name, const name& from, const name& to_account,
                         const asset& quantity, const string& memo, bool is_refund);
  void reporter_worked(const name& reporter);
  void free_ram(const name& channel);

  checksum256 get_trx_id() {
    size_t size = transaction_size();
    char buf[size];
    size_t read = read_transaction(buf, size);
    check(size == read, "read_transaction failed");
    return sha256(buf, read);
  }

  struct memo_x_transfer {
    string version;
    string to_blockchain;
    string to_account;
    string memo;
  };

  memo_x_transfer parse_memo(string memo) {
    string to_memo = "";
    if(memo.find("|") != string::npos) {
      auto major_parts = split(memo, "|");  // split the memo by "|"
      to_memo = memo;
      to_memo.erase(0, major_parts[0].length() + 1);
      memo = major_parts[0];
    }

    auto res = memo_x_transfer();
    auto parts = split(memo, "@");
    res.version = "2.0";
    res.to_blockchain = parts[1];
    res.to_account = parts[0];
    res.memo = to_memo;
    return res;
  }

  reporters_table::const_iterator check_reporter(name reporter) {
    reporters_table reporters_t(get_self(), get_self().value);
    return reporters_t.require_find(reporter.value, "the signer is not a known reporter");
  }

  /**
     * TODO this function is hardcoded for EOS / TLOS, change to allow remote chain and contract to be configured
     * TODO is this necessary? Seems like reporter works anyway
     * @param chain_name
     * @return
     */
  name get_ibc_contract_for_channel(name channel) {
    channels_table channels(get_self(), get_self().value);
    auto channel_itt = channels.find(channel.value);
    check(channel_itt != channels.end(), "unknown channel, \"" + channel.to_string() + "\"");
    check(channel_itt->enabled, "channel disabled");

    return channel_itt->remote_contract;
  }

  uint32_t get_num_reporters() {
    uint32_t count = 0;
    reporters_table _reporters_table(get_self(), get_self().value);
    for(auto it = _reporters_table.begin(); it != _reporters_table.end(); it++) {
      count++;
    }

    return count;
  }

  std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;

    do {
      pos = str.find(delim, prev);
      if(pos == std::string::npos) pos = str.length();
      std::string token = str.substr(prev, pos - prev);
      tokens.push_back(token);
      prev = pos + delim.length();
    } while(pos < str.length() && prev < str.length());
    return tokens;
  }

  void push_first_free(std::vector<eosio::name> & vec, eosio::name val) {
    for(auto it = vec.begin(); it != vec.end(); it++) {
      if(it->value == 0) {
        *it = val;
        return;
      }
    }
    eosio::check(false, "push_first_free: iterated past vector");
  }

  uint32_t count_non_empty(const std::vector<eosio::name>& vec) {
    uint32_t count = 0;
    for(auto it : vec) {
      if(it.value == 0) {
        break;
      }
      count++;
    }
    return count;
  }
  float pow(float x, int y) {
    float result = 1;
    for(int i = 0; i < y; ++i) {
      result *= x;
    }
    return result;
  }

  float asset_to_float(asset target) {
    return float(target.amount) / pow(10.0, target.symbol.precision());
  }

  asset float_to_asset(float target, symbol symbol) {
    return asset(uint64_t(target * pow(10.0, symbol.precision())), symbol);
  }

  void add_fee_token(reporters_table & reporters_t, reporters_table::const_iterator target_itr, const extended_asset& quantity) {
    reporters_t.modify(target_itr, get_self(), [&](reporters_row& row) {
      vector<eosio::extended_asset>::iterator token_itr = find_if(row.unclaimed_fees.begin(), row.unclaimed_fees.end(), [&](extended_asset& token) {
        return (token.get_extended_symbol() == quantity.get_extended_symbol());
      });
      if(token_itr == row.unclaimed_fees.end()) row.unclaimed_fees.push_back(quantity);
      else
        token_itr->quantity += quantity.quantity;
    });
  }
};
