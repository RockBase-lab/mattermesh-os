#include <cassert>
#include <iostream>
#include <vector>
#include "ui/panel_blit.h"
struct Panel {
  std::vector<int> calls;
  const uint8_t* expected;
  bool inverted = false;
  void writeImageForFullRefresh(const uint8_t* p, int x, int y, int w, int h, bool inv, bool mirror,
                                bool pgm) {
    assert(p == expected && x == 0 && y == 0 && w == 240 && h == 320 && inv && !mirror && !pgm);
    calls.push_back(1);
  }
  void refresh(bool part) {
    assert(!part);
    calls.push_back(2);
  }
  void writeImageAgain(const uint8_t* p, int, int, int, int, bool inv, bool, bool) {
    assert(p == expected && inv);
    calls.push_back(3);
  }
  void powerOff() {
    calls.push_back(4);
  }
  void writeImagePart(const uint8_t* p, int xp, int yp, int w, int h, int x, int y, int rw, int rh,
                      bool inv, bool mir, bool pgm) {
    assert(p == expected && xp == x && yp == y && x == 80 && y == 100 && w == 240 && h == 320 &&
           rw == 16 && rh == 8 && inv && !mir && !pgm);
    calls.push_back(5);
  }
  void refresh(int x, int y, int w, int h) {
    assert(x == 80 && y == 100 && w == 16 && h == 8);
    calls.push_back(6);
  }
  void refreshTyping(int x, int y, int w, int h) {
    assert(x == 80 && y == 100 && w == 16 && h == 8);
    calls.push_back(8);
  }
  void writeImagePartAgain(const uint8_t* p, int, int, int, int, int, int, int, int, bool inv, bool,
                           bool) {
    assert(p == expected);
    calls.push_back(inv ? 7 : 9);
  }
};
int main() {
  std::vector<uint8_t> data(9600);
  Panel p;
  p.expected = data.data();
  mm::RefreshPlan plan;
  mm::paintFramebuffer(p, data.data(), plan);
  assert(p.calls.empty());
  plan.changed = plan.full = true;
  mm::paintFramebuffer(p, data.data(), plan);
  assert((p.calls == std::vector<int>{1, 2, 3, 4}));
  p.calls.clear();
  plan.full = false;
  plan.area = {80, 100, 16, 8};
  mm::paintFramebuffer(p, data.data(), plan);
  assert((p.calls == std::vector<int>{5, 6, 7, 4})); // partial update then panel power off
  p.calls.clear();
  plan.typing = true;
  mm::paintFramebuffer(p, data.data(), plan);
  assert((p.calls == std::vector<int>{5, 8, 7, 4})); // typing waveform instead of the OTP partial
  plan.typing = false;
  p.calls.clear();
  mm::repaintArea(p, data.data(), plan.area);
  assert((p.calls ==
          std::vector<int>{
              9, 5, 6, 7,
              4})); // inverse into the previous buffer, image, OTP partial, image again, power off
  mm::RefreshTracker t;
  t.commit(data.data());
  data[0] = 1;
  data[9599] = 128;
  auto delta = t.plan(data.data(), false, 0);
  assert(delta.changed && !delta.full && delta.changedPixels == 2 && delta.area.w == 240 &&
         delta.area.h == 320);
  t.commit(data.data(), delta);
  assert(!t.plan(data.data(), false, mm::PartialLimit).changed);
  data[1] = 1;
  {
    auto l = t.plan(data.data(), false, mm::PartialLimit);
    assert(!l.full && t.maintenanceDue());
  } // soft limit: partial now, full deferred to idlet.reset();t.commit(data.data());for(int
    // i=0;i<6;i++){data[1]^=1;auto
    // d=t.plan(data.data(),false,0);d.area={0,0,240,320};t.commit(data.data(),d);}data[1]^=1;{auto
    // l=t.plan(data.data(),false,0);assert(!l.full&&t.maintenanceDue());}
  std::cout
      << "PASS: exact differential current/previous sequence and inversion, dirty byte crop, "
         "sparse full-bounds partial, no-op maintenance skipped, ghosting area/count budgets\n";
}
