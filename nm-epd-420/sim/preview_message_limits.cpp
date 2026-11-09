#include <filesystem>
#include <fstream>
#include "ui/renderer.h"
int main(int argc, char** argv) {
  std::filesystem::path dir = argc > 1 ? argv[1] : "screenshots/mattermesh/message-limits";
  std::filesystem::create_directories(dir);
  GFXcanvas1 canvas(mm::W, mm::H);
  mm::Renderer renderer(canvas);
  mm::State state;
  state.status.clock = "10:24";
  state.status.battery = 100;
  state.chatName = "Public";
  state.page = mm::Page::Chat;
  state.messageLimit = matter::messageBodyLimit(false, 16);
  state.messages.push_back({"River", "10:18", "Meet at the ridge?", "2 hop | it", false});
  auto save = [&](const char* name) {
    renderer.draw(state);
    std::ofstream f(dir / (std::string(name) + ".pbm"), std::ios::binary);
    f << "P4\n" << mm::W << " " << mm::H << "\n";
    f.write((char*)canvas.getBuffer(), ((mm::W + 7) / 8) * mm::H);
  };
  save("empty");
  state.input = std::string(95, 'x');
  save("threshold95-hidden");
  state.input += 'x';
  save("threshold96-visible");
  state.input = "The last characters remain visible while writing.";
  save("writing");
  state.input = std::string(138, 'x') + u8"🙂";
  save("channel-full-utf8");
  state.messageLimit = 160;
  state.chatName = "River";
  state.input = std::string(150, 'x') + " last text";
  save("direct-full");
  state.firstRepeater = "Received via: long repeater name";
  save("counter-with-rpt");
  state.page = mm::Page::Home;
  state.quickReply = true;
  state.quickReplyEditable = true;
  state.quickSender = "River";
  state.quickChannel = "Public";
  state.quickPreview = "Meet at the ridge?";
  state.quickInput = "@[River] " + std::string(133, 'x');
  state.quickMessageLimit = 142;
  renderer.lock(state);
  {
    std::ofstream f(dir / "quick-full.pbm", std::ios::binary);
    f << "P4\n" << mm::W << " " << mm::H << "\n";
    f.write((char*)canvas.getBuffer(), ((mm::W + 7) / 8) * mm::H);
  }
}
