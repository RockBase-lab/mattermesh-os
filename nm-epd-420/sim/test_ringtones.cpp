#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include "services/ringtones.h"
int main() {
  using namespace device;
  assert(RINGTONE_COUNT == 15 && std::string(RINGTONES[7].name) == "Gran Vals" &&
         RINGTONES[13].notes[0].toHz == 900 && std::string(RINGTONES[0].name) == "Classic" &&
         RINGTONE_MESSAGE_DEFAULT != RINGTONE_MENTION_DEFAULT);
  assert(ringtoneIndex(-1) == RINGTONE_MESSAGE_DEFAULT &&
         ringtoneIndex(RINGTONE_COUNT) == RINGTONE_MESSAGE_DEFAULT && ringtoneIndex(3) == 3);
  std::set<std::string> names;
  for (int t = 0; t < RINGTONE_COUNT; t++) {
    names.insert(RINGTONES[t].name);
    int frames = ringtoneFrames(t), warm = SPEAKER_RATE * RINGTONE_WARM_MS / 1000, peak = 0,
        nonzero = 0;
    assert(frames > 0 && ringtoneMs(t) <= 1000); // short: never a long or annoying melody
    for (int n = 0; n < RINGTONES[t].count; n++) {
      const auto& note = RINGTONES[t].notes[n];
      assert(note.hz >= 880 && (!note.toHz || note.toHz >= 880) && note.ms >= 45);
    } // audible on the T-Deck speaker
    for (int i = 0; i < frames + 400; i++) {
      int v = ringtoneSample(t, i, 40);
      assert(ringtoneSample(t, i, 0) == 0);
      assert(abs(v) <= SPEAKER_PEAK * 40 / 100 + 1);
      if (i < warm || i >= frames - SPEAKER_RATE * RINGTONE_TAIL_MS / 1000)
        assert(v == 0);
      if (v)
        nonzero++;
      peak = std::max(peak, abs(v));
    }
    // No clicks: the envelope is near zero on the first and last samples of every note.
    int at = warm;
    for (int n = 0; n < RINGTONES[t].count; n++) {
      int length = SPEAKER_RATE * RINGTONES[t].notes[n].ms / 1000;
      for (int k = 0; k < 2; k++) {
        assert(abs(ringtoneSample(t, at + k, 40)) < SPEAKER_PEAK * 40 / 100 / 10);
        assert(abs(ringtoneSample(t, at + length - 1 - k, 40)) < SPEAKER_PEAK * 40 / 100 / 10);
      }
      at += length + SPEAKER_RATE * RINGTONES[t].notes[n].gapMs / 1000;
    }
    assert(peak > SPEAKER_PEAK * 40 / 100 / 3 && nonzero > SPEAKER_RATE / 20);
  }
  assert(int(names.size()) == RINGTONE_COUNT);
  // Classic keeps the original beep: 880 Hz for 220 ms after the 30 ms warm-up.
  assert(RINGTONES[0].count == 1 && RINGTONES[0].notes[0].hz == 880 &&
         RINGTONES[0].notes[0].ms == 220);
  assert(mentionsName("@[Aria T-Deck] ciao", "Aria T-Deck") &&
         mentionsName("ok @[aria t-deck]!", "Aria T-Deck"));
  assert(!mentionsName("Aria T-Deck ciao", "Aria T-Deck") &&
         !mentionsName("@[Aria] ciao", "Aria T-Deck") &&
         !mentionsName("@[Aria T-Deck2]", "Aria T-Deck") && !mentionsName("@[x]", "") &&
         !mentionsName("", "Aria"));
  std::cout
      << "PASS: " << RINGTONE_COUNT
      << " distinct ringtones (Classic unchanged), volume 0 silent, warm-up/tail silence, bounded "
         "level, click-free envelopes; @[name] mention match is exact and case-insensitive\n";
}
