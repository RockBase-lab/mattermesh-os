#include <cassert>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <Arduino.h>
#include <TDeckMaxBoard.h>
#include "services/location.h"
#include "services/gps_assist.h"
#include "services/gps_backup.h"
#include <vector>
#include <algorithm>
#include "services/time_sync_policy.h"
namespace device {
bool timeSyncGPS(uint32_t, bool, bool) {
  return false;
}
} // namespace device
static bool checksumOk(const device::UbxFrame& f) {
  if (f.size < 8 || f.bytes[0] != 0xb5 || f.bytes[1] != 0x62 ||
      f.size != 8u + f.bytes[4] + (f.bytes[5] << 8))
    return false;
  uint8_t a = 0, b = 0;
  for (size_t i = 2; i < f.size - 2; i++) {
    a += f.bytes[i];
    b += a;
  }
  return f.bytes[f.size - 2] == a && f.bytes[f.size - 1] == b;
}
static int32_t i32(const uint8_t* p) {
  return int32_t(uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 |
                 uint32_t(p[3]) << 24);
}
static std::string sentence(std::string text) {
  unsigned crc = 0;
  for (char c : text)
    crc ^= uint8_t(c);
  char end[8];
  snprintf(end, sizeof(end), "*%02X\r\n", crc);
  return "$" + text + end;
}
int main() {
  using namespace device;
  auto ana = ubxEnableAssistAutonomous();
  const uint8_t anaPayload[] = {0, 3, 0, 0, 0x01, 0x00, 0x23, 0x10, 1};
  assert(checksumOk(ana) && ana.bytes[2] == 0x06 && ana.bytes[3] == 0x8a && ana.size == 17 &&
         !memcmp(ana.bytes + 6, anaPayload, 9));
  for (auto when : {gpsUtcEpoch(2026, 10, 18, 12, 34, 56), gpsUtcEpoch(2028, 2, 29, 23, 59, 59),
                    gpsUtcEpoch(2024, 1, 1, 0, 0, 0), gpsUtcEpoch(2099, 12, 31, 0, 0, 1)}) {
    auto t = ubxTimeUtc(when);
    const uint8_t* p = t.bytes + 6;
    assert(checksumOk(t) && t.bytes[2] == 0x13 && t.bytes[3] == 0x40 && t.bytes[4] == 24 &&
           p[0] == 0x10 && p[1] == 0 && p[2] == 0 && p[3] == 0x80);
    assert(gpsUtcEpoch(p[4] | p[5] << 8, p[6], p[7], p[8], p[9], p[10]) == when &&
           (p[16] | p[17] << 8) == GPS_ASSIST_TIME_ACCURACY_S);
  }
  assert(ubxTimeUtc(0).size == 0 && ubxTimeUtc(GPS_ASSIST_MIN_EPOCH - 1).size == 0);
  auto pos = ubxPositionLlh(45.9341234, -7.6);
  assert(checksumOk(pos) && pos.bytes[4] == 20 && pos.bytes[6] == 1 &&
         i32(pos.bytes + 10) == 459341234 && i32(pos.bytes + 14) == -76000000 &&
         i32(pos.bytes + 18) == 0 &&
         uint32_t(i32(pos.bytes + 22)) == GPS_ASSIST_POSITION_ACCURACY_CM);
  assert(ubxPositionLlh(0, 0).size == 0 && ubxPositionLlh(NAN, 1).size == 0 &&
         ubxPositionLlh(91, 1).size == 0 && ubxPositionLlh(1, 181).size == 0);
  // Location service: frames go out once per power-on, only after valid NMEA fixes the baud.
  ExtensionIOXL9555 io;
  locationBegin(io, true);
  assert(locationSet(45.9, 7.6));
  assert(locationEnabled(false) && locationEnabled(true));
  SerialGPS.written.clear();
  locationLoop();
  assert(SerialGPS.written.empty() &&
         locationReceiverDiagnostic().find("assist=pending") != std::string::npos);
  SerialGPS.inject(sentence("GPGGA,014600.00,,,,,0,00,99.9,,,,,,"));
  locationLoop();
  std::string expected = std::string((const char*)ana.bytes, ana.size);
  assert(SerialGPS.written.compare(0, expected.size(), expected) == 0);
  auto ack = ubxEnableAckAiding();
  const uint8_t ackPayload[] = {0, 1, 0, 0, 0x25, 0x00, 0x11, 0x10, 1};
  assert(checksumOk(ack) && ack.size == 17 && !memcmp(ack.bytes + 6, ackPayload, 9) &&
         SerialGPS.written.substr(17, 17) == std::string((const char*)ack.bytes, 17));
  auto sent = SerialGPS.written.size();
  assert(sent == 17 + 17 + 32 + 28 &&
         locationReceiverDiagnostic().find("assist=ana+time+position") != std::string::npos &&
         locationReceiverDiagnostic().find("database") == std::string::npos);
  assert(SerialGPS.written.substr(sent - 28) ==
         std::string((const char*)ubxPositionLlh(45.9, 7.6).bytes, 28));
  SerialGPS.inject(sentence("GPGGA,014601.00,,,,,0,00,99.9,,,,,,"));
  locationLoop();
  assert(SerialGPS.written.size() == sent);
  assert(locationEnabled(false) && locationEnabled(true));
  SerialGPS.written.clear();
  SerialGPS.inject(sentence("GPGGA,014602.00,,,,,0,00,99.9,,,,,,"));
  locationLoop();
  assert(SerialGPS.written.size() == sent);
  // Store attached with a saved navigation database: replayed after the aiding frames, one entry
  // per MGA-ACK; the store is a host callback (SD on the device).
  static std::vector<uint8_t> saved;
  static int loads = 0, savesDone = 0;
  static std::vector<uint8_t> savedOut;
  locationStore({[](std::vector<uint8_t>& out) {
                   loads++;
                   out = saved;
                   return !saved.empty();
                 },
                 [](const std::vector<uint8_t>& d) {
                   savesDone++;
                   savedOut = d;
                   return true;
                 }});
  GpsBackup::frame(saved, 0x13, 0x80, nullptr, 0);
  saved.clear();
  saved.assign(GpsBackup::HeaderSize, 0);
  saved[0] = 'M';
  saved[1] = 'M';
  saved[2] = 'G';
  saved[3] = 'N';
  saved[4] = 1;
  const uint8_t e1[] = {1, 2, 3}, e2[] = {4, 5};
  GpsBackup::frame(saved, 0x13, 0x80, e1, 3);
  GpsBackup::frame(saved, 0x13, 0x80, e2, 2);
  assert(locationEnabled(false) && locationEnabled(true));
  SerialGPS.written.clear();
  SerialGPS.inject(sentence("GPGGA,014603.00,,,,,0,00,99.9,,,,,,"));
  locationLoop();
  assert(loads == 1 && locationReceiverDiagnostic().find("+database") != std::string::npos &&
         SerialGPS.written.size() == sent + 11 &&
         SerialGPS.written.substr(sent) ==
             std::string((const char*)saved.data() + GpsBackup::HeaderSize, 11));
  fakeMillis += GpsBackup::AckTimeoutMs;
  locationLoop();
  assert(SerialGPS.written.size() == sent + 21 &&
         locationBackupDiagnostic().find("restores=0") !=
             std::string::npos); // second entry after the ACK timeout
  fakeMillis += GpsBackup::AckTimeoutMs;
  locationLoop();
  assert(SerialGPS.written.size() == sent + 21 &&
         locationBackupDiagnostic().find("restores=1") != std::string::npos);
  // Fix seen: a dump is polled 30 s later and the reply (two entries + MGA-ACK) is saved through
  // the store with the frames verbatim.
  SerialGPS.inject(sentence("GPGGA,014604.00,4424.0,N,00854.0,E,1,06,1.0,100.0,M,,M,,"));
  locationLoop();
  assert(locationState().fix);
  SerialGPS.written.clear();
  fakeMillis += GPS_BACKUP_AFTER_FIX_MS - 1;
  locationLoop();
  assert(SerialGPS.written.empty());
  fakeMillis++;
  locationLoop();
  std::vector<uint8_t> poll;
  GpsBackup::frame(poll, 0x13, 0x80, nullptr, 0);
  assert(SerialGPS.written == std::string((const char*)poll.data(), poll.size()));
  std::vector<uint8_t> reply;
  GpsBackup::frame(reply, 0x13, 0x80, e1, 3);
  GpsBackup::frame(reply, 0x13, 0x80, e2, 2);
  const uint8_t ackEnd[] = {1, 0, 0, 0x80, 2, 0, 0, 0};
  GpsBackup::frame(reply, 0x13, 0x60, ackEnd, 8);
  SerialGPS.inject(std::string((const char*)reply.data(), reply.size()));
  locationLoop();
  assert(savesDone == 1 && GpsBackup::count(savedOut) == 2 &&
         std::equal(savedOut.begin() + GpsBackup::HeaderSize, savedOut.end(),
                    saved.begin() + GpsBackup::HeaderSize) &&
         locationBackupDiagnostic().find("saves=1") != std::string::npos);
  // Lease release with a fix: the GPS stays on for the dump, then goes off once the reply is in (or
  // after the deadline).
  auto drain = [&] {
    for (int i = 0; i < 3; i++) {
      fakeMillis += GpsBackup::AckTimeoutMs;
      locationLoop();
    }
    assert(locationBackupDiagnostic().find("phase=idle") != std::string::npos);
  };
  fakeMillis += GPS_BACKUP_BEFORE_OFF_MS;
  assert(locationEnabled(false) && locationTimeLease(true) && locationState().enabled &&
         locationState().timeLease);
  SerialGPS.written.clear();
  SerialGPS.inject(sentence("GPGGA,014605.00,4424.0,N,00854.0,E,1,06,1.0,100.0,M,,M,,"));
  locationLoop();
  drain();
  fakeMillis += GPS_BACKUP_INTERVAL_MS;
  locationLoop();
  SerialGPS.inject(std::string((const char*)reply.data(), reply.size()));
  locationLoop();
  assert(savesDone == 2);
  fakeMillis += GPS_BACKUP_BEFORE_OFF_MS; // the 10 min interval also spans power cycles
  SerialGPS.written.clear();
  assert(locationTimeLease(false) && locationState().enabled && locationState().timeLease &&
         SerialGPS.written ==
             std::string((const char*)poll.data(), poll.size())); // still on: dumping first
  locationLoop();
  assert(locationState().enabled);
  fakeMillis += GPS_BACKUP_OFF_DEADLINE_MS;
  locationLoop();
  assert(!locationState().enabled && !locationState().timeLease &&
         savesDone == 2); // deadline: nothing came back, off anyway
  assert(locationTimeLease(true));
  SerialGPS.inject(sentence("GPGGA,014606.00,4424.0,N,00854.0,E,1,06,1.0,100.0,M,,M,,"));
  locationLoop();
  drain();
  fakeMillis += GPS_BACKUP_INTERVAL_MS;
  locationLoop();
  SerialGPS.inject(std::string((const char*)reply.data(), reply.size()));
  locationLoop();
  assert(savesDone == 3);
  fakeMillis += GPS_BACKUP_BEFORE_OFF_MS;
  assert(locationTimeLease(false) && locationState().enabled);
  SerialGPS.inject(std::string((const char*)reply.data(), reply.size()));
  locationLoop();
  assert(savesDone == 4 && !locationState().enabled); // reply in: saved, then off
  assert(locationTimeLease(true) && locationTimeLease(false) &&
         !locationState().enabled); // saved 0 s ago: off at once
  assert(locationEnabled(true));
  SerialGPS.inject(sentence("GPGGA,014607.00,4424.0,N,00854.0,E,1,06,1.0,100.0,M,,M,,"));
  locationLoop();
  drain();
  fakeMillis += GPS_BACKUP_BEFORE_OFF_MS;
  assert(locationTimeLease(true) && locationTimeLease(false) &&
         locationState().enabled); // user ON: a lease release never powers off
  std::cout << "PASS: navigation database store: replay after aiding (ACK paced), dump 30 s after "
               "the fix saved verbatim, lease release dumps first then powers off "
               "(deadline/reply), user ON untouched\n";
  std::cout << "PASS: UBX aiding frames (ANA RAM+BBR, MGA-INI time/position) checksummed, calendar "
               "round-trip, invalid inputs dropped, sent once per GPS power-on after NMEA\n";
}
