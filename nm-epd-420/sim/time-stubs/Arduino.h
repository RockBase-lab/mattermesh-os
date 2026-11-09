#pragma once
#include <stdint.h>
#include <cstdlib>
#include <ctime>
inline uint32_t fakeMillis = 0;
inline uint32_t millis() {
  return fakeMillis;
}
