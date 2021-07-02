#include <bridge.hpp>
ACTION bridge::init(const name& admin_account, const name& current_chain_name, const uint32_t& expire_after_seconds, const uint32_t& weight_threshold) {
  settings_singleton settings_t(get_self(), get_self().value);

  require_auth(get_self());

  bool settings_exists = settings_t.exists();

  check(!settings_exists, "settings already defined");
  check(is_account(admin_account), "admin account does not exist");
  check(weight_threshold > 0, "threshold must be positive");

  settings_t.set(
    settings_row {
      .admin_account = admin_account,
      .current_chain_name = current_chain_name,
      .enabled = false,
      .expire_after = seconds(expire_after_seconds),
      .weight_threshold = weight_threshold,
    },
    get_self());
}

ACTION bridge::update(const name& channel, const uint32_t& expire_after_seconds, const uint32_t& weight_threshold) {
  settings_singleton settings_t(get_self(), get_self().value);
  settings_row settings_r = settings_t.get();
  reports_table reports_t(get_self(), channel.value);

  require_auth(get_self());

  check(weight_threshold > 0, "minimum reporters must be positive");

  settings_r.weight_threshold = weight_threshold;
  settings_r.expire_after = seconds(expire_after_seconds);
  settings_t.set(settings_r, get_self());

  // need to update all unconfirmed reports and check if they are now confirmed
  for(auto report = reports_t.begin(); report != reports_t.end(); report++) {
    if(!report->confirmed && report->confirmation_weight >= weight_threshold) {
      reports_t.modify(report, eosio::same_payer, [&](reports_row& row) {
        row.confirmed = true;
      });
    }
  }
}

void bridge::enable(const bool& enable) {
  settings_singleton settings_table(get_self(), get_self().value);
  check(settings_table.exists(), "contract not initialised");
  auto settings = settings_table.get();

  require_auth(settings.admin_account);

  settings.enabled = enable;
  settings_table.set(settings, get_self());
}

bridge::settings_row bridge::get_settings() {
  settings_singleton settings_table(get_self(), get_self().value);
  check(settings_table.exists(), "contract not initialised");
  return settings_table.get();
}