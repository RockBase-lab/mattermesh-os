#include <cassert>
#include <iostream>
#include <limits>
#include "services/touch_hold.h"
#include "services/gps_position_policy.h"
#include "ui/renderer.h"
#include "../hardware/lib/HynTouch/src/TouchContact.h"
int main() {
  HynTouchContact contact;
  contact.report(true, 1, 50, 100);
  assert(contact.down && contact.x == 50 && contact.y == 100);
  contact.report(false, 0, 0, 0);
  assert(contact.down);
  contact.report(true, 0, 0, 0);
  assert(!contact.down);
  contact.report(true, 1, 60, 110);
  contact.reset();
  assert(!contact.down);
  device::TouchHold hold;
  hold.begin(3, 50, 100, 100);
  assert(hold.update(50, 100, 799) == -1);
  assert(hold.update(51, 102, 800) == 3);
  assert(hold.consumed && hold.update(50, 100, 5000) == -1);
  hold.reset();
  hold.begin(2, 50, 100, 100);
  assert(hold.update(63, 100, 500) == -1);
  assert(hold.update(50, 100, 5000) == -1);
  hold.reset();
  hold.begin(-1, 50, 100, 100);
  assert(hold.update(50, 100, 5000) == -1);
  hold.begin(1, 0, 0, 0xffffff00);
  assert(hold.update(0, 0, 500) == 1);
  using device::gpsCoordinateMessage;
  assert(gpsCoordinateMessage(true, true, true, 0, 45.934123, 7.629456) ==
         "GPS: 45.934123, 7.629456");
  assert(gpsCoordinateMessage(true, true, true, 14999, -45.1, -179.2) ==
         "GPS: -45.100000, -179.200000");
  assert(!gpsCoordinateMessage(true, true, true, 0, 0, 0).empty());
  assert(gpsCoordinateMessage(false, true, true, 0, 44, 8).empty());
  assert(gpsCoordinateMessage(true, false, true, 0, 44, 8).empty());
  assert(gpsCoordinateMessage(true, true, false, 0, 44, 8).empty());
  assert(gpsCoordinateMessage(true, true, true, 15000, 44, 8).empty());
  assert(gpsCoordinateMessage(true, true, true, UINT32_MAX, 44, 8).empty());
  assert(gpsCoordinateMessage(true, true, true, 0, 91, 8).empty());
  assert(gpsCoordinateMessage(true, true, true, 0, 44, -181).empty());
  assert(gpsCoordinateMessage(true, true, true, 0, std::numeric_limits<double>::quiet_NaN(), 8)
             .empty());
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer r(canvas);
  mm::State s;
  s.page = mm::Page::Messages;
  s.channelOptions = true;
  s.channelOptionName = "Italia";
  r.draw(s);
  s.channelOptionMuted = true;
  r.draw(s);
  s.deleteChannelConfirm = true;
  r.draw(s);
  for (auto rect :
       {mm::ChannelAction, mm::channelOptionRect(1), mm::channelOptionRect(2), mm::ChatClearYes,
        mm::ChatClearNo, mm::PositionSend, mm::PositionCancel, mm::chatOptionRect(3)}) {
    assert(rect.x >= mm::Margin && rect.x + rect.w <= mm::W - mm::Margin);
    assert(rect.y >= mm::Top && rect.y + rect.h <= mm::Bottom);
  }
  s.channelOptions = false;
  s.page = mm::Page::Chat;
  s.positionShare = true;
  s.gpsAvailable = true;
  s.gpsEnabled = false;
  s.positionMessage = "GPS is off. Enable it to acquire your position.";
  r.draw(s);
  s.gpsEnabled = true;
  s.positionMessage = "Waiting for a fresh GPS fix...";
  r.draw(s);
  s.positionCoordinates = gpsCoordinateMessage(true, true, true, 0, 45.934123, 7.629456);
  r.draw(s);
  std::cout
      << "PASS: long touch threshold/slop/consumed/outside/wrap; live GPS "
         "coordinates/age/OFF/cache-invalid/nonfinite/range; channel and position modal bounds\n";
}
