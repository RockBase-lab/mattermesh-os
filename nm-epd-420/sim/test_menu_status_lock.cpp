#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/renderer.h"
#include "ui/keyboard_navigation.h"
#include "ui/navigation_catalog.h"
static int ink(GFXcanvas1& canvas, mm::Rect r) {
  int n = 0;
  for (int y = r.y; y < r.y + r.h; y++)
    for (int x = r.x; x < r.x + r.w; x++)
      n += canvas.getPixel(x, y);
  return n;
}
int main() {
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Components c(canvas);
  mm::Renderer renderer(canvas);
  mm::State s;
  for (bool acquiring : {false, true}) {
    s.status = mm::Status{};
    s.status.acquiringTime = acquiring;
    c.statusBar(s.status);
    const int right = c.statusLayout(s.status).iconsRight;
    assert(!ink(canvas,
                {mm::StatusIconsX, mm::StatusIconY, right - mm::StatusIconsX, mm::IconStatus}));
    std::vector<uint8_t> inactive(canvas.getBuffer(), canvas.getBuffer() + 9600);
    s.status.mesh = true;
    c.statusBar(s.status);
    assert(!memcmp(inactive.data(), canvas.getBuffer(), 9600));
    for (int i = 0; i < 4; i++) {
      s.status = mm::Status{};
      s.status.acquiringTime = acquiring;
      if (i == 0)
        s.status.unread = 1;
      else if (i == 1)
        s.status.gps = true;
      else if (i == 2)
        s.status.wifi = true;
      else
        s.status.bt = true;
      c.statusBar(s.status);
      assert(ink(canvas,
                 {right - mm::IconStatus, mm::StatusIconY, mm::IconStatus, mm::IconStatus}) > 0);
      assert(!ink(canvas, {mm::StatusIconsX, mm::StatusIconY,
                           right - mm::IconStatus - mm::StatusIconsX, mm::IconStatus}));
    }
  }
  // An old delivery count must not change pixels, including when scrolled
  // away from the newest outgoing message. Its bubble remains in history.
  s = mm::State{};
  s.page = mm::Page::Chat;
  s.chatName = "Public";
  s.scope = "it";
  s.messages = {{"YOU", "10:12", "One", "", true},
                {"River", "10:13", "Reply", "2 hop | it", false},
                {"YOU", "10:14", "Two", "", true},
                {"River", "10:15", "Received", "3 hop | it", false}};
  s.messages[0].repeaters = 1;
  s.messages[2].repeaters = 2;
  for (int offset : {0, 2}) {
    s.chatOffset = offset;
    renderer.chat(s);
    std::vector<uint8_t> original(canvas.getBuffer(), canvas.getBuffer() + 9600);
    s.messages[0].repeaters += 5;
    renderer.chat(s);
    assert(!memcmp(original.data(), canvas.getBuffer(), 9600));
    bool old = false;
    for (auto hit : renderer.messageBounds())
      if (hit.second == 0)
        old = true;
    assert(old);
  }
  s.chatOffset = 0;
  renderer.chat(s);
  std::vector<uint8_t> original(canvas.getBuffer(), canvas.getBuffer() + 9600);
  s.messages[2].repeaters = 8;
  renderer.chat(s);
  assert(memcmp(original.data(), canvas.getBuffer(), 9600));
  s.latestOwnMessage = -1;
  renderer.chat(s);
  original.assign(canvas.getBuffer(), canvas.getBuffer() + 9600);
  s.messages[2].repeaters = 3;
  renderer.chat(s);
  assert(!memcmp(original.data(), canvas.getBuffer(), 9600));
  // The idle bottom hint stays unchanged. All progress pixels are at the top.
  s = mm::State{};
  renderer.lock(s);
  original.assign(canvas.getBuffer(), canvas.getBuffer() + 9600);
  s.unlockTapCount = 1;
  renderer.lock(s);
  assert(memcmp(original.data() + mm::HoldBannerY * 30, canvas.getBuffer() + mm::HoldBannerY * 30,
                mm::HoldBannerH * 30));
  for (int y = 0; y < mm::H; y++)
    if (y < mm::HoldBannerY || y >= mm::HoldBannerY + mm::HoldBannerH)
      assert(!memcmp(original.data() + y * 30, canvas.getBuffer() + y * 30, 30));
  renderer.unlockTapBanner(1);
  original.assign(canvas.getBuffer(), canvas.getBuffer() + 9600);
  renderer.unlockTapBanner(2);
  assert(memcmp(original.data(), canvas.getBuffer(), 9600));
  // Every tile stays reachable, and edges never wrap into another row.
  assert(mm::moveAppFocus(-1, 's', 6, 3) == 0);
  assert(mm::moveAppFocus(0, 'a', 6, 3) == 0);
  assert(mm::moveAppFocus(0, 'w', 6, 3) == 0);
  int focus = 0;
  for (auto key : {'d', 'd', 's', 'a', 'a', 'w'})
    focus = mm::moveAppFocus(focus, key, 6, 3);
  assert(focus == 0);
  assert(mm::moveAppFocus(2, 'd', 6, 3) == 2);
  assert(mm::moveAppFocus(5, 's', 6, 3) == 5);
  assert(mm::moveAppFocus(4, 's', 5, 3) == 4);
  for (int i = 0; i < 6; i++)
    for (char key : std::string("wasd")) {
      int next = mm::moveAppFocus(i, key, 6, 3);
      assert(next >= 0 && next < 6);
    }
  std::cout
      << "PASS: active-only status icons/GPS including time acquisition, latest own Heard count "
         "only/full history/search, top-only unlock progress, WASD grid bounds and reachability\n";
}
