#include <bridge.hpp>

ACTION bridge::addtoken(const name& channel, const extended_symbol& token_symbol, const bool& do_issue, const asset& min_quantity, const extended_symbol& remote_token, const float& fee_pct, const asset& fee_flat, const bool& enabled) {
  auto settings = get_settings();
  require_auth(settings.admin_account);

  check(channel_exists(channel), "unknown channel");
  check(fee_pct >= 0 && fee_pct <= 20, "token_row.fee_pct can't be greater than 20%");

  tokens_table tokens_t(get_self(), channel.value);
  auto remote_token_index = tokens_t.get_index<name("byremote")>();

  // check token not already added
  check(tokens_t.find(token_symbol.get_symbol().raw()) == tokens_t.end(), "token already exists");
  check(remote_token_index.find(remote_token.get_symbol().code().raw()) == remote_token_index.end(), "remote token already exists");
  check(remote_token.get_symbol().precision() == min_quantity.symbol.precision(), "remote_token and min_quantity precisions must match.");
  check(fee_flat.symbol == min_quantity.symbol, "fee_flat and min_quantity token symbol/precision must match.");
  check(fee_flat.amount < min_quantity.amount, "fee_flat quantity must be less than min_quantity.");
  const asset empty_asset = asset(0, token_symbol.get_symbol());

  tokens_t.emplace(get_self(), [&](tokens_row& row) {
    row.token_info = token_symbol;
    row.do_issue = do_issue;
    row.min_quantity = min_quantity;
    row.channel = channel;
    row.remote_token = remote_token;
    row.enabled = enabled;
    row.fee_flat = fee_flat;
    row.fee_pct = fee_pct;
    row.total_claimed_fees = empty_asset;
    row.unclaimed_fees = empty_asset;
    // row.successful_tr
  });
}

ACTION bridge::updatetoken(const name& channel, const extended_symbol& token_symbol, const asset& min_quantity, const bool& enabled) {
  auto settings = get_settings();
  require_auth(settings.admin_account);

  tokens_table _tokens(get_self(), channel.value);
  auto token = _tokens.find(token_symbol.get_symbol().code().raw());
  check(token != _tokens.end(), "token not found");

  _tokens.modify(token, get_self(), [&](auto& s) {
    s.enabled = enabled;
    s.min_quantity = min_quantity;
  });
}
