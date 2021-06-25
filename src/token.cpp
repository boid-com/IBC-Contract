#include <bridge.hpp>
ACTION bridge::addtoken(const name& channel, extended_symbol token_symbol, bool do_issue, asset min_quantity, extended_symbol remote_token, bool enabled) {
  auto settings = get_settings();
  require_auth(settings.admin_account);

  check(channel_exists(channel), "unknown channel");

  tokens_table _tokens(get_self(), channel.value);
  auto remote_token_index = _tokens.get_index<name("byremote")>();

  // check token not already added
  check(_tokens.find(token_symbol.get_symbol().raw()) == _tokens.end(), "token already exists");
  check(remote_token_index.find(remote_token.get_symbol().code().raw()) == remote_token_index.end(), "remote token already exists");

  _tokens.emplace(get_self(), [&](auto& row) {
    row.token_info = token_symbol;
    row.do_issue = do_issue;
    row.min_quantity = min_quantity;
    row.channel = channel;
    row.remote_token = remote_token;
    row.enabled = enabled;
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
