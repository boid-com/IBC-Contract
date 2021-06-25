
#include <bridge.hpp>
ACTION bridge::addchannel(name& channel_name, name& remote_contract) {
  auto settings = get_settings();
  require_auth(settings.admin_account);
  // require_auth(get_self());

  check(channel_name != settings.current_chain_name, "cannot create channel to self");

  channels_table _channels(get_self(), get_self().value);
  check(_channels.find(channel_name.value) == _channels.end(), "channel already exists");

  _channels.emplace(get_self(), [&](auto& row) {
    row.channel_name = channel_name;
    row.remote_contract = remote_contract;
  });
}

bool bridge::channel_exists(name channel_name) {
  channels_table _channels(get_self(), get_self().value);
  return (_channels.find(channel_name.value) != _channels.end());
}