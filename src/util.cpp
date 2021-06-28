#include <bridge.hpp>
void bridge::free_ram(const name& channel) {
  reports_table reports_t(get_self(), channel.value);
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

  auto reports_by_expiry = reports_t.get_index<name("byexpiry")>();
  count = 0;
  for(auto it = reports_by_expiry.lower_bound(0);
      it != reports_by_expiry.upper_bound(now) && count < 2;
      count++, it = reports_by_expiry.lower_bound(0)) {
    // track reports that were not executed and where no refund was initiated
    if(!it->executed && !it->failed) {
      expiredreports_table expiredreports_t(get_self(), channel.value);
      expiredreports_t.emplace(get_self(), [&](auto& x) { x = *it; });
    }
    reports_by_expiry.erase(it);
  }
}

#if defined(DEBUG)
ACTION bridge::clrreporters() {
  require_auth(get_self());
  cleanTable<reporters_table>(get_self(), get_self().value, 100);
};
ACTION bridge::clrtransfers(name channel) {
  require_auth(get_self());
  cleanTable<transfers_table>(get_self(), channel.value, 100);
};
ACTION bridge::clrtokens(name channel) {
  require_auth(get_self());
  cleanTable<tokens_table>(get_self(), channel.value, 100);
};
ACTION bridge::clrreports(name channel) {
  require_auth(get_self());
  cleanTable<reports_table>(get_self(), channel.value, 100);
};
#endif
