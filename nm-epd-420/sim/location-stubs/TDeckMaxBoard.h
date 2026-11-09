#pragma once
#include <Arduino.h>
#include <string>
#define BOARD_XL9555_02_GPS_EN 2
#define BOARD_GPS_RXD 44
#define BOARD_GPS_TXD 43
struct FakeUART {
  std::string injected, written;
  size_t cursor = 0;
  bool active = false;
  unsigned baud = 0, reads = 0, begins = 0, ends = 0, bytes = 0;
  void setRxBufferSize(int) {}
  void setTxBufferSize(int) {}
  size_t write(const uint8_t* d, size_t n) {
    written.append((const char*)d, n);
    return n;
  }
  void begin(unsigned rate, int, int, int) {
    active = true;
    baud = rate;
    begins++;
  }
  void end() {
    active = false;
    baud = 0;
    ends++;
    bytes = 0;
  }
  int available() {
    return cursor < injected.size() ? int(injected.size() - cursor) : bytes;
  }
  int read() {
    reads++;
    if (cursor < injected.size())
      return uint8_t(injected[cursor++]);
    if (bytes)
      bytes--;
    return 0;
  }
  void inject(const std::string& s) {
    injected = s;
    cursor = 0;
  }
};
inline FakeUART SerialGPS;
