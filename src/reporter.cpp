#include <bridge.hpp>
void bridge::addreporter(const name& reporter, const uint8_t& weight) {
  auto globals = get_settings();
  require_auth(globals.admin_account);

  reporters_table reporters_t(get_self(), get_self().value);
  check(weight > 0, "weight must be greater than 0");

  check(is_account(reporter), "reporter account does not exist");
  reporters_table::const_iterator reporters_itr = reporters_t.find(reporter.value);

  check(reporters_itr == reporters_t.end(), "reporter already defined");

  reporters_t.emplace(get_self(), [&](reporters_row& row) {
    row.account = reporter;
    row.weight = weight;
  });
}

void bridge::rmreporter(const name& reporter) {
  auto globals = get_settings();
  require_auth(globals.admin_account);

  reporters_table _reporters_table(get_self(), get_self().value);

  auto it = _reporters_table.find(reporter.value);
  check(it != _reporters_table.end(), "reporter does not exist");
  check(it->unclaimed_points, "reporter has unclaimed points, claim points first and try again.");
  check(it->unclaimed_fees.empty(), "reporter has unclaimed fees, claim fees first and try again.");
  _reporters_table.erase(it);
}
