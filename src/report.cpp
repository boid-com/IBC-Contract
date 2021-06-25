#include <bridge.hpp>
void bridge::report(const name& reporter, const name& channel, const transfers_row& transfer) {
  auto settings = get_settings();

  require_auth(reporter);
  check_reporter(reporter);
  reporter_worked(reporter);
  check(transfer.expires_at > current_time_point(), "transfer already expired");
  free_ram(channel);

  // we don't want report to fail, report anything at this point it will fail at execute and then
  // initiate a refund
  // check(is_account(transfer.to_account), "to account does not exist");

  // TODO we don't want report to fail, report anything at this point it will fail at execute and then
  // initiate a refund
  // check( channel == name(transfer.from_blockchain), "channel is invalid" )

  reports_table _reports_table(get_self(), channel.value);

  uint128_t transfer_id = reports_row::_by_transfer_id(transfer);

  auto reports_by_transfer = _reports_table.get_index<"bytransferid"_n>();
  auto report = reports_by_transfer.lower_bound(transfer_id);
  bool new_report = report == reports_by_transfer.upper_bound(transfer_id);

  if(!new_report) {
    new_report = true;
    // check and find report with same transfer data
    while(report != reports_by_transfer.upper_bound(transfer_id)) {
      if(report->transfer == transfer) {
        new_report = false;
        break;
      }
      report++;
    }
  }

  // first reporter
  if(new_report) {
    _reports_table.emplace(get_self(), [&](auto& s) {
      auto reserved_capacity = get_num_reporters();
      s.id = _reports_table.available_primary_key();
      s.transfer = transfer;
      // NA let first reporter pay for RAM
      // need to add actual elements because capacity is not serialized
      // does not work: s.confirmed_by.reserve(reserved_capacity);
      s.confirmed_by = std::vector<name>(reserved_capacity, eosio::name(""));
      push_first_free(s.confirmed_by, reporter);

      s.failed_by = std::vector<name>(reserved_capacity, eosio::name(""));
      s.confirmed = 1 >= settings.threshold;
      s.executed = false;
    });

  } else {
    // checks that the reporter didn't already report the transfer
    check(std::find(report->confirmed_by.begin(), report->confirmed_by.end(), reporter) == report->confirmed_by.end(),
          "the reporter already reported the transfer");

    // can use same_payer here because confirmed_by has enough capacity unless a new reporter was added in between
    reports_by_transfer.modify(report, eosio::same_payer, [&](auto& s) {
      push_first_free(s.confirmed_by, reporter);
      s.confirmed = count_non_empty(s.confirmed_by) >= settings.threshold;
    });
  }
}

void bridge::exec(const name& reporter, const name& channel, const uint64_t& report_id) {
  settings_singleton _settings_table(get_self(), get_self().value);
  auto _settings = _settings_table.get();
  check(_settings.enabled, "bridge disabled");

  reports_table _reports_table(get_self(), channel.value);
  tokens_table _tokens_table(get_self(), channel.value);

  require_auth(reporter);
  check_reporter(reporter);
  reporter_worked(reporter);
  free_ram(channel);

  auto report = _reports_table.find(report_id);
  check(report != _reports_table.end(), "report does not exist");
  check(report->confirmed, "not confirmed yet");
  check(!report->executed, "already executed");
  check(!report->failed, "transfer already failed");
  check(report->transfer.expires_at > current_time_point(), "report's transfer already expired");

  //    auto _token = _tokens_table.get( report->transfer.quantity.symbol.code().raw(), "token not found" );
  // lookup by remote token
  auto remote_token_index = _tokens_table.get_index<name("byremote")>();
  auto _token = remote_token_index.get(report->transfer.quantity.symbol.code().raw(), "token not found");
  name token_contract = _token.token_info.get_contract();

  // convert original symbol to symbol on this chain
  asset quantity = asset(report->transfer.quantity.amount, _token.token_info.get_symbol());

  // Change logic, always issue tokens if do_issue. always retire on reverse path
  if(_token.do_issue) {
    // issue tokens first, self must be issuer of token
    token::issue_action issue_act(token_contract, {get_self(), "active"_n});
    issue_act.send(get_self(), quantity, report->transfer.memo);
  }

  token::transfer_action transfer_act(token_contract, {get_self(), "active"_n});
  transfer_act.send(get_self(), report->transfer.to_account, quantity, report->transfer.memo);

  _reports_table.modify(report, eosio::same_payer, [&](auto& s) {
    s.executed = true;
  });
}

void bridge::execfailed(const name& reporter, const name& channel, const uint64_t& report_id) {
  settings_singleton _settings_table(get_self(), get_self().value);
  auto _settings = _settings_table.get();

  check(_settings.enabled, "bridge disabled");

  reports_table _reports_table(get_self(), channel.value);
  tokens_table _tokens_table(get_self(), channel.value);

  require_auth(reporter);
  check_reporter(reporter);
  reporter_worked(reporter);
  free_ram(channel);

  auto report = _reports_table.find(report_id);
  check(report != _reports_table.end(), "report does not exist");
  check(report->confirmed, "not confirmed yet");
  check(!report->executed, "already executed");
  check(!report->failed, "transfer already failed");
  check(report->transfer.expires_at > current_time_point(), "report's transfer already expired");
  check(std::find(report->failed_by.begin(), report->failed_by.end(), reporter) == report->failed_by.end(),
        "report already marked as failed by reporter");

  //    auto _token = _tokens_table.get( report->transfer.quantity.symbol.code().raw(), "token not found" );
  auto remote_token_index = _tokens_table.get_index<name("byremote")>();
  auto _token = remote_token_index.get(report->transfer.quantity.symbol.code().raw(), "token not found");

  bool failed = false;
  _reports_table.modify(report, eosio::same_payer, [&](auto& s) {
    push_first_free(s.failed_by, reporter);
    s.failed = failed = count_non_empty(s.failed_by) >= _settings.threshold;
  });

  // init a cross-chain refund transfer
  if(failed) {
    // if original transfer already was a refund stop refund ping pong and just record it in a table requiring manual review
    if(report->transfer.is_refund) {
      // no_transfers_t failed_transfers_table(get_self(), get_self().value);
      // failed_transfers_table.emplace(get_self(),
      //                                [&](auto &x) { x = report->transfer; });
    } else {
      auto channel_name = report->transfer.from_blockchain;
      auto from = get_ibc_contract_for_channel(report->transfer.from_blockchain);
      auto to = report->transfer.from_account;
      auto quantity = asset(report->transfer.quantity.amount, _token.token_info.get_symbol());
      register_transfer(channel_name, from, to, quantity, "refund", true);
    }
  }
}

void bridge::clearexpired(const name& channel, const uint64_t& count) {
  reports_table _reports_table(get_self(), channel.value);

  require_auth(get_self());

  auto current_count = 0;
  expired_reports_table expired_reports_table(get_self(), channel.value);
  for(auto it = expired_reports_table.begin(); it != expired_reports_table.end() && current_count < count; current_count++, it++) {
    expired_reports_table.erase(it);
  }
}

// must make sure to always clear transfers on other chain first otherwise would report twice
void bridge::clearreports(const name& channel, std::vector<uint64_t> ids) {
  reports_table _reports_table(get_self(), channel.value);

  require_auth(get_self());

  for(auto id : ids) {
    auto it = _reports_table.find(id);
    check(it != _reports_table.end(), "some id does not exist");
    _reports_table.erase(it);
  }
}
