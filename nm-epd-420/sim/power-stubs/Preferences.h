#pragma once
#include <map>
#include <string>
#include <stdint.h>
inline std::map<std::string, uint32_t> fakePreferences;
struct Preferences {
  bool begin(const char*, bool) {
    return true;
  }
  uint32_t getUInt(const char* key, uint32_t fallback) {
    return fakePreferences.count(key) ? fakePreferences[key] : fallback;
  }
  bool getBool(const char* key, bool fallback) {
    return fakePreferences.count(key) ? bool(fakePreferences[key]) : fallback;
  }
  size_t putUInt(const char* key, uint32_t value) {
    fakePreferences[key] = value;
    return 4;
  }
  size_t putBool(const char* key, bool value) {
    fakePreferences[key] = value;
    return 1;
  }
};
