#include <cassert>
#include <vector>
#include <iostream>
#include "services/gps_probe.h"
static std::vector<uint8_t> packet(uint8_t id, std::vector<uint8_t> data) {
  std::vector<uint8_t> f = {0xb5, 0x62, 0x0a, id, uint8_t(data.size()), uint8_t(data.size() >> 8)};
  f.insert(f.end(), data.begin(), data.end());
  uint8_t a = 0, b = 0;
  for (size_t i = 2; i < f.size(); i++) {
    a += f[i];
    b += a;
  }
  f.push_back(a);
  f.push_back(b);
  return f;
}
int main() {
  device::GpsProbe p;
  std::vector<std::vector<uint8_t>> sent;
  auto send = [&](const uint8_t* d, size_t n) { sent.emplace_back(d, d + n); };
  assert(p.start() && !p.start());
  p.tick(100, send);
  assert(sent.size() == 1 && sent[0] == packet(4, {}));
  p.tick(200, send);
  assert(sent.size() == 1);
  std::vector<uint8_t> ver(40);
  ver[0] = 'M';
  ver[1] = '1';
  ver[2] = '0';
  auto f = packet(4, ver);
  for (auto c : f)
    assert(p.feed(c, 300));
  assert(p.versionValid && p.version.find("M10") != std::string::npos);
  p.tick(301, send);
  assert(sent.size() == 2 && sent[1] == packet(0x38, {}));
  std::vector<uint8_t> rf(28);
  rf[1] = 1;
  rf[5] = 3;
  rf[6] = 2;
  rf[7] = 1;
  rf[16] = 0x34;
  rf[17] = 0x12;
  rf[18] = 0x78;
  rf[19] = 0x16;
  rf[20] = 42;
  f = packet(0x38, rf);
  auto broken = f;
  broken.back() ^= 1;
  for (auto c : broken)
    p.feed(c, 400);
  assert(p.bad == 1 && !p.rfValid && p.busy());
  for (auto c : f)
    p.feed(c, 401);
  assert(!p.busy() && p.rfValid && p.antenna == 2 && p.antennaPower == 1 && p.jamming == 3 &&
         p.noise == 0x1234 && p.agc == 0x1678 && p.cwSuppression == 42);
  assert(!p.feed('$', 500));
  assert(p.feed(0xb5, 500));
  assert(!p.feed('$', 501));
  assert(p.start());
  p.tick(0xfffffff0, send);
  p.tick(0x5cc, send);
  assert(p.timeouts == 1 && p.waiting);
  p.tick(0xba8, send);
  assert(p.timeouts == 2 && !p.busy());
  p.start();
  p.cancel();
  assert(!p.busy());
  for (auto c : std::vector<uint8_t>{0xb5, 0x62, 0x0a, 4, 0xff, 0xff})
    p.feed(c, 1000);
  assert(p.bad == 2 && !p.feed('$', 1001));
  device::GpsProbe r;
  sent.clear();
  uint8_t cfg[] = {1, 0, 0, 0};
  assert(!r.poll(0x06, 0x8a, cfg, 4) && !r.poll(0x06, 0x01, nullptr, 0) &&
         !r.poll(0x02, 0x15, nullptr, 0));
  cfg[0] = 0;
  assert(r.poll(0x0a, 0x31, nullptr, 0) && !r.poll(0x01, 0x35, nullptr, 0) && !r.start());
  r.tick(10, send);
  assert(sent.size() == 1 && sent[0] == packet(0x31, {}) && r.rawState == r.RawWaiting);
  f = packet(0x31, {1, 2, 3});
  for (auto c : f)
    r.feed(c, 20);
  assert(r.rawState == r.RawDone &&
         r.rawDiagnostic(20).find("payload=010203") != std::string::npos);
  assert(r.poll(0x06, 0x8b, cfg, 4));
  r.tick(30, send);
  std::vector<uint8_t> nak = {0xb5, 0x62, 0x05, 0x00, 2, 0, 0x06, 0x8b};
  uint8_t na = 0, nb = 0;
  for (size_t i = 2; i < nak.size(); i++) {
    na += nak[i];
    nb += na;
  }
  nak.push_back(na);
  nak.push_back(nb);
  for (auto c : nak)
    r.feed(c, 40);
  assert(r.rawState == r.RawNak);
  assert(r.poll(0x01, 0x35, nullptr, 0));
  r.tick(50, send);
  r.tick(1600, send);
  assert(r.rawState == r.RawTimeout && r.start());
  std::cout
      << "PASS: read-only raw NAV/MON/CFG-VALGET polls (CFG writes rejected), reply/NAK/timeout\n";
  std::cout
      << "PASS: exact read-only MON-VER/MON-RF polls, serialized replies/timeout/wrap/cancel, "
         "bounded UBX/checksum rejection, RF offsets and NMEA coexistence\n";
}
