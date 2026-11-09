#include <cassert>
#include <cstring>
#include <iostream>
#include "ui/components.h"
int main() {
  GFXcanvas1 frame(mm::W, mm::H);
  mm::Components c(frame);
  for (auto font : {mm::Font::ChatBody, mm::Font::ChatMeta}) {
    frame.fillScreen(mm::BG);
    c.text(10, 10, "°", font);
    int r = std::max(2, c.ascent(font) / 5);
    assert(frame.getPixel(10 + r, 10));
    assert(!frame.getPixel(10 + r, 10 + r));
    std::vector<uint8_t> degree(frame.getBuffer(), frame.getBuffer() + 9600);
    frame.fillScreen(mm::BG);
    c.text(10, 10, "⁰", font);
    assert(!std::memcmp(degree.data(), frame.getBuffer(), 9600));
    assert(c.width("12°", font) == c.width("12⁰", font));
    assert(c.width("18℃", font) == c.width("18°C", font));
    auto lines = c.lines("Plateau Rosa 12⁰ windy today.", 80, font);
    for (auto& line : lines)
      assert(c.width(line, font) <= 80 || line == "°" || c.width("R", font) > 80);
  }
  frame.fillScreen(mm::FG);
  c.text(10, 10, "°", mm::Font::ChatBody, mm::BG);
  assert(!frame.getPixel(12, 10));
  assert(frame.getPixel(12, 12));
  std::cout << "PASS: degree/superscript-zero hollow glyph, shared width/wrapping, Celsius "
               "normalization, white-on-black\n";
}
