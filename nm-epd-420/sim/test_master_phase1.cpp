#include <cassert>
#include <iostream>
#include "services/space_gesture.h"
#include "services/reply_policy.h"
#include "services/received_repeater.h"
#include "services/lock_messages.h"
#include "ui/renderer.h"
#include "ui/lock_wallpapers.h"
#include "ui/refresh.h"
int main() {
  using namespace device;
  auto p = replyPlan("it-vda", 7);
  assert(p.kind == ReplyKind::Scoped && p.scope == "it-vda" && p.hops == 7);
  assert(replyPlan("*", 0).kind == ReplyKind::Scoped);
  assert(replyPlan("unknown", -1).kind == ReplyKind::Scoped);
  assert(replyPlan("", -1).kind == ReplyKind::Scoped);
  assert(incomingRepeater("flood:ab,1234,abcdef") == "abcdef");
  assert(incomingRepeater("flood:").empty());
  assert(incomingRepeater("direct").empty());
  assert(incomingRepeater("flood:xy").empty());
  ReceivedRepeater rx;
  rx.receive(1, 2, "flood:ab,cd", 0xfffffff0);
  assert(rx.visible(20, 6000));
  assert(!rx.visible(7000, 6000));
  rx.receive(2, 3, "direct", 8000);
  assert(!rx.visible(8000, 6000));
  SpaceTaps t;
  assert(t.press(100));
  assert(!t.release(200));
  assert(!t.completed);
  assert(t.press(400));
  assert(t.release(500));
  assert(t.release(501));
  t.reset();
  t.press(100, true);
  assert(!t.release(3100, true) && !t.completed);
  t.reset();
  t.press(100, true);
  t.release(100, true);
  t.press(100, true);
  assert(t.release(100, true));
  t.reset();
  t.press(100);
  t.release(200);
  assert(t.expire(751));
  t.press(1000);
  assert(t.expire(11000));
  t.reset();
  t.press(0xfffffff0, true);
  t.release(20, true);
  t.press(80, true);
  assert(t.release(110, true));
  LockMessages empty;
  empty.reset({});
  assert(!empty.next({}) && empty.ended);
  assert(empty.next({123}) == 123 && !empty.ended);
  LockMessages q;
  q.reset({1, 2, 3});
  assert(q.current == 3);
  assert(q.next({1, 2, 3}) == 2);
  assert(q.next({1, 2, 3}) == 1);
  assert(!q.next({1, 2, 3}) && q.ended);
  assert(!q.next({1, 2, 3}));
  q.receive({1, 2, 3, 4}, 4);
  assert(q.current == 4 && !q.browsing);
  assert(q.next({1, 2, 3, 4}) == 3);
  q.receive({2, 3, 4, 5}, 5);
  assert(q.current == 5);
  assert(q.next({2, 3, 4, 5}) == 4);
  for (uint32_t n = 6; n < 2000; n++) {
    q.next({n - 3, n - 2, n - 1, n});
    assert(q.ids.size() <= 4);
  }
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer renderer(canvas);
  mm::State s;
  renderer.lock(s);
  std::vector<uint8_t> none(canvas.getBuffer(), canvas.getBuffer() + mm::W / 8 * mm::H);
  s.lockHasMessage = true;
  s.receivedPreview = "A new received message";
  renderer.lock(s);
  assert(memcmp(none.data(), canvas.getBuffer(), none.size()));
  assert((mm::LockMessageH - mm::LockCardHeader - 6) /
             mm::Components::lineHeight(mm::Font::ChatBody) >=
         3);
  assert(mm::LockRepeater.y >= mm::LockMessage.y + mm::LockMessage.h + mm::LockRepeaterGap);
  assert(mm::LockRepeater.h >= mm::Components::lineHeight(mm::Font::ChatMeta));
  std::vector<uint8_t> noRepeater(canvas.getBuffer(), canvas.getBuffer() + mm::W / 8 * mm::H);
  s.lockRepeater = "Received via: Mongioie";
  renderer.lock(s);
  bool footerInk = false;
  for (int y = 0; y < mm::H; y++)
    for (int x = 0; x < mm::W; x++) {
      bool before = noRepeater[y * (mm::W / 8) + x / 8] & (128 >> (x % 8)),
           after = canvas.getPixel(x, y);
      if (before != after) {
        assert(mm::LockRepeater.contains(x, y));
        footerInk = true;
      }
    }
  assert(footerInk);
  // Without a repeater line the wallpaper runs on to the bottom edge (no plate).
  s.lockRepeater = "";
  renderer.lock(s);
  {
    int ink = 0;
    for (int x = 0; x < mm::W; x++)
      ink += canvas.getPixel(x, mm::H - 2) != 0;
    assert(ink > 0);
  }
  // Every wallpaper keeps the lock anatomy: the art reaches the bottom edge below the card, the
  // summit row is opaque somewhere under the digits (the artworks keep their own summit column),
  // the date row above the clock is untouched by the mountain.
  {
    std::vector<uint8_t> base(canvas.getBuffer(), canvas.getBuffer() + 9600);
    for (int w = 0; w < mm_wallpaper::count; w++) {
      s.wallpaper = w;
      renderer.lock(s);
      {
        int ink = 0;
        for (int x = 0; x < mm::W; x++)
          for (int y = mm::LockMessage.y + mm::LockMessage.h + 1; y < mm::H; y++)
            ink += canvas.getPixel(x, y) != 0;
        assert(ink > 100);
      }
      for (int y = 0; y < mm::LockClockY; y++)
        for (int x = 0; x < mm::W; x++)
          assert(canvas.getPixel(x, y) == (((base[y * 30 + x / 8] >> (7 - x % 8)) & 1) != 0));
      uint8_t maskRow[30];
      {
        PackBitsReader mask(mm_wallpaper::list[w].mask);
        for (int y = 0; y <= mm_wallpaper::summitY + 3; y++)
          mask.read(maskRow, 30);
      }
      bool summit = false;
      for (int x = 20; x < mm::W - 20; x++)
        if (maskRow[x / 8] & (0x80 >> (x % 8)))
          summit = true;
      assert(summit);
    }
    s.wallpaper = 0;
    renderer.lock(s);
  }
  mm::RefreshTracker refresh;
  refresh.commit(canvas.getBuffer());
  s.receivedPreview = "Other message";
  renderer.lock(s);
  auto plan = refresh.plan(canvas.getBuffer(), false, 0);
  assert(plan.changed && !plan.full && plan.area.y >= mm::LockMessageY);
  std::cout << "PASS: structured reply scope/hops; no guessed incoming hop; timers/rollover; "
               "double SPACE/no long-press/bounce/FIFO; lock read queue; three-line "
               "preview/isolated RPT footer/no permanent footer; partial RX update\n";
}
