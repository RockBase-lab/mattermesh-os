#include <cassert>
#include <iostream>
#include <vector>
#include "ui/components.h"
static bool pixel(const std::vector<uint8_t>& b, int x, int y) {
  return b[y * (mm::W / 8) + x / 8] & (128 >> (x % 8));
}
int main() {
  GFXcanvas1 frame(mm::W, mm::H);
  mm::Components c(frame);
  mm::Status s;
  for (int mask = 0; mask < 16; mask++)
    for (bool charging : {false, true}) {
      s = mm::Status{};
      s.clock = "23:59";
      s.unread = mask & 1;
      s.gps = mask & 2;
      s.wifi = mask & 4;
      s.bt = mask & 8;
      s.charging = charging;
      int count = !!(mask & 1) + !!(mask & 2) + !!(mask & 4) + !!(mask & 8);
      s.battery = 100;
      auto full = c.statusLayout(s);
      c.statusBar(s);
      std::vector<uint8_t> before(frame.getBuffer(), frame.getBuffer() + mm::W / 8 * mm::H);
      s.battery = 99;
      auto narrow = c.statusLayout(s);
      c.statusBar(s);
      int shift = full.percentWidth - narrow.percentWidth;
      assert(shift > 0 && narrow.batteryX - full.batteryX == shift &&
             narrow.iconsRight - full.iconsRight == shift);
      int first = count ? full.iconsRight - ((count - 1) * mm::StatusIconStep + mm::IconStatus)
                        : full.batteryX;
      for (int y = 0; y < mm::StatusH - 1; y++)
        for (int x = first; x < full.batteryX + mm::StatusBatteryW; x++)
          assert(pixel(before, x, y) == bool(frame.getPixel(x + shift, y)));
      for (int y = 0; y < mm::StatusH; y++)
        for (int x = 0; x < mm::StatusAntennaX + mm::StatusAntennaW; x++)
          assert(pixel(before, x, y) == bool(frame.getPixel(x, y)));
      for (int percent : {100, 99, 50, 10, 9, 1, 0, -1, 101}) {
        s.battery = percent;
        auto layout = c.statusLayout(s);
        c.statusBar(s);
        assert(layout.percentX + layout.percentWidth == mm::W - mm::StatusRightMargin);
        assert(layout.batteryX + mm::StatusBatteryW + mm::StatusBatteryGap == layout.percentX);
        assert(layout.iconsRight + mm::StatusIndicatorGap == layout.batteryX);
        int start =
            layout.iconsRight - (count ? (count - 1) * mm::StatusIconStep + mm::IconStatus : 0);
        assert(start > mm::StatusAntennaX + mm::StatusAntennaW);
        for (int y = mm::StatusIconY; y < mm::StatusIconY + mm::IconStatus; y++)
          for (int x = mm::StatusIconsX; x < start; x++)
            assert(!frame.getPixel(x, y));
        for (int i = 0; i < count; i++) {
          bool ink = false;
          for (int y = 2; y < 14; y++)
            for (int x = start + i * mm::StatusIconStep;
                 x < start + i * mm::StatusIconStep + mm::IconStatus; x++)
              ink = ink || frame.getPixel(x, y);
          assert(ink);
        }
      }
    }
  s = mm::Status{};
  s.battery = 0;
  c.statusBar(s);
  auto empty = c.statusLayout(s);
  assert(frame.getPixel(empty.batteryX, mm::StatusBatteryY) &&
         !frame.getPixel(empty.batteryX + 3, mm::StatusBatteryY + 3));
  s.battery = 100;
  c.statusBar(s);
  auto full = c.statusLayout(s);
  assert(frame.getPixel(full.batteryX + 3, mm::StatusBatteryY + 3));
  s.battery = 10;
  auto two = c.statusLayout(s);
  s.battery = 9;
  auto one = c.statusLayout(s);
  assert(one.batteryX > two.batteryX);
  std::cout
      << "PASS: dynamic 100->99 and10->9 status geometry, all16 icon masks, charging, actual "
         "translated pixels, no clock/antenna movement or overlap, unknown/invalid percentages\n";
}
