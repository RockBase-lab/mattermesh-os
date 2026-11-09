#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/renderer.h"
#include "ui/utf8.h"
#include "services/policy.h"
int main() {
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Components c(canvas);
  std::string source = "abcdefabcdefabcdefabcdefabcdefabcdefabcdef🙂👍";
  auto tail = c.inputTail(source, 70);
  assert(c.width(tail, mm::Font::ChatBody) <= 70 && tail.find("..") == std::string::npos &&
         tail.substr(tail.size() - 8) == "🙂👍");
  uint32_t cp;
  for (size_t at = 0; at < tail.size();)
    at = mm::utf8Next(tail, at, cp);
  auto lines = c.lines("🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂", 40, mm::Font::ChatBody);
  std::string joined;
  for (auto& line : lines) {
    assert(c.width(line, mm::Font::ChatBody) <= 40);
    joined += line;
  }
  assert(joined == "🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂");
  const int mv[] = {4200, 4100, 4000, 3900, 3800, 3700, 3600, 3500, 3400, 3300,
                    4150, 4050, 3950, 3850, 3750, 3650, 3550, 3450, 3350};
  const int pct[] = {100, 100, 92, 77, 62, 46, 31, 15, 0, 0,
                     100, 100, 85, 69, 54, 38, 23, 8,  0}; // 100 % to 4.05 V, 0 % at 3.40 V
  for (int i = 0; i < 19; i++)
    assert(device::batteryPercent(0, true, mv[i], false) == pct[i]);
  assert(device::batteryPercent(100, true, 3900, true) == 100);
  assert(device::batteryPercent(0, false, 3200, false) == 0 &&
         device::batteryPercent(0, false, 4300, false) == 100 &&
         device::batteryPercent(0, false, -1, false) == -1);
  mm::State s;
  s.page = mm::Page::Chat;
  s.chatName = "Italia";
  s.input = source;
  s.emojiPicker = true;
  s.messages.push_back({"River", "10:18", "Ciao 🙂👍", "2 hop | it", false});
  mm::Renderer r(canvas);
  r.draw(s);
  for (auto label : {"Chat", "Contacts", "Map", "Weather", "Menu"})
    assert(c.width(label, mm::Font::NavLabel) <= mm::MenuHomeTabW - 2);
  canvas.fillScreen(0);
  mm::Status status;
  status.charging = false;
  c.statusBar(status);
  unsigned char before[9600];
  std::memcpy(before, canvas.getBuffer(), sizeof(before));
  status.charging = true;
  c.statusBar(status);
  assert(std::memcmp(before, canvas.getBuffer(), sizeof(before)) != 0);
  assert(std::memcmp(before + mm::W / 8 * mm::StatusH, canvas.getBuffer() + mm::W / 8 * mm::StatusH,
                     sizeof(before) - mm::W / 8 * mm::StatusH) == 0);
  std::cout << "PASS: UTF-8 emoji wrap, scrolling tail/caret width, 19 requested battery reference "
               "values, charging marker, Home labels and chat rendering\n";
}
