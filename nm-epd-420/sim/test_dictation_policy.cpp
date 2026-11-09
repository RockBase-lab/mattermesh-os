#include <cassert>
#include <iostream>
#include "services/dictation_policy.h"
int main() {
  using namespace device;
  DictationHold hold;
  assert(hold.press(MicSource::Keyboard));
  assert(!hold.press(MicSource::Keyboard));
  assert(!hold.release(MicSource::Touch));
  assert(hold.release(MicSource::Keyboard));
  assert(!hold.release(MicSource::Keyboard));
  assert(hold.press(MicSource::Touch));
  assert(hold.release(MicSource::Touch));
  assert(hold.press(MicSource::Keyboard));
  hold.cancel();
  assert(hold.source == MicSource::None);
  assert(DICTATION_MAX_BYTES == 192000 && !OFFLINE_ITALIAN_AVAILABLE);
  bool trimmed;
  assert(appendDictation("Existing draft", "Andrea 12 gradi", 96, trimmed) ==
             "Existing draft Andrea 12 gradi" &&
         !trimmed);
  assert(appendDictation(std::string(96, 'x'), "test", 96, trimmed) == std::string(96, 'x') &&
         trimmed);
  assert(appendDictation("1234", "è", 6, trimmed) == "1234" && trimmed);
  assert(appendDictation("1234", "è", 7, trimmed) == "1234 è" && !trimmed);
  assert(appendDictation("saved", "", 96, trimmed) == "saved" && !trimmed);
  std::cout << "PASS: momentary ownership/release/outside-source/cancel/no toggle, 6s PCM limit, "
               "no offline model claim, append preserves draft/UTF-8/byte limit\n";
}
