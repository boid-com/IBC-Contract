#include <bridge.hpp>
ACTION bridge::init(name admin_account, name current_chain_name, uint32_t expire_after_seconds, uint8_t threshold) {
  settings_singleton _settings_table(get_self(), get_self().value);

  require_auth(get_self());

  bool settings_exists = _settings_table.exists();

  check(!settings_exists, "settings already defined");
  check(is_account(admin_account), "admin account does not exist");
  check(threshold > 0, "threshold must be positive");

  _settings_table.set(
    settings {
      .admin_account = admin_account,
      .current_chain_name = current_chain_name,
      .enabled = false,
      .expire_after = seconds(expire_after_seconds),
      .threshold = threshold,
    },
    get_self());
}

void bridge::update(const name& channel, const uint32_t& expire_after_seconds, const uint64_t& threshold) {
  settings_singleton _settings_table(get_self(), get_self().value);
  auto _settings = _settings_table.get();
  reports_table _reports_table(get_self(), channel.value);

  require_auth(get_self());

  check(threshold > 0, "minimum reporters must be positive");

  _settings.threshold = threshold;
  _settings.expire_after = seconds(expire_after_seconds);
  _settings_table.set(_settings, get_self());

  // need to update all unconfirmed reports and check if they are now confirmed
  for(auto report = _reports_table.begin(); report != _reports_table.end(); report++) {
    if(!report->confirmed && count_non_empty(report->confirmed_by) >= threshold) {
      _reports_table.modify(report, eosio::same_payer, [&](auto& s) {
        s.confirmed = true;
      });
    }
  }
}

void bridge::enable(bool enable) {
  settings_singleton settings_table(get_self(), get_self().value);
  check(settings_table.exists(), "contract not initialised");
  auto settings = settings_table.get();

  require_auth(settings.admin_account);

  settings.enabled = enable;
  settings_table.set(settings, get_self());
}

bridge::settings bridge::get_settings() {
  settings_singleton settings_table(get_self(), get_self().value);
  check(settings_table.exists(), "contract not initialised");
  return settings_table.get();
}