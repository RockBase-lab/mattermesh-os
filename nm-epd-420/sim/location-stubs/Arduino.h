#pragma once
#include <stdint.h>
#include <stddef.h>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define SERIAL_8N1 0
inline uint32_t fakeMillis = 100;
inline uint32_t millis() {
  return fakeMillis;
}
