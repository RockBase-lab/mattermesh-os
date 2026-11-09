#pragma once

// POSIX TZ rules used by ESP-IDF and newlib. Keep the default compatible with earlier firmware.
namespace device {
struct TimeZoneOption {
  const char* label;
  const char* posix;
};
constexpr TimeZoneOption TIME_ZONES[] = {
    {"UTC", "UTC0"},
    {"Central Europe (UTC+1 / DST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"United Kingdom (UTC+0 / DST)", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"China (UTC+8)", "CST-8"},
    {"Japan (UTC+9)", "JST-9"},
    {"Korea (UTC+9)", "KST-9"},
    {"India (UTC+5:30)", "IST-5:30"},
    {"US Eastern (UTC-5 / DST)", "EST5EDT,M3.2.0,M11.1.0"},
    {"US Central (UTC-6 / DST)", "CST6CDT,M3.2.0,M11.1.0"},
    {"US Mountain (UTC-7 / DST)", "MST7MDT,M3.2.0,M11.1.0"},
    {"US Pacific (UTC-8 / DST)", "PST8PDT,M3.2.0,M11.1.0"},
    {"Australia Eastern (UTC+10 / DST)", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"New Zealand (UTC+12 / DST)", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
};
constexpr int TIME_ZONE_COUNT = int(sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]));
constexpr int DEFAULT_TIME_ZONE = 1;
} // namespace device
