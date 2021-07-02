#include <bridge.hpp>
void bridge::addreporter(const name& reporter, const uint8_t& weight) {
  auto settings = get_settings();
  require_auth(settings.admin_account);

  reporters_table reporters_t(get_self(), get_self().value);

  check(is_account(reporter), "reporter account does not exist");
  reporters_table::const_iterator reporters_itr = reporters_t.find(reporter.value);

  check(reporters_itr == reporters_t.end(), "reporter already defined");

  reporters_t.emplace(get_self(), [&](reporters_row& row) {
    row.account = reporter;
    row.weight = weight;
  });
}

void bridge::rmreporter(const name& reporter) {
  auto settings = get_settings();
  require_auth(settings.admin_account);

  reporters_table _reporters_table(get_self(), get_self().value);

  auto it = _reporters_table.find(reporter.value);
  check(it != _reporters_table.end(), "reporter does not exist");

  _reporters_table.erase(it);
}
