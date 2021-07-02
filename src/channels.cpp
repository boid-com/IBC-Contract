
#include <bridge.hpp>
ACTION bridge::addchannel(const name& channel_name, const name& remote_contract) {
  auto settings = get_settings();
  require_auth(settings.admin_account);
  // require_auth(get_self());

  check(channel_name != settings.current_chain_name, "cannot create channel to self");

  channels_table channels_t(get_self(), get_self().value);
  check(channels_t.find(channel_name.value) == channels_t.end(), "channel already exists");

  channels_t.emplace(get_self(), [&](channels_row& row) {
    row.channel_name = channel_name;
    row.remote_contract = remote_contract;
  });
}

bool bridge::channel_exists(const name channel_name) {
  channels_table channels_t(get_self(), get_self().value);
  return (channels_t.find(channel_name.value) != channels_t.end());
}