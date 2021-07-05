#include <bridge.hpp>
void bridge::register_transfer(const name& channel_name, const name& from, const name& to_account,
                               const asset& quantity, const string& memo, bool is_refund) {
  settings_singleton settings_table(get_self(), get_self().value);
  auto settings = settings_table.get();

  check(settings.enabled, "bridge disabled");

  channels_table channels(get_self(), get_self().value);
  auto channel_itt = channels.find(channel_name.value);
  check(channel_itt != channels.end(), "unknown channel, \"" + channel_name.to_string() + "\"");
  //    check( channel_itt-> enabled, "channel disabled");

  transfers_table _transfers_table(get_self(), channel_name.value);
  tokens_table tokens_t(get_self(), channel_name.value);

  const auto transfer_id = channel_itt->next_transfer_id;

  print(">>> ", channel_name, ", ", quantity.to_string());

  auto _token = tokens_t.get(quantity.symbol.code().raw(), "token not found");

  // record this transfer in case we need to refund it
  _transfers_table.emplace(get_self(), [&](auto& x) {
    x.id = transfer_id;
    x.transaction_id = get_trx_id();
    x.from_blockchain = settings.current_chain_name;
    x.to_blockchain = channel_name;
    x.from_account = from;
    x.to_account = to_account;
    x.quantity = quantity;
    x.memo = memo;
    x.transaction_time = current_time_point();
    x.expires_at = current_time_point() + settings.expire_after;
    x.is_refund = is_refund;
  });

  if(_token.do_issue && !is_refund) {
    // we never want proxy tokens that are not backed by real tokens, issue and retire as required
    // to mantain integrity of supply
    token::retire_action retire_act(_token.token_info.get_contract(), {get_self(), "active"_n});
    retire_act.send(quantity, "retire on transfer");
  }

  channels.modify(channel_itt, get_self(), [&](auto& c) {
    c.next_transfer_id += 1;
  });
}

void bridge::cleartransfers(const name& channel_name, const std::vector<uint64_t>& ids) {
  transfers_table _transfers_table(get_self(), channel_name.value);

  require_auth(get_self());

  for(auto id : ids) {
    auto it = _transfers_table.find(id);
    check(it != _transfers_table.end(), "some id does not exist");
    _transfers_table.erase(it);
  }
}
