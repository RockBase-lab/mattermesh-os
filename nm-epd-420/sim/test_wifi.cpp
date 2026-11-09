#include <cassert>
#include <iostream>
#include <cstring>
#include "services/wifi_policy.h"
#include "ui/renderer.h"
int main() {
  using namespace device;
  assert(validWifiCredentials("Hidden net", "password123"));
  assert(validWifiCredentials("Open net", ""));
  assert(!validWifiCredentials("", "password123"));
  assert(!validWifiCredentials(std::string(33, 'n'), "password123"));
  assert(!validWifiCredentials("net", "short"));
  assert(!validWifiCredentials("net", std::string(64, 'a')));
  assert(!validWifiCredentials(std::string("a\0b", 3), "password123"));
  assert(!wifiMayCommit(true, false, "new", "new"));
  assert(!wifiMayCommit(true, true, "new", "old"));
  assert(wifiMayCommit(true, true, "new", "new"));
  WifiConnectionStart start;
  start.queue(100);
  assert(!start.ready(200, false));
  assert(!start.ready(500, true));
  assert(start.ready(500, false));
  assert(!start.timedOut(2099));
  assert(start.timedOut(2100));
  start.cancel();
  assert(!start.ready(3000, false) && !start.timedOut(3000));
  start.queue(0xfffffff0);
  assert(start.ready(200, false));
  start.cancel();
  WifiRetryPolicy retry;
  retry.disconnect();
  assert(!retry.due(true, false, false, false, 60000, 0));
  retry.connect();
  assert(retry.due(true, false, false, false, 60000, 0));
  assert(!retry.due(false, false, false, false, 60000, 0));
  assert(!retry.due(true, false, true, false, 60000, 0));
  assert(!retry.due(true, false, false, true, 60000, 0));
  assert(retry.due(true, false, false, false, 100, 0xffff0000));
  std::string password = "AbCd123456789!";
  assert(wifiInputDisplay(password, false) == std::string(password.size(), '*'));
  assert(wifiInputDisplay(password, true) == password);
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer r(canvas);
  mm::State s;
  s.page = mm::Page::Wifi;
  s.wifiEditing = true;
  s.wifiHidden = true;
  s.wifiSSID = std::string(32, 'S');
  s.wifiPasswordMask = std::string(63, '*');
  s.wifiEditField = 1;
  r.draw(s);
  std::vector<uint8_t> hidden(canvas.getBuffer(), canvas.getBuffer() + mm::W / 8 * mm::H);
  s.wifiShowPassword = true;
  s.wifiPasswordMask = password;
  r.draw(s);
  assert(memcmp(hidden.data(), canvas.getBuffer(), hidden.size()));
  for (auto rect : {mm::WifiSSIDInput, mm::WifiPassword, mm::WifiEye, mm::WifiConnect,
                    mm::WifiCancel, mm::WifiManual, mm::WifiDisconnect})
    assert(rect.x >= 0 && rect.y >= mm::Top && rect.x + rect.w <= mm::W &&
           rect.y + rect.h <= mm::Bottom);
  mm::Components c(canvas);
  auto tail = c.inputTail(std::string(63, 'p'), mm::WifiPassword.w - mm::WifiEye.w - 17,
                          mm::Font::ChatBody);
  assert(c.width(tail, mm::Font::ChatBody) + mm::WifiPassword.x + 8 < mm::WifiEye.x);
  std::cout << "PASS: SSID/password bounds, hidden/open, no premature credential commit, manual "
               "disconnect pauses reconnect, input mask/show/cursor tail and panel bounds\n";
}
