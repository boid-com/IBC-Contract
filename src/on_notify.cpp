#include <bridge.hpp>

bool is_data_transfer(string memo) {
  return true;
}

void handle_data_transfer(name from, asset quantity, string memo) {
}

void bridge::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
  auto globals = get_settings();
  // check(false, memo.substr(0, 7));

  // allow maintenance when bridge disabled
  if(from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

  // check bridge
  check(globals.enabled, "bridge disabled");

  const memo_x_transfer& memo_object = parse_memo(memo);
  check(strncmp(&memo_object.to_blockchain.back(), ".", 1), "invalid channel name");
  name channel_name = name(memo_object.to_blockchain);

  check(globals.current_chain_name != channel_name, "cannot send to the same chain");

  channels_table channels(get_self(), get_self().value);
  auto channel_itr = channels.find(channel_name.value);
  check(channel_itr != channels.end(), "unknown channel, \"" + channel_name.to_string() + "\"");
  check(channel_itr->enabled, "channel disabled");

  // check token
  tokens_table tokens_t(get_self(), channel_name.value);

  tokens_table::const_iterator tokens_itr = tokens_t.require_find(quantity.symbol.code().raw(), "token not found");
  check(to == get_self(), "contract not involved in transfer");
  check(get_first_receiver() == tokens_itr->token_info.get_contract(), "incorrect token contract");
  check(quantity.symbol == tokens_itr->token_info.get_symbol(), "correct token contract, but wrong symbol");
  check(quantity >= tokens_itr->min_quantity, "sent quantity is less than required min quantity");
  const float fee_pct = tokens_itr->fee_pct / float(100);
  const asset fee_share = float_to_asset(asset_to_float(quantity) * fee_pct, quantity.symbol);
  const asset total_fee = fee_share + tokens_itr->fee_flat;
  // check(false, to_string(fee_pct) + " " + fee_share.to_string() + " " + total_fee.to_string());
  const asset final_quantity = quantity - total_fee;
  check(final_quantity >= tokens_itr->min_quantity, "After fee of " + total_fee.to_string() + " the transfer quantity " + quantity.to_string() + " is below the minimum of " + tokens_itr->min_quantity.to_string());

  // if(memo.substr(0, 6) == "IBCDATA") {
  //   xfer_memo += "| ibc_from:" + reports_itr->transfer.from_account.to_string() + "@" + reports_itr->transfer.from_blockchain.to_string();
  // }
  // we cannot check remote account but at least verify is correct size
  check(memo_object.to_account.size() > 0 && memo_object.to_account.size() < 13, "invalid memo: target name \"" + memo_object.to_account + "\" is not valid");

  // store the collected fee
  tokens_t.modify(tokens_itr, get_self(), [&](tokens_row& row) { row.unclaimed_fees += total_fee; });

  register_transfer(channel_name, from, name(memo_object.to_account), final_quantity, memo_object.memo, false);
}
