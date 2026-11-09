#include <cassert>
#include <iostream>
#include <Arduino.h>
#include "services/power.h"
int main() {
  using namespace device;
  powerBegin(100, 180);
  assert(fakePWM[1] == 100 && fakePWM[2] == 180);
  assert(lightEnabled(false, false) && lightEnabled(true, false));
  activity();
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  powerBegin(100, 180);
  assert(!powerState().displayEnabled && !powerState().keyboardEnabled);
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  auto activityAt = powerState().lastActivity;
  flashKeyboardNotification();
  assert(fakePWM[2] == 191 && fakePWM[1] == 0);
  fakeMillis = 160;
  powerLoop();
  assert(fakePWM[2] == 0);
  fakeMillis = 300;
  powerLoop();
  assert(fakePWM[2] == 191);
  fakeMillis = 460;
  powerLoop();
  assert(fakePWM[2] == 0);
  fakeMillis = 600;
  powerLoop();
  assert(!notificationFlashing() && fakePWM[2] == 0);
  assert(powerState().lastActivity == activityAt);
  assert(powerState().notificationFlashes == 1);
  flashKeyboardNotification();
  assert(!notificationFlashing());
  fakeMillis = 2100;
  powerLocked(true);
  flashKeyboardNotification();
  assert(fakePWM[2] == 191 && fakePWM[1] == 0);
  fakeMillis = 2260;
  powerLoop();
  fakeMillis = 2400;
  powerLoop();
  assert(fakePWM[2] == 191);
  fakeMillis = 2560;
  powerLoop();
  fakeMillis = 2700;
  powerLoop();
  assert(fakePWM[2] == 0);
  powerLocked(false);
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  lightEnabled(true, true);
  assert(fakePWM[2] == 180);
  powerLocked(true);
  auto beforePreview = powerState().lastActivity;
  unlockLightPreview(true);
  assert(fakePWM[1] == 0 && fakePWM[2] == 180);
  fakeMillis += 60000;
  powerLoop();
  assert(fakePWM[2] == 180 && powerState().lastActivity == beforePreview);
  unlockLightPreview(false);
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  lightEnabled(false, true);
  unlockLightPreview(true);
  assert(fakePWM[1] == 100 && fakePWM[2] == 180);
  powerLocked(false);
  assert(fakePWM[1] == 100 && fakePWM[2] == 180);
  powerLocked(true);
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  lightEnabled(false, false);
  lightEnabled(true, false);
  unlockLightPreview(true);
  assert(fakePWM[1] == 0 && fakePWM[2] == 0);
  unlockLightPreview(false);
  std::cout << "PASS: persistent light settings, flashes and unlock preview respecting OFF, "
               "intensity, expired timers, cancellation and activity timestamp\n";
}
