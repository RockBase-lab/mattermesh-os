#include <cassert>
#include <iostream>
#include "services/touch_pipeline.h"
int main() {
  using namespace device;
  TouchPipeline p;
  TouchAction a;
  p.feed(true, 20, 30, 100, 1, 3);
  p.feed(false, 0, 0, 160, 1);
  p.feed(false, 0, 0, 183, 1);
  assert(!p.pop(a));
  p.feed(false, 0, 0, 184, 1);
  assert(p.pop(a) && a.kind == TouchActionKind::Tap && a.x == 20 && a.y == 30 && a.target == 3 &&
         a.releasedAt == 160);
  p.feed(true, 50, 50, 200, 1, 4);
  p.feed(false, 0, 0, 205, 1);
  p.feed(true, 51, 50, 210, 1);
  p.feed(false, 0, 0, 220, 1);
  p.feed(false, 0, 0, 244, 1);
  assert(p.pop(a) && a.kind == TouchActionKind::Tap && !p.pop(a));
  p.feed(true, 20, 80, 300, 1, 4);
  p.feed(true, 20, 120, 350, 1);
  p.feed(false, 0, 0, 400, 1);
  p.feed(false, 0, 0, 424, 1);
  assert(p.pop(a) && a.kind == TouchActionKind::Swipe && a.dy == 40);
  p.feed(true, 20, 80, 500, 1, 4);
  p.feed(true, 20, 80, 1200, 1);
  assert(p.pop(a) && a.kind == TouchActionKind::Hold && a.target == 4);
  p.feed(false, 0, 0, 1210, 1);
  p.feed(false, 0, 0, 1234, 1);
  assert(!p.pop(a));
  // Mostly horizontal move: Drag with dx/dy (map panning); a short wiggle is still cancelled.
  {
    TouchPipeline q;
    q.feed(true, 200, 100, 0, 1);
    q.feed(true, 120, 110, 50, 1);
    q.feed(false, 0, 0, 60, 1);
    q.feed(false, 0, 0, 84, 1);
    assert(q.pop(a) && a.kind == TouchActionKind::Drag && a.dx == -80 && a.dy == 10);
    q.feed(true, 100, 100, 200, 1);
    q.feed(true, 120, 100, 250, 1);
    q.feed(false, 0, 0, 260, 1);
    q.feed(false, 0, 0, 284, 1);
    assert(!q.pop(a) && q.cancelled == 1);
    q.feed(true, 100, 100, 300, 1);
    q.feed(true, 140, 160, 350, 1);
    q.feed(false, 0, 0, 360, 1);
    q.feed(false, 0, 0, 384, 1);
    assert(q.pop(a) && a.kind == TouchActionKind::Swipe && a.dx == 40 && a.dy == 60);
  }
  p.feed(true, 20, 80, 1300, 1, 4);
  p.feed(true, 20, 80, 1400, 2);
  p.feed(false, 0, 0, 1410, 2);
  p.feed(false, 0, 0, 1434, 2);
  assert(!p.pop(a) && p.cancelled == 1);
  p.feed(true, 20, 80, 1500, 2, 4, "", false);
  p.feed(true, 20, 80, 1600, 2, 4, "", true);
  p.feed(false, 0, 0, 1610, 2);
  p.feed(false, 0, 0, 1634, 2);
  assert(!p.pop(a));
  // Several complete taps captured while a panel refresh blocks dispatch.
  for (int i = 0; i < 9; i++) {
    uint32_t t = 2000 + i * 100;
    p.feed(true, 20 + i, 30, t, 2, -1, "full-key");
    p.feed(false, 0, 0, t + 30, 2);
    p.feed(false, 0, 0, t + 54, 2);
  }
  assert(p.size() == 8 && p.dropped == 1);
  for (int i = 0; i < 8; i++)
    assert(p.pop(a) && a.x == 20 + i && a.key == "full-key");
  assert(!p.pop(a));
  p.reset();
  p.feed(true, 1, 1, 0xfffffff0u, 3);
  p.feed(false, 0, 0, 0xfffffff8u, 3);
  p.feed(false, 0, 0, 0x10, 3);
  assert(p.pop(a));
  // Dragging horizontally across repeater rows must not activate the key from
  // the initial contact, nor fall through to a tap: it becomes a keyless Drag.
  p.reset();
  p.feed(true, 20, 80, 100, 4, 2, "original-repeater");
  p.feed(true, 70, 82, 150, 4);
  p.feed(false, 0, 0, 180, 4);
  p.feed(false, 0, 0, 204, 4);
  assert(p.pop(a) && a.kind == TouchActionKind::Drag && a.key.empty() && a.target == -1 &&
         !p.pop(a)); // ignored outside Maps
  p.reset();
  p.feed(true, 20, 80, 300, 4, 2);
  p.feed(true, 20, 100, 350, 4);
  p.feed(true, 20, 80, 380, 4);
  p.feed(false, 0, 0, 400, 4);
  p.feed(false, 0, 0, 424, 4);
  assert(!p.pop(a));
  p.reset();
  p.feed(true, 20, 80, 500, 4, 2);
  p.feed(true, 25, 130, 550, 4);
  p.feed(false, 0, 0, 570, 4);
  p.feed(false, 0, 0, 594, 4);
  assert(p.pop(a) && a.kind == TouchActionKind::Swipe && a.dy == 50);
  std::cout << "PASS: 24ms release/bounce/one-shot hold/swipe/scene guard, busy-frame FIFO/full "
               "identity/overflow/wrap/reset\n";
}
