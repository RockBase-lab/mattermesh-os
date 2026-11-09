#pragma once

#include <stdint.h>

namespace device {

struct StandbySleepState {
  uint32_t entries = 0, errors = 0;
  uint64_t totalMs = 0;
  uint32_t lastMs = 0;
  int lastWake = 0;
};

// Light sleep with the SX1262 left in continuous RX. DIO1, either button or the
// periodic service timer wakes the CPU; RAM and the application state are kept.
bool standbyLightSleep(uint32_t maxMs);
StandbySleepState standbySleepState();

} // namespace device
