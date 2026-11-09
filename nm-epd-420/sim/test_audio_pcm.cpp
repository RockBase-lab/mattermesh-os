#include <cassert>
#include <iostream>
#include "services/audio_pcm.h"
int main() {
  int16_t raw[8] = {-1000, -32768, 1000, -32768, -1000, -32768, 1000, -32768}, mono[4];
  assert(device::microphoneMono(raw, mono, 4) == 1000);
  assert(mono[0] == -1000 && mono[1] == 1000);
  int16_t other[8] = {0, 2000, 0, -2000, 0, 2000, 0, -2000};
  assert(device::microphoneMono(other, mono, 4) == 2000);
  int16_t silence[8] = {0};
  assert(device::microphoneMono(silence, mono, 4) == 0);
  int16_t dc[8] = {10000, -32768, 10000, -32768, 10000, -32768, 10000, -32768};
  assert(device::microphoneMono(dc, mono, 4) == 0);
  // Start transient: 0..99 ms silent, 100..159 ms linear fade-in, 160 ms+ untouched, short captures
  // safe.
  static int16_t pcm[4000];
  for (auto& v : pcm)
    v = 20000;
  device::suppressCaptureStart(pcm, 4000);
  assert(pcm[0] == 0 && pcm[1599] == 0 && pcm[1600] == 0 && pcm[2080] == 10000 &&
         pcm[2559] > 19000 && pcm[2559] < 20000 && pcm[2560] == 20000 && pcm[3999] == 20000);
  int16_t brief[10] = {-30000, 30000, -30000, 30000, -30000, 30000, -30000, 30000, -30000, 30000};
  device::suppressCaptureStart(brief, 10);
  for (auto v : brief)
    assert(v == 0);
  std::cout << "PASS: speech vs DC reference, either I2S channel, silence and DC removal, start "
               "transient mute/fade\n";
}
