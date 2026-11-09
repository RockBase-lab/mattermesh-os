#include <cassert>
#include <iostream>
#include "services/policy.h"
#include "services/touch_keys.h"
int main() {
  using namespace device;
  assert(BATTERY_FULL_MV == 4200 && BATTERY_PERCENT_FULL_MV == 4050 &&
         BATTERY_PERCENT_EMPTY_MV == 3400);
  assert(batteryPercent(57, true, 4100, false) == 100);
  assert(batteryPercent(57, true, 4200, false) == 100);
  assert(batteryPercent(57, true, 4000, true) == 100);
  assert(batteryPercent(150, true, 4000, false) == 92);
  assert(batteryPercent(-2, true, 4000, false) == 92);
  assert(batteryPercent(0, false, -1, false) == -1);
  assert(batteryPercent(0, false, 3200, false) == 0);
  assert(batteryPercent(0, false, 4150, false) == 100);
  assert(batteryPercent(0, false, 4050, false) == 100);
  assert(batteryPercent(0, false, 4049, false) == 100);
  assert(batteryPercent(0, false, 3400, false) == 0);
  assert(batteryPercent(0, false, 3401, false) == 0);
  assert(batteryPercent(0, false, 3407, false) == 1);
  assert(batteryPercent(0, false, 3725, false) == 50);
  assert(batteryPercent(0, false, 4000, false, 4132) == 92);
  assert(batteryPercent(0, false, 3300, false, 4132) == 0);
  assert(batteryPercent(0, false, 4100, false, 3300) ==
         100); // calibration no longer moves the
               // curveassert(validTimeout(30)&&validTimeout(0)&&!validTimeout(31));assert(!expired(29999,0,30)&&expired(30000,0,30));assert(!expired(UINT32_MAX,0,0));assert(expired(16000,UINT32_MAX-20000,30));assert(expired(31000,0,15)&&!expired(31000,0,60));std::cout<<"PASS:
               // linear voltage battery percentage, clamping, independent timers, never and millis
               // rollover\n";
  assert(touchKeyAction(TOUCH_KEY_HEART, true) == TouchKeyAction::CursorLeft &&
         touchKeyAction(TOUCH_KEY_MESSAGE, true) == TouchKeyAction::CursorRight &&
         touchKeyAction(TOUCH_KEY_SEND, true) == TouchKeyAction::Send);
  assert(touchKeyAction(TOUCH_KEY_HEART, false) == TouchKeyAction::None &&
         touchKeyAction(TOUCH_KEY_SEND, false) == TouchKeyAction::None &&
         touchKeyAction(3, true) == TouchKeyAction::None);
  std::cout << "PASS: touch keys act only while writing: heart left, message right, send\n";
}
