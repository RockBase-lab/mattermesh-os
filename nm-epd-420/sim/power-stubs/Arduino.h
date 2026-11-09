#pragma once
#include <stdint.h>
#include <vector>
#include <utility>
inline uint32_t fakeMillis = 0;
inline uint8_t fakePWM[3] = {};
inline std::vector<std::pair<int, int>> pwmWrites;
inline uint32_t millis() {
  return fakeMillis;
}
inline void analogWrite(int pin, uint8_t pwm) {
  fakePWM[pin] = pwm;
  pwmWrites.push_back({pin, pwm});
}
