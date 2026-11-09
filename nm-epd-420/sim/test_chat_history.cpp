#include <cassert>
#include <iostream>
#include "services/chat_history.h"
int main() {
  using namespace device;
  std::vector<uint32_t> ids = {1, 2, 3, 4, 5};
  int offset = 0;
  for (int i = 1; i < 5; i++) {
    offset = historyStep(offset, 100, ids.size());
    assert(offset == i);
  }
  assert(historyStep(offset, 1, ids.size()) == 4);
  assert(historyStep(0, -1, 10) == 0 && historyStep(0, 1, 0) == 0);
  for (int i = 4; i > 0; i--) {
    offset = historyStep(offset, -8, 10);
    assert(offset == i - 1);
  }
  std::cout << "PASS: one-message scroll; bounds; empty/oldest/latest\n";
}
