#include <bridge.hpp>
void bridge::reporter_worked(const name& reporter) {
  reporters_table reporters_t(get_self(), get_self().value);

  reporters_table::const_iterator reporters_itr = reporters_t.find(reporter.value);
  check(reporters_itr != reporters_t.end(), "reporter does not exist while PoW");

  reporters_t.modify(reporters_itr, eosio::same_payer, [&](reporters_row& row) {
    row.unclaimed_points++;
  });
}
