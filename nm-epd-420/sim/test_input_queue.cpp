#include <cassert>
#include <iostream>
#include "services/input_queue.h"
#include "services/lock_policy.h"
int main() {
  device::InputQueue q;
  for (int i = 0; i < 128; i++)
    assert(q.push(uint8_t(i), 100 + i));
  assert(!q.push(1, 999) && q.dropped == 1);
  assert(q.discardOverflow() && q.size() == 0);
  for (int i = 0; i < 100; i++)
    assert(q.push(uint8_t(i), 1000 + i));
  device::InputEvent e;
  for (int i = 0; i < 100; i++) {
    assert(q.pop(e) && e.raw == i && e.at == uint32_t(1000 + i));
  }
  assert(!q.pop(e));
  std::cout << "PASS: 128-key queue, overflow recovery, FIFO/timestamps\n";
}
