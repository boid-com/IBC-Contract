#include <bridge.hpp>
void bridge::free_ram(const name& channel) {
  reports_table _reports_table(get_self(), channel.value);
  transfers_table _transfers_table(get_self(), channel.value);

  // delete 2 expired transfers and 2 expired reports
  uint64_t now = current_time_point().sec_since_epoch();
  auto count = 0;
  auto transfers_by_expiry = _transfers_table.get_index<name("byexpiry")>();
  for(auto it = transfers_by_expiry.lower_bound(0);
      it != transfers_by_expiry.upper_bound(now) && count < 2;
      count++, it = transfers_by_expiry.lower_bound(0)) {
    transfers_by_expiry.erase(it);
  }

  auto reports_by_expiry = _reports_table.get_index<name("byexpiry")>();
  count = 0;
  for(auto it = reports_by_expiry.lower_bound(0);
      it != reports_by_expiry.upper_bound(now) && count < 2;
      count++, it = reports_by_expiry.lower_bound(0)) {
    // track reports that were not executed and where no refund was initiated
    if(!it->executed && !it->failed) {
      expired_reports_table expired_reports_table(get_self(), channel.value);
      expired_reports_table.emplace(get_self(), [&](auto& x) { x = *it; });
    }
    reports_by_expiry.erase(it);
  }
}
