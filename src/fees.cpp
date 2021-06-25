#include <bridge.hpp>
void bridge::reporter_worked(const name& reporter) {
  reporters_table _reporters_table(get_self(), get_self().value);

  auto it = _reporters_table.find(reporter.value);
  check(it != _reporters_table.end(), "reporter does not exist while PoW");

  _reporters_table.modify(it, eosio::same_payer, [&](auto& s) {
    s.points++;
  });
}
