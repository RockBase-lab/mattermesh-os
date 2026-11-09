#include <cassert>
#include <iostream>
#include "services/lock_policy.h"
int main() {
  assert(!device::autoLockDue(59999, 0, false));
  assert(device::autoLockDue(60000, 0, false));
  assert(!device::autoLockDue(90000, 40000, false));
  assert(!device::autoLockDue(90000, 0, true));
  assert(device::autoLockDue(uint32_t(0xfffffff0u + 60000u), 0xfffffff0u, false));
  std::cout << "PASS: auto-lock after 60 s idle, not when already locked, time rollover\n";
}
