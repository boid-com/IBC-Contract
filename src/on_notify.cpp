#include <bridge.hpp>
void bridge::on_transfer(name from, name to, asset quantity, string memo) {
  auto settings = get_settings();

  // allow maintenance when bridge disabled
  if(from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

  // check bridge
  check(settings.enabled, "bridge disabled");

  // check channel
  const memo_x_transfer& memo_object = parse_memo(memo);
  std::string channel(memo_object.to_blockchain);
  std::transform(channel.begin(), channel.end(), channel.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  name channel_name = name(channel);
  check(settings.current_chain_name != channel_name, "cannot send to the same chain");

  channels_table channels(get_self(), get_self().value);
  auto channel_itt = channels.find(channel_name.value);
  check(channel_itt != channels.end(), "unknown channel, \"" + channel + "\"");
  check(channel_itt->enabled, "channel disabled");

  // check token
  tokens_table _tokens_table(get_self(), channel_name.value);

  auto _token = _tokens_table.get(quantity.symbol.code().raw(), "token not found");
  check(to == get_self(), "contract not involved in transfer");
  check(get_first_receiver() == _token.token_info.get_contract(), "incorrect token contract");
  check(quantity.symbol == _token.token_info.get_symbol(), "correct token contract, but wrong symbol");
  check(quantity >= _token.min_quantity, "sent quantity is less than required min quantity");

  // we cannot check remote account but at least verify is correct size
  check(memo_object.to_account.size() > 0 && memo_object.to_account.size() < 13,
        "invalid memo: target name \"" + memo_object.to_account + "\" is not valid");

  register_transfer(channel_name, from, name(memo_object.to_account), quantity, memo_object.memo, false);
}
