#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "ui/renderer.h"
#include "services/geo.h"
// Contacts page (Home tab, section 1): layout inside the LoRa panel, tabs in the header, rendering
// states, distance text.
int main() {
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer renderer(canvas);
  auto disjoint = [](mm::Rect a, mm::Rect b) {
    return a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
  };
  auto inPanel = [](mm::Rect r) {
    return r.x >= mm::LoRaPanel.x + 1 && r.x + r.w <= mm::LoRaPanel.x + mm::LoRaPanel.w - 1 &&
           r.y >= mm::LoRaPanel.y + 1 && r.y + r.h <= mm::LoRaPanel.y + mm::LoRaPanel.h - 1;
  };
  // Tabs sit in the header strip, end before the panel's rounded corner and do not overlap each
  // other or the back button.
  for (int i = 0; i < mm::ContactTabCount; i++) {
    auto t = mm::contactTabRect(i);
    assert(t.y >= mm::StatusH && t.y + t.h <= mm::Top + 1);
    assert(t.x + t.w <= mm::W - mm::Margin - mm::Radius);
    assert(t.x >= mm::BackButton.x + mm::BackButton.w);
    for (int j = 0; j < i; j++)
      assert(disjoint(t, mm::contactTabRect(j)));
  }
  assert(inPanel(mm::ContactSearch) && inPanel(mm::ContactPrevious) && inPanel(mm::ContactNext));
  for (int i = 0; i < mm::ContactRows; i++) {
    auto r = mm::contactRowRect(i);
    assert(inPanel(r));
    assert(disjoint(r, mm::ContactSearch) && disjoint(r, mm::ContactPrevious) &&
           disjoint(r, mm::ContactNext));
    for (int j = 0; j < i; j++)
      assert(disjoint(r, mm::contactRowRect(j)));
  }
  auto last = mm::contactRowRect(mm::ContactRows - 1);
  assert(last.y + last.h <= mm::ContactNoticeY && mm::ContactNoticeY + 12 <= mm::ContactPrevious.y);
  // Rendering: focused row inverted, other rows outlined, empty state and search state change the
  // frame.
  mm::State s;
  s.page = mm::Page::Mesh;
  s.radioReady = true;
  s.filteredNodeCount = 3;
  s.contactLive = 2;
  const char* names[] = {"River", "Base Camp RPT", "Archive"};
  for (int i = 0; i < 3; i++) {
    mm::Node n;
    n.name = names[i];
    n.key = std::string(64, 'a' + i);
    n.type = i == 1 ? 2 : 1;
    n.live = i < 2;
    n.age = 30;
    n.rssi = -80;
    n.snr = 6;
    n.hops = i;
    n.distance = i == 0 ? "1.2 km NE" : "";
    s.nodes.push_back(n);
  }
  auto frame = [&]() {
    return std::vector<uint8_t>(canvas.getBuffer(), canvas.getBuffer() + ((mm::W + 7) / 8) * mm::H);
  };
  renderer.draw(s);
  auto list = frame();
  auto r0 = mm::contactRowRect(0), r1 = mm::contactRowRect(1);
  assert(canvas.getPixel(r0.x + 3, r0.y + 3) &&
         !canvas.getPixel(r1.x + 3, r1.y + 3)); // row 0 focused (filled), row 1 outlined
  assert(!canvas.getPixel(mm::contactRowRect(3).x + 3,
                          mm::contactRowRect(3).y + 3)); // no fourth row drawn
  s.contactFocus = 1;
  renderer.draw(s);
  assert(!canvas.getPixel(r0.x + 3, r0.y + 3) && canvas.getPixel(r1.x + 3, r1.y + 3));
  s.contactFocus = 0;
  s.filter = 2;
  renderer.draw(s);
  auto rpt = frame();
  assert(rpt != list);
  s.filter = 0;
  s.contactSearching = true;
  s.contactQuery = "riv";
  renderer.draw(s);
  auto searching = frame();
  assert(searching != list);
  s.contactSearching = false;
  s.contactQuery = "";
  s.nodes.clear();
  s.filteredNodeCount = s.contactLive = 0;
  renderer.draw(s);
  auto empty = frame();
  assert(empty != list);
  for (int i = 0; i < mm::ContactRows; i++) {
    auto r = mm::contactRowRect(i);
    assert(!canvas.getPixel(r.x, r.y + 5));
  }
  s.radioReady = false;
  renderer.draw(s);
  assert(frame() != empty);
  // Distance text.
  assert(mm::formatDistance(850.4, "NE") == "850 m NE");
  assert(mm::formatDistance(2440, "N") == "2.4 km N");
  assert(mm::formatDistance(120400, "W") == "120 km W");
  double d = mm::geoDistanceMeters(44.0, 8.0, 45.0, 8.0);
  assert(d > 111000 && d < 111400);
  assert(!strcmp(mm::geoHeading(44.0, 8.0, 45.0, 8.0), "N"));
  assert(!strcmp(mm::geoHeading(44.0, 8.0, 44.0, 8.1), "E"));
  assert(!strcmp(mm::geoHeading(44.0, 8.0, 43.5, 7.5), "SW"));
  std::cout << "PASS: Contacts page layout (tabs in header, rows/search/pager inside the panel), "
               "focus/search/filter/empty/offline frames, distance text"
            << std::endl;
  return 0;
}
