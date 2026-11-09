#include <cassert>
#include <iostream>
#include <vector>
#include "ui/refresh.h"
// E-ink planner: page navigation is a partial update, the flashing full refresh is deferred to idle
// (maintenanceDue) and only forced on the first frame / marked transitions / hard caps; tiny
// partials do not count toward PartialLimit.
int main() {
  const int stride = (mm::W + 7) / 8, size = stride * mm::H;
  std::vector<uint8_t> a(size, 0), b(size, 0xff), c(size, 0);
  c[0] = 1;
  mm::RefreshTracker t;
  auto first = t.plan(a.data(), false, 0);
  assert(first.changed && first.full);
  t.commit(a.data(), first);
  assert(!t.maintenanceDue());
  auto nav = t.plan(b.data(), false, 0);
  assert(nav.changed && !nav.full && nav.area.w == mm::W && nav.area.h == mm::H &&
         mm::RefreshTracker::countsAsPartial(nav));
  t.commit(b.data(), nav);
  assert(!t.maintenanceDue()); // first big change: partial, no maintenance yet
  for (int i = 1; i < 5; i++) {
    auto n = t.plan(i % 2 ? a.data() : b.data(), false, i);
    assert(n.changed && !n.full);
    t.commit(i % 2 ? a.data() : b.data(), n);
    assert(!t.maintenanceDue());
  } // 5 screens: still within budget
  auto sixth = t.plan(b.data(), false, 5);
  assert(!sixth.full && !t.maintenanceDue());
  t.commit(b.data(), sixth); // six screens of partial area now committed
  auto seventh = t.plan(a.data(), false, 6);
  assert(!seventh.full && t.maintenanceDue());
  t.commit(a.data(), seventh); // budget spent: still partial, full deferred
  auto idle = t.plan(a.data(), true, 7);
  assert(idle.changed && idle.full);
  t.commit(a.data(), idle);
  assert(!t.maintenanceDue()); // idle maintenance clears it
  auto tiny = t.plan(c.data(), false, 0);
  assert(tiny.changed && !tiny.full &&
         !mm::RefreshTracker::countsAsPartial(tiny)); // one pixel: never counted as a partial
  t.commit(c.data(), tiny);
  auto same = t.plan(c.data(), false, 0);
  assert(!same.changed);
  auto cap = t.plan(a.data(), false, mm::PartialLimit * 3);
  assert(cap.full); // hard cap still forces a full refresh
  auto limit = t.plan(a.data(), false, mm::PartialLimit);
  assert(!limit.full && t.maintenanceDue()); // soft limit only marks maintenance
  std::cout << "PASS: e-ink refresh policy (navigation partial, deferred idle full, tiny partials "
               "uncounted, hard cap)"
            << std::endl;
  return 0;
}
