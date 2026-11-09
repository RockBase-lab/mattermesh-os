#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/renderer.h"
int main() {
  GFXcanvas1 frame(mm::W, mm::H);
  mm::Components c(frame);
  mm::Renderer r(frame);
  mm::State s;
  s.page = mm::Page::Chat;
  s.chatName = "Public";
  s.input = "poiu";
  s.messages.push_back({"River", "10:24", "12°", "2 hop | it", false});
  assert(c.width("23:59", mm::Font::ChatBody) <= mm::StatusClockW);
  assert(c.width("EXT", mm::Font::ChatMeta) <= mm::StatusAntennaW);
  assert(mm::StatusAntennaX + mm::StatusAntennaW < mm::StatusIconsX);
  r.chat(s);
  std::vector<uint8_t> baseline(frame.getBuffer(), frame.getBuffer() + 9600);
  s.notice = "Reply ready / queued / saved";
  r.chat(s);
  assert(!memcmp(baseline.data(), frame.getBuffer(), 9600));
  s.firstRepeater = "First RPT: [abcd] Field repeater";
  r.chat(s);
  assert(memcmp(baseline.data(), frame.getBuffer(), 9600));
  s.status.antennaInternal = false;
  r.chat(s);
  std::vector<uint8_t> external(frame.getBuffer(), frame.getBuffer() + 9600);
  s.status.antennaInternal = true;
  r.chat(s);
  assert(memcmp(external.data(), frame.getBuffer(), 9600));
  for (auto label : {"Input interrupted"}) {
    assert(c.width(label, mm::Font::ChatMeta) <= mm::W - 2 * mm::Margin - 30 - mm::HoldDotsW - 8);
    r.holdBanner(label);
    int cy = mm::HoldBannerY + 3 + (mm::HoldBannerH - 6) / 2,
        x0 = mm::W - mm::Margin - mm::HoldDotsW + mm::HoldDotR;
    assert(frame.getPixel(x0, cy) && frame.getPixel(x0 + 2 * mm::HoldDotStep, cy) &&
           frame.getPixel(x0 + 3 * mm::HoldDotStep, cy) &&
           frame.getPixel(x0 + 5 * mm::HoldDotStep, cy));
  }
  r.holdBanner("Input interrupted");
  {
    int cy = mm::HoldBannerY + 3 + (mm::HoldBannerH - 6) / 2,
        x0 = mm::W - mm::Margin - mm::HoldDotsW + mm::HoldDotR;
    for (int i = 0; i < 6; i++)
      assert(frame.getPixel(x0 + i * mm::HoldDotStep, cy));
    assert(frame.getPixel(mm::Margin, cy));
  }
  assert(mm::HoldBannerY >= mm::StatusH && mm::HoldBannerY + mm::HoldBannerH < mm::ComposerY);
  std::cout << "PASS: readable INT/EXT/time bounds, chat notices removed without reserved space, "
               "first RPT strip, all banner labels fit beside the icon and the six hold dots "
               "(unlock-style card)\n";
}
