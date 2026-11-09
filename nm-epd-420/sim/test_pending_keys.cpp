#include <cassert>
#include <iostream>
#include <string>
#include "services/pending_keys.h"
#include "services/first_repeater.h"
int main() {
  device::PendingKeys keys;
  std::string typed;
  auto flush = [&]() { keys.flush([&](const char* t) { typed += t; }); };
  keys.append('a');
  keys.append('b');
  flush();
  assert(typed == "ab");
  // An undecided space holds back what follows (also multi-byte letters); a tap types it, the lock
  // gesture drops it.
  keys.append(' ', true);
  keys.append("\xc3\xa8");
  flush();
  assert(typed == "ab" && keys.waiting());
  keys.resolveAll(false);
  flush();
  assert(typed == "ab \xc3\xa8" && !keys.waiting());
  keys.append(' ', true);
  keys.append('w');
  keys.append(' ', true);
  keys.resolveAll(true);
  flush();
  assert(typed == "ab \xc3\xa8w");
  for (int i = 0; i < 128; i++)
    assert(keys.append('x'));
  assert(!keys.append('x'));
  assert(!keys.waiting() && keys.count == 0);
  device::FirstRepeater rpt;
  rpt.sent(42, 1);
  assert(!rpt.visible(1, 0));
  assert(!rpt.observe(41, "abcd", 100));
  assert(!rpt.observe(42, "", 100));
  assert(rpt.observe(42, "abcd", 100));
  assert(rpt.visible(1, 6099) && !rpt.visible(1, 6100) && !rpt.visible(0, 100));
  assert(!rpt.observe(42, "1122", 200) && rpt.hash == "abcd");
  rpt.sent(43, 2);
  assert(!rpt.visible(1, 101) && !rpt.observed);
  assert(rpt.observe(43, "ab", 0xfffffff0));
  assert(rpt.visible(2, 0xfffffff0u + 5999u) && !rpt.visible(2, 0xfffffff0u + 6000u));
  std::cout << "PASS: typed keys keep their order behind an undecided space (tap types it, lock "
               "gesture drops it), overflow; first observed repeater matched to latest send/chat, "
               "no invented route, exact 6s expiry\n";
}
