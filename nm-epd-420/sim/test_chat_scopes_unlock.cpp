#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/renderer.h"
#include "services/unlock_taps.h"
#include "services/scope_policy.h"
#include "services/locked_cpu_policy.h"
int main() {
  using device::normalizedScope;
  for (auto name : {"it", "it-vda", "eu", "us-ca", "*", "region123"})
    assert(normalizedScope(name) == name);
  assert(normalizedScope(" #IT-VDA ") == "it-vda");
  for (auto name : {"", "--", "it--lig", "-it", "it-", "it lig", "unknown", "it/lig", "🇮🇹"})
    assert(normalizedScope(name).empty());
  assert(normalizedScope(std::string(32, 'a')).empty());
  device::UnlockTaps taps;
  assert(taps.press(100));
  assert(!taps.press(200));
  assert(!taps.expire(10000));
  assert(!taps.release(10000));
  assert(taps.taps == 1);
  assert(!taps.press(10010));
  assert(taps.press(10100));
  assert(taps.taps == 2 && taps.release(10200));
  taps.reset();
  assert(taps.press(11000));
  assert(!taps.release(11100));
  assert(taps.expire(11100 + device::UNLOCK_TAP_WINDOW_MS + 1));
  assert(taps.taps == 0);
  taps.reset();
  assert(taps.press(15000));
  assert(!taps.release(15010) && taps.taps == 0);
  assert(taps.press(15050));
  assert(!taps.release(15100));
  assert(taps.press(18000) && taps.taps == 1);
  device::UnlockTaps wrap;
  assert(wrap.press(0xfffffff0));
  assert(!wrap.release(50));
  assert(wrap.press(100));
  assert(wrap.release(180));
  assert(device::lockedCpuFrequency(false, false, false, false) == 40);
  assert(device::lockedCpuFrequency(true, false, false, false) == 80);
  assert(device::lockedCpuFrequency(false, true, false, false) == 80);
  assert(device::lockedCpuFrequency(false, false, true, false) == 80);
  assert(device::lockedCpuFrequency(false, false, false, true) == 80);
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer r(canvas);
  mm::State s;
  s.page = mm::Page::Chat;
  s.chatName = "Italia";
  s.messageMenu = true;
  r.draw(s);
  for (int i = 0; i < 3; i++) {
    auto rect = mm::chatOptionRect(i);
    assert(rect.x >= mm::Margin && rect.x + rect.w <= mm::W - mm::Margin && rect.y >= mm::Top &&
           rect.y + rect.h < mm::Bottom);
  }
  s.messageMenu = false;
  s.scopePicker = true;
  s.regionScopes = {"*", "it", "it-vda", "it-pie", "it-tos", "eu"};
  s.scopeFocus = 2;
  r.draw(s);
  s.scopeEditing = true;
  s.scopeInput = "it-vda";
  r.draw(s);
  s.scopePicker = s.scopeEditing = false;
  s.clearChatConfirm = true;
  r.draw(s);
  s = mm::State{};
  r.lock(s);
  std::vector<uint8_t> idle(canvas.getBuffer(), canvas.getBuffer() + 9600);
  s.unlockTapCount = 1;
  r.lock(s);
  std::vector<uint8_t> one(canvas.getBuffer(), canvas.getBuffer() + 9600);
  s.unlockTapCount = 2;
  r.lock(s);
  assert(memcmp(one.data(), canvas.getBuffer(), 9600));
  for (int y = 0; y < mm::H; y++)
    if (y < mm::HoldBannerY || y >= mm::HoldBannerY + mm::HoldBannerH)
      assert(!memcmp(idle.data() + y * 30, canvas.getBuffer() + y * 30, 30));
  std::cout << "PASS: scope normalization/invalid names, two distinct debounced "
               "taps/held-key/bounce/timeout/wrap, read-message Home route, conditional 40/80 MHz "
               "policy, menu/scope/clear sheets, 1/2 and 2/2 top-only banners\n";
}
