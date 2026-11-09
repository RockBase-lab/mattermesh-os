#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include "services/vibrations.h"
int main() {
  using namespace device;
  assert(VIBRATION_COUNT >= 6 && VIBRATION_MESSAGE_DEFAULT != VIBRATION_MENTION_DEFAULT);
  assert(vibrationIndex(-1) == VIBRATION_MESSAGE_DEFAULT &&
         vibrationIndex(VIBRATION_COUNT) == VIBRATION_MESSAGE_DEFAULT);
  // Single keeps the original notification: one 120 ms pulse at full user intensity.
  assert(std::string(VIBRATIONS[0].name) == "Single" && vibrationStrength(0, 0) == 100 &&
         vibrationStrength(0, 119) == 100 && vibrationStrength(0, 120) == -1);
  std::set<std::string> names;
  for (int p = 0; p < VIBRATION_COUNT; p++) {
    names.insert(VIBRATIONS[p].name);
    int total = vibrationMs(p);
    assert(total > 0 && total <= 600); // short, never a long buzz
    const auto& v = VIBRATIONS[p];
    assert(v.steps[0].strength > 0 &&
           v.steps[v.count - 1].strength > 0); // starts and ends with a pulse
    for (int i = 0; i < v.count; i++) {
      assert(v.steps[i].strength <= 100);
      if (v.steps[i].strength)
        assert(v.steps[i].ms >= 50);
    } // ERM spin-up
    uint32_t at = 0;
    for (int i = 0; i < v.count; i++) {
      assert(vibrationStrength(p, at) == v.steps[i].strength &&
             vibrationStrength(p, at + v.steps[i].ms - 1) == v.steps[i].strength);
      at += v.steps[i].ms;
    }
    assert(vibrationStrength(p, total) == -1 && vibrationStrength(p, 100000) == -1);
  }
  assert(int(names.size()) == VIBRATION_COUNT);
  std::cout << "PASS: " << VIBRATION_COUNT
            << " distinct vibration patterns (Single = original 120 ms), pulses >=50 ms, <=600 ms "
               "total, step timing and end\n";
}
