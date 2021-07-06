#include <bridge.hpp>
void bridge::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
  auto globals = get_settings();

  // allow maintenance when bridge disabled
  if(from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

  // check bridge
  check(globals.enabled, "bridge disabled");

  // check channel
  const memo_x_transfer& memo_object = parse_memo(memo);
  std::string channel(memo_object.to_blockchain);
  std::transform(channel.begin(), channel.end(), channel.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  name channel_name = name(channel);
  check(globals.current_chain_name != channel_name, "cannot send to the same chain");

  channels_table channels(get_self(), get_self().value);
  auto channel_itr = channels.find(channel_name.value);
  check(channel_itr != channels.end(), "unknown channel, \"" + channel + "\"");
  check(channel_itr->enabled, "channel disabled");

  // check token
  tokens_table tokens_t(get_self(), channel_name.value);

  tokens_table::const_iterator tokens_itr = tokens_t.require_find(quantity.symbol.code().raw(), "token not found");
  check(to == get_self(), "contract not involved in transfer");
  check(get_first_receiver() == tokens_itr->token_info.get_contract(), "incorrect token contract");
  check(quantity.symbol == tokens_itr->token_info.get_symbol(), "correct token contract, but wrong symbol");
  check(quantity >= tokens_itr->min_quantity, "sent quantity is less than required min quantity");
  float fee_pct = tokens_itr->fee_pct / 100;
  asset total_fee = float_to_asset(asset_to_float(quantity) * fee_pct, quantity.symbol) + tokens_itr->fee_flat;
  asset final_quantity = quantity - total_fee;
  check(final_quantity > tokens_itr->min_quantity, "After fee of " + total_fee.to_string() + " the transfer quantity " + quantity.to_string() + " is below the minimum of " + tokens_itr->min_quantity.to_string());

  // we cannot check remote account but at least verify is correct size
  check(memo_object.to_account.size() > 0 && memo_object.to_account.size() < 13, "invalid memo: target name \"" + memo_object.to_account + "\" is not valid");

  // store the collected fee
  tokens_t.modify(tokens_itr, get_self(), [&](tokens_row& row) { row.unclaimed_fees += total_fee; });

  register_transfer(channel_name, from, name(memo_object.to_account), final_quantity, memo_object.memo, false);
}
