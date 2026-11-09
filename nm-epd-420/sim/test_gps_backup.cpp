#include <cassert>
#include <iostream>
#include <vector>
#include "services/gps_backup.h"
using device::GpsBackup;
static std::vector<std::vector<uint8_t>> sent;
static void send(const uint8_t* d, size_t n) {
  sent.emplace_back(d, d + n);
}
static void feed(GpsBackup& b, const std::vector<uint8_t>& f, uint32_t now) {
  assert(f.size() >= 8);
  b.onFrame(f[2], f[3], f.data() + 6, uint16_t(f.size() - 8), now);
}
int main() {
  std::vector<uint8_t> poll;
  GpsBackup::frame(poll, 0x13, 0x80, nullptr, 0);
  assert(poll.size() == 8 && poll[6] == 0x93 &&
         poll[7] ==
             0xcc); // checksum of 13 80 00 00 (ck_b sums a over class, id and both length bytes)
  const uint8_t e1[] = {1, 2, 3}, e2[] = {4, 5, 6, 7}, e3[] = {9};
  std::vector<uint8_t> f1, f2, f3, ackEnd, ackOne, nav;
  GpsBackup::frame(f1, 0x13, 0x80, e1, 3);
  GpsBackup::frame(f2, 0x13, 0x80, e2, 4);
  GpsBackup::frame(f3, 0x13, 0x80, e3, 1);
  const uint8_t endPayload[] = {1, 0, 0, 0x80, 3, 0, 0, 0},
                onePayload[] = {1, 0, 0, 0x80, 0, 0, 0, 0};
  GpsBackup::frame(ackEnd, 0x13, 0x60, endPayload, 8);
  GpsBackup::frame(ackOne, 0x13, 0x60, onePayload, 8);
  GpsBackup::frame(nav, 0x01, 0x07, e1, 3);
  // Dump: poll, entries collected verbatim, closed by the MGA-ACK for msgId 0x80; other classes
  // ignored.
  GpsBackup b;
  assert(b.idle() && b.startDump(1000, 1760000000u, send) && !b.startDump(1000, 0, send) &&
         sent.size() == 1 && sent[0] == poll && !b.idle());
  feed(b, f1, 1010);
  feed(b, nav, 1011);
  feed(b, f2, 1020);
  b.tick(1500, send);
  assert(!b.idle() && b.entries == 2 && !b.ready);
  feed(b, f3, 1600);
  feed(b, ackEnd, 1601);
  assert(b.idle() && b.ready && b.entries == 3 && b.dumps == 1);
  auto blob = b.take();
  assert(!b.ready && b.blob.empty() && GpsBackup::count(blob) == 3 &&
         GpsBackup::savedEpoch(blob) == 1760000000u &&
         blob.size() == GpsBackup::HeaderSize + f1.size() + f2.size() + f3.size());
  assert(std::vector<uint8_t>(blob.begin() + GpsBackup::HeaderSize,
                              blob.begin() + GpsBackup::HeaderSize + f1.size()) == f1);
  // Dump without the closing ACK: finished after 1.5 s idle; nothing received -> aborted, nothing
  // to save.
  sent.clear();
  assert(b.startDump(2000, 0, send));
  feed(b, f1, 2010);
  b.tick(2010 + GpsBackup::DumpIdleMs - 1, send);
  assert(!b.idle());
  b.tick(2010 + GpsBackup::DumpIdleMs, send);
  assert(b.idle() && b.ready && b.entries == 1);
  b.take();
  assert(b.startDump(3000, 0, send));
  b.tick(3000 + GpsBackup::DumpIdleMs, send);
  assert(b.idle() && !b.ready && b.aborted == 1);
  // Oversized dump aborted.
  assert(b.startDump(4000, 0, send));
  std::vector<uint8_t> big(500, 7), fb;
  GpsBackup::frame(fb, 0x13, 0x80, big.data(), 500);
  for (int i = 0; i < 40; i++)
    feed(b, fb, 4001 + i);
  assert(b.idle() && !b.ready && b.aborted == 2);
  // Validation: magic, version, class/id, checksum, truncation.
  auto bad = blob;
  bad[4] = 2;
  assert(GpsBackup::count(bad) < 0);
  bad = blob;
  bad.back() ^= 1;
  assert(GpsBackup::count(bad) < 0);
  bad = blob;
  bad.pop_back();
  assert(GpsBackup::count(bad) < 0);
  bad = blob;
  bad[GpsBackup::HeaderSize + 3] = 0x40;
  assert(GpsBackup::count(bad) < 0);
  assert(GpsBackup::count(std::vector<uint8_t>(GpsBackup::HeaderSize, 0)) < 0 &&
         GpsBackup::count({}) < 0);
  std::vector<uint8_t> empty(GpsBackup::HeaderSize, 0);
  empty[0] = 'M';
  empty[1] = 'M';
  empty[2] = 'G';
  empty[3] = 'N';
  empty[4] = 1;
  assert(GpsBackup::count(empty) == 0 && !b.startRestore(empty, 0, send));
  // Restore: one entry at a time, next on MGA-ACK or after the 150 ms timeout; done after the last.
  sent.clear();
  assert(b.startRestore(blob, 5000, send) && !b.idle() && sent.size() == 1 && sent[0] == f1);
  b.tick(5100, send);
  assert(sent.size() == 1);
  feed(b, ackOne, 5120);
  assert(b.acks == 1);
  b.tick(5121, send);
  assert(sent.size() == 2 && sent[1] == f2);
  b.tick(5121 + GpsBackup::AckTimeoutMs - 1, send);
  assert(sent.size() == 2);
  b.tick(5121 + GpsBackup::AckTimeoutMs, send);
  assert(sent.size() == 3 && sent[2] == f3 && !b.idle());
  feed(b, ackOne, 5300);
  b.tick(5301, send);
  assert(b.idle() && b.restores == 1 && b.sent == 3 && b.blob.empty());
  assert(!b.startRestore(bad, 6000, send) && b.idle());
  // No dump while restoring, no restore while dumping.
  sent.clear();
  assert(b.startRestore(blob, 7000, send) && !b.startDump(7001, 0, send));
  b.tick(7000 + GpsBackup::AckTimeoutMs, send);
  b.tick(7000 + GpsBackup::AckTimeoutMs * 2, send);
  b.tick(7000 + GpsBackup::AckTimeoutMs * 3, send);
  assert(b.idle() && b.sent == 3);
  assert(b.startDump(8000, 0, send) && !b.startRestore(blob, 8001, send));
  std::cout
      << "PASS: GPS navigation database dump (poll, verbatim entries, ACK/idle close, abort on "
         "empty/oversized), blob validation, ACK-paced replay with timeout, exclusive phases\n";
}
