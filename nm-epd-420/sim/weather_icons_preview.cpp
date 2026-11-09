#include <fstream>
#include <filesystem>
#include "ui/weather_icons.h"
int main() {
  std::filesystem::create_directories("assets/icons/weather");
  const char* names[] = {"clear", "partly_cloudy", "cloudy", "fog",    "drizzle",
                         "rain",  "snow",          "storm",  "unknown"};
  for (int i = 0; i < 9; i++)
    for (int size : {24, 64}) {
      GFXcanvas1 c(size, size);
      mm::drawWeatherGlyph(c, mm::WeatherGlyph(i), 0, 0, size);
      std::ofstream f(std::string("assets/icons/weather/weather_") + names[i] + "_" +
                          std::to_string(size) + ".pbm",
                      std::ios::binary);
      f << "P4\n" << size << " " << size << "\n";
      f.write((char*)c.getBuffer(), ((size + 7) / 8) * size);
    }
}
