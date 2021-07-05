#include <bridge.hpp>
void bridge::reporter_worked(const name& reporter) {
  reporters_table reporters_t(get_self(), get_self().value);

  reporters_table::const_iterator reporters_itr = reporters_t.find(reporter.value);
  check(reporters_itr != reporters_t.end(), "reporter does not exist while PoW");

  reporters_t.modify(reporters_itr, eosio::same_payer, [&](reporters_row& row) {
    row.unclaimed_points++;
  });

  // increment global unclaimed points
  settings_singleton settings_table(get_self(), get_self().value);
  settings_row settings_r = settings_table.get();
  settings_r.unclaimed_points++;
  settings_table.set(settings_r, same_payer);
}

ACTION bridge::claimpoints(const name& reporter) {
  check(has_auth(reporter) || has_auth(get_self()), "Not authorized");
  reporters_table reporters_t(get_self(), get_self().value);
  reporters_table::const_iterator reporters_itr = reporters_t.require_find(reporter.value, "Unable to find reporter");
  check(reporters_itr->unclaimed_points > 0, "Reporter doesn't have points to claim");
  auto reporter_unclaimed_points = reporters_itr->unclaimed_points;

  channels_table channels_t(get_self(), get_self().value);
  check(channels_t.begin() != channels_t.end(), "channel ha");
  channels_table::const_iterator channels_itr = channels_t.begin();

  settings_singleton settings_table(get_self(), get_self().value);
  settings_row settings_r = settings_table.get();

  const float reporters_share = float(reporter_unclaimed_points) / float(settings_r.unclaimed_points);

  while(channels_itr != channels_t.end()) {
    tokens_table tokens_t(get_self(), channels_itr->channel_name.value);
    tokens_table::const_iterator tokens_itr = tokens_t.begin();
    while(tokens_itr != tokens_t.end()) {
      const asset reporter_claim_quantity = tokens_itr->unclaimed_fees * reporters_share;
      const extended_asset reporter_claim_extended = extended_asset(reporter_claim_quantity, tokens_itr->token_info.get_contract());
      if(tokens_itr->unclaimed_fees.amount > 0) {
        // add the token to the reporters row to withdraw later
        add_fee_token(reporters_t, reporters_itr, reporter_claim_extended);
        // subtract the token from the unclaimed fees
        tokens_t.modify(tokens_itr, get_self(), [&](tokens_row& row) {
          row.unclaimed_fees -= reporter_claim_quantity;
        });
        // move to next token
        tokens_itr++;
      } else
        // token doesn't have any fees to collect
        tokens_itr++;
    }
    channels_itr++;
  }
  // finished looping through the channels and tokens and collecting the reporters share of the fees

  // count points as claimed for the reporter
  reporters_t.modify(reporters_itr, get_self(), [&](reporters_row& row) {
    row.total_claimed_points += reporter_unclaimed_points;
    row.unclaimed_points = 0;
    row.last_claim_time = current_time_point();
  });

  //subtract the claimed points from the global unclaimed points
  settings_r.unclaimed_points -= reporter_unclaimed_points;
  settings_table.set(settings_r, same_payer);
}
ACTION bridge::claimfees(const name& reporter) {
  check(has_auth(reporter) || has_auth(get_self()), "Not authorized");
}
