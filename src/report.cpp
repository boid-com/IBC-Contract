#include <bridge.hpp>
ACTION bridge::report(const name& reporter, const name& channel, const transfers_row& transfer) {
  auto settings = get_settings();

  require_auth(reporter);
  reporters_table::const_iterator reporter_itr = check_reporter(reporter);
  reporter_worked(reporter);
  check(transfer.expires_at > current_time_point(), "transfer already expired");
  free_ram(channel);

  // we don't want report to fail, report anything at this point it will fail at execute and then
  // initiate a refund
  // check(is_account(transfer.to_account), "to account does not exist");

  // TODO we don't want report to fail, report anything at this point it will fail at execute and then
  // initiate a refund
  // check( channel == name(transfer.from_blockchain), "channel is invalid" )

  reports_table reports_t(get_self(), channel.value);

  uint128_t transfer_id = reports_row::_by_transfer_id(transfer);

  auto reports_by_transfer = reports_t.get_index<"bytransferid"_n>();
  auto reports_itr = reports_by_transfer.lower_bound(transfer_id);
  bool new_report = reports_itr == reports_by_transfer.upper_bound(transfer_id);

  if(!new_report) {
    new_report = true;
    // check and find report with same transfer data
    while(reports_itr != reports_by_transfer.upper_bound(transfer_id)) {
      if(reports_itr->transfer == transfer) {
        new_report = false;
        break;
      }
      reports_itr++;
    }
  }

  // first reporter
  if(new_report) {
    reports_t.emplace(reporter, [&](reports_row& row) {
      auto reserved_capacity = get_num_reporters();
      row.id = reports_t.available_primary_key();
      row.transfer = transfer;
      row.confirmation_weight = reporter_itr->weight;
      row.confirmed_by.push_back(reporter);
      row.confirmed = reporter_itr->weight >= settings.weight_threshold;
      row.executed = false;
    });

  } else {
    // checks that the reporter didn't already report the transfer
    check(std::find(reports_itr->confirmed_by.begin(), reports_itr->confirmed_by.end(), reporter) == reports_itr->confirmed_by.end(),
          "the reporter already reported the transfer");

    reports_by_transfer.modify(reports_itr, reporter, [&](reports_row& row) {
      row.confirmed_by.push_back(reporter);
      row.confirmation_weight += reporter_itr->weight;
      row.confirmed = row.confirmation_weight >= settings.weight_threshold;
    });
  }
}

ACTION bridge::exec(const name& reporter, const name& channel, const uint64_t& report_id) {
  globals_singleton settings_t(get_self(), get_self().value);
  globals settings_r = settings_t.get();
  check(settings_r.enabled, "bridge disabled");

  reports_table reports_t(get_self(), channel.value);
  tokens_table tokens_t(get_self(), channel.value);

  require_auth(reporter);
  check_reporter(reporter);
  reporter_worked(reporter);
  free_ram(channel);

  reports_table::const_iterator reports_itr = reports_t.find(report_id);
  check(reports_itr != reports_t.end(), "report does not exist");
  check(reports_itr->confirmed, "not confirmed yet");
  check(!reports_itr->executed, "already executed");
  check(!reports_itr->failed, "transfer already failed");
  check(reports_itr->transfer.expires_at > current_time_point(), "report's transfer already expired");

  //    auto _token = tokens_t.get( report->transfer.quantity.symbol.code().raw(), "token not found" );
  // lookup by remote token
  auto remote_token_index = tokens_t.get_index<name("byremote")>();
  tokens_row tokens_r = remote_token_index.get(reports_itr->transfer.quantity.symbol.code().raw(), "token not found");
  name token_contract = tokens_r.token_info.get_contract();

  // convert original symbol to symbol on this chain
  asset quantity = asset(reports_itr->transfer.quantity.amount, tokens_r.token_info.get_symbol());

  // Change logic, always issue tokens if do_issue. always retire on reverse path
  if(tokens_r.do_issue) {
    // issue tokens first, self must be issuer of token
    token::issue_action issue_act(token_contract, {get_self(), "active"_n});
    issue_act.send(get_self(), quantity, reports_itr->transfer.memo);
  }

  token::transfer_action transfer_act(token_contract, {get_self(), "active"_n});
  transfer_act.send(get_self(), reports_itr->transfer.to_account, quantity, reports_itr->transfer.memo);

  reports_t.modify(reports_itr, eosio::same_payer, [&](auto& s) {
    s.executed = true;
  });
}

ACTION bridge::execfailed(const name& reporter, const name& channel, const uint64_t& report_id) {
  globals_singleton settings_t(get_self(), get_self().value);
  auto settings_r = settings_t.get();

  check(settings_r.enabled, "bridge disabled");

  reports_table reports_t(get_self(), channel.value);
  tokens_table tokens_t(get_self(), channel.value);

  require_auth(reporter);
  reporters_table::const_iterator reporter_itr = check_reporter(reporter);
  reporter_worked(reporter);
  free_ram(channel);

  reports_table::const_iterator reports_itr = reports_t.find(report_id);
  check(reports_itr != reports_t.end(), "report does not exist");
  check(reports_itr->confirmed, "not confirmed yet");
  check(!reports_itr->executed, "already executed");
  check(!reports_itr->failed, "transfer already failed");
  check(reports_itr->transfer.expires_at > current_time_point(), "report's transfer already expired");
  check(std::find(reports_itr->failed_by.begin(), reports_itr->failed_by.end(), reporter) == reports_itr->failed_by.end(),
        "report already marked as failed by reporter");

  auto remote_token_index = tokens_t.get_index<name("byremote")>();
  auto _token = remote_token_index.get(reports_itr->transfer.quantity.symbol.code().raw(), "token not found");

  bool failed = false;
  reports_t.modify(reports_itr, eosio::same_payer, [&](reports_row& row) {
    row.failed_by.push_back(reporter);
    row.failed_weight += reporter_itr->weight;
    row.failed = failed = row.failed_weight >= settings_r.weight_threshold;
  });

  // init a cross-chain refund transfer
  if(failed) {
    // if original transfer already was a refund stop refund ping pong and just record it in a table requiring manual review
    if(reports_itr->transfer.is_refund) {
      // no_transfers_t failed_transfers_table(get_self(), get_self().value);
      // failed_transfers_table.emplace(get_self(),
      //                                [&](auto &x) { x = report->transfer; });
    } else {
      auto channel_name = reports_itr->transfer.from_blockchain;
      auto from = get_ibc_contract_for_channel(reports_itr->transfer.from_blockchain);
      auto to = reports_itr->transfer.from_account;
      auto quantity = asset(reports_itr->transfer.quantity.amount, _token.token_info.get_symbol());
      register_transfer(channel_name, from, to, quantity, "refund", true);
    }
  }
}

void bridge::clearexpired(const name& channel, const uint64_t& count) {
  reports_table reports_t(get_self(), channel.value);

  require_auth(get_self());

  auto current_count = 0;
  expiredreports_table expiredreports_t(get_self(), channel.value);
  for(auto it = expiredreports_t.begin(); it != expiredreports_t.end() && current_count < count; current_count++, it++) {
    expiredreports_t.erase(it);
  }
}

// must make sure to always clear transfers on other chain first otherwise would report twice
void bridge::clearreports(const name& channel, const std::vector<uint64_t>& ids) {
  reports_table reports_t(get_self(), channel.value);

  require_auth(get_self());

  for(auto id : ids) {
    auto it = reports_t.find(id);
    check(it != reports_t.end(), "some id does not exist");
    reports_t.erase(it);
  }
}
