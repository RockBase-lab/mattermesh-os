#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "services/map_math.h"
using namespace device;
// The running-sum tile conversion must match the brute-force 5x5 mean it replaced, bit for bit.
static void bruteConvert(const uint8_t* luma, const uint8_t* tone, uint8_t* bits) {
  MapDither dither;
  bool forced[MapTileSize];
  for (int y = 0; y < MapTileSize; y++) {
    for (int x = 0; x < MapTileSize; x++) {
      int sum = 0, count = 0;
      for (int dy = -2; dy <= 2; dy++) {
        int yy = y + dy;
        if (yy < 0 || yy >= MapTileSize)
          continue;
        for (int dx = -2; dx <= 2; dx++) {
          int xx = x + dx;
          if (xx < 0 || xx >= MapTileSize)
            continue;
          sum += luma[yy * MapTileSize + xx];
          count++;
        }
      }
      int l = luma[y * MapTileSize + x];
      forced[x] = l <= MapLineMax && l * count + MapLineDelta * count < sum;
    }
    dither.line(tone + y * MapTileSize, bits + y * MapTileStride, forced);
  }
}
int main() {
  // Projection: Breuil-Cervinia at z14 is tile 8539/5833 (slippy-map reference), round-trip within
  // 1e-9 deg.
  auto c = mapProject(45.934, 7.629, 14);
  assert(int(c.x / 256) == 8539 && int(c.y / 256) == 5833);
  double lat, lon;
  mapUnproject(c, 14, lat, lon);
  assert(fabs(lat - 45.934) < 1e-9 && fabs(lon - 7.629) < 1e-9);
  auto zero = mapProject(0, 0, 6);
  assert(fabs(zero.x - mapWorldSize(6) / 2) < 1e-9 && fabs(zero.y - mapWorldSize(6) / 2) < 1e-9);
  assert(mapProject(89.9, 0, 6).y > -1e-6 && mapProject(-89.9, 0, 6).y < mapWorldSize(6) + 1e-6);
  assert(fabs(mapMetersPerPixel(0, 0) - 156543.03392) < 1e-6 &&
         fabs(mapMetersPerPixel(60, 1) - 39135.76) < 0.01);
  // View 240x222 centred on a tile corner: 4 visible tiles first, then the 12-tile ring.
  MapTileRef t[64];
  MapCoord corner{1000.0 * 256, 1000.0 * 256};
  int n = mapVisibleTiles(corner, 12, 240, 222, 0, t, 64);
  assert(n == 4);
  for (int i = 0; i < n; i++) {
    assert(t[i].x == 999 || t[i].x == 1000);
    assert(t[i].screenX == -136 || t[i].screenX == 120);
    assert(t[i].screenY == -145 || t[i].screenY == 111);
  }
  n = mapVisibleTiles(corner, 12, 240, 222, 1, t, 64);
  assert(n == 16);
  for (int i = 4; i < 16; i++)
    assert(t[i].x < 998 || t[i].x > 1000 || t[i].y < 998 || t[i].y > 1000 || t[i].x == 998 ||
           t[i].x == 1001 || t[i].y == 998 || t[i].y == 1001);
  // Antimeridian wrap and polar clipping.
  n = mapVisibleTiles({10, 10}, 6, 240, 222, 0, t, 64);
  for (int i = 0; i < n; i++) {
    assert(t[i].y == 0);
    assert(t[i].x == 0 || t[i].x == 63);
  }
  assert(n == 2);
  assert(mapVisibleTiles(corner, 12, 240, 222, 1, t, 3) == 3);
  char path[48];
  mapTilePath(path, sizeof(path), 14, 8598, 5938);
  assert(std::string(path) == "/maps/osm1/14/8598/5938.mmt");
  // OSM tones: paper and white roads white, ink black, water a light texture, motorways dark.
  auto rgb = [](int r, int g, int b) {
    return uint16_t((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
  };
  assert(mapOsmTone(rgb(242, 239, 233)) == 255 && mapOsmTone(0xffff) == 255 && mapOsmTone(0) == 0);
  assert(mapOsmTone(rgb(170, 211, 223)) > 150 && mapOsmTone(rgb(170, 211, 223)) < 230);
  assert(mapOsmTone(rgb(232, 146, 162)) < 100);
  assert(mapLuma565(0xffff) == 254 || mapLuma565(0xffff) == 255);
  assert(mapLuma565(0) == 0);
  // Conversion: flat white -> blank, flat black -> solid, 1px dark line on white stays solid, mid
  // grey dithers. (luma drives the line detector, tone the fill; the same buffer serves both here)
  std::vector<uint8_t> l(65536, 255), bits(MapTileBytes);
  mapConvertTile(l.data(), l.data(), bits.data());
  for (auto b : bits)
    assert(b == 0);
  std::fill(l.begin(), l.end(), 0);
  mapConvertTile(l.data(), l.data(), bits.data());
  for (auto b : bits)
    assert(b == 0xff);
  std::fill(l.begin(), l.end(), 255);
  for (int x = 0; x < 256; x++)
    l[100 * 256 + x] = 160;
  {
    std::vector<uint8_t> t(65536, 255);
    mapConvertTile(l.data(), t.data(), bits.data());
  }
  for (int x = 0; x < 256; x++) {
    assert(mapBit(bits.data(), MapTileStride, x, 100));
    assert(!mapBit(bits.data(), MapTileStride, x, 99) &&
           !mapBit(bits.data(), MapTileStride, x, 101));
  }
  std::fill(l.begin(), l.end(), 60);
  mapConvertTile(l.data(), l.data(), bits.data());
  int black = 0;
  for (int y = 0; y < 256; y++)
    for (int x = 0; x < 256; x++)
      black += mapBit(bits.data(), MapTileStride, x, y);
  assert(black > 5000 && black < 60000);
  // Blit with clipping into a 240x222 view.
  std::vector<uint8_t> tile(MapTileBytes, 0), view((240 + 7) / 8 * 222, 0);
  tile[0] = 0x80;
  tile[MapTileBytes - 1] = 0x01;
  mapBlit(tile.data(), -10, -10, view.data(), 240, 222);
  for (auto b : view)
    assert(b == 0);
  mapBlit(tile.data(), 5, 7, view.data(), 240, 222);
  assert(mapBit(view.data(), 30, 5, 7));
  std::fill(view.begin(), view.end(), 0xff);
  mapBlit(tile.data(), -200, -200, view.data(), 240, 222);
  assert(mapBit(view.data(), 30, 55, 55) && !mapBit(view.data(), 30, 0, 0) &&
         mapBit(view.data(), 30, 56, 0));
  // Segment clipping to the map area.
  {
    double ax = -10, ay = 50, bx = 300, by = 50;
    assert(mapClipSegment(ax, ay, bx, by, 0, 0, 240, 222) && ax == 0 && bx == 239 && ay == 50 &&
           by == 50);
    ax = 10;
    ay = 10;
    bx = 20;
    by = 20;
    assert(mapClipSegment(ax, ay, bx, by, 0, 0, 240, 222) && ax == 10 && by == 20);
    ax = -5;
    ay = -5;
    bx = -1;
    by = 300;
    assert(!mapClipSegment(ax, ay, bx, by, 0, 0, 240, 222));
    ax = -100;
    ay = 111;
    bx = 120;
    by = -100;
    assert(mapClipSegment(ax, ay, bx, by, 0, 0, 240, 222) && ax >= 0 && ay >= 0 && bx >= 0 &&
           by >= 0 && ax < 240 && ay < 222);
  }
  // Fit: Zermatt-Chatillon fits around z10-11; a single point gets maxZoom; all points end up
  // inside the margin box.
  {
    double la[] = {46.02, 45.75}, lo[] = {7.75, 7.62}, cl, cn;
    int z = mapFitZoom(la, lo, 2, 240, 222, 20, 16, cl, cn);
    assert(z >= 9 && z <= 11 && fabs(cl - 45.885) < 0.01 && fabs(cn - 7.685) < 0.01);
    auto c = mapProject(cl, cn, z);
    for (int i = 0; i < 2; i++) {
      auto p = mapProject(la[i], lo[i], z);
      assert(fabs(p.x - c.x) <= 100 && fabs(p.y - c.y) <= 91);
    }
    assert(mapFitZoom(la, lo, 1, 240, 222, 20, 15, cl, cn) == 15 && fabs(cl - 46.02) < 1e-9);
    double far[] = {45.9, -33.9}, farLon[] = {7.6, 151.2};
    assert(mapFitZoom(far, farLon, 2, 240, 222, 20, 16, cl, cn) == MapMinZoom);
    assert(mapFitZoom(la, lo, 0, 240, 222, 20, 16, cl, cn) == -1);
  }
  {
    uint32_t seed = 12345;
    std::vector<uint8_t> l(65536), t(65536), a(MapTileBytes), b(MapTileBytes);
    for (int round = 0; round < 4; round++) {
      for (size_t k = 0; k < l.size(); k++) {
        seed = seed * 1103515245 + 12345;
        l[k] = uint8_t(round == 3 ? (k % 7 == 0 ? 20 : 230) : seed >> 16);
        t[k] = uint8_t(seed >> 8);
      }
      mapConvertTile(l.data(), t.data(), a.data());
      bruteConvert(l.data(), t.data(), b.data());
      assert(a == b);
    }
  }
  std::cout
      << "PASS: Web-Mercator projection/round-trip, tile ring order/wrap/clip, SD tile path, tone "
         "curve, line-preserving dither, clipped 1bpp blit, segment clipping, route fit zoom\n";
}
