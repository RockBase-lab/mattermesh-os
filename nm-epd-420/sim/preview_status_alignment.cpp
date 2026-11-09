#include <fstream>
#include <filesystem>
#include "ui/components.h"
int main() {
  std::filesystem::path dir = "screenshots/mattermesh/status-alignment";
  std::filesystem::create_directories(dir);
  GFXcanvas1 frame(mm::W, mm::H);
  mm::Components c(frame);
  mm::Status s;
  s.clock = "10:24";
  s.unread = 1;
  s.gps = s.wifi = s.bt = true;
  std::ofstream json(dir / "geometry.json");
  json << "[";
  bool first = true;
  for (int level : {100, 99, 9, 0, -1}) {
    s.battery = level;
    c.statusBar(s);
    auto g = c.statusLayout(s);
    if (!first)
      json << ",";
    first = false;
    json << "{\"percent\":" << level << ",\"battery_x\":" << g.batteryX
         << ",\"percent_x\":" << g.percentX << ",\"percent_width\":" << g.percentWidth
         << ",\"icons_right\":" << g.iconsRight << "}";
    std::ofstream f(dir / (std::to_string(level) + ".pbm"), std::ios::binary);
    f << "P4\n" << mm::W << " " << mm::StatusH << "\n";
    f.write((char*)frame.getBuffer(), mm::W / 8 * mm::StatusH);
  }
  json << "]\n";
}
