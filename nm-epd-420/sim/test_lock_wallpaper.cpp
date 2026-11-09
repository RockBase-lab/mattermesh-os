#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/renderer.h"
#include "ui/lock_wallpapers.h"
// The byte-wise wallpaper path (GFXcanvas1) must paint exactly what the per-pixel path (any
// Adafruit_GFX) paints.
int main() {
  for (int w = 0; w < mm_wallpaper::count; w++)
    for (bool message : {false, true}) {
      mm::State s;
      s.wallpaper = w;
      s.lockHasMessage = message;
      s.status.clock = "12:34";
      s.date = "Thu, Oct 23";
      GFXcanvas1 fast(mm::W, mm::H), slow(mm::W, mm::H);
      mm::Renderer(fast).lock(s);
      Adafruit_GFX& base = slow;
      mm::Renderer(base).lock(s);
      assert(!memcmp(fast.getBuffer(), slow.getBuffer(), size_t(mm::W / 8) * mm::H));
    }
  std::cout << "PASS: lock wallpaper byte path matches the per-pixel path for every wallpaper, "
               "with and without a message\n";
}
