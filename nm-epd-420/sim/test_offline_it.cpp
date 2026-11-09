#include "offline_it/quartznet.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
static void set32(uint8_t* p, uint32_t n) {
  memcpy(p, &n, 4);
}
static void checksums(std::vector<uint8_t>& b) {
  set32(b.data() + 36, mm_it::crc32(b.data() + 64, b.size() - 64));
  set32(b.data() + 60, mm_it::crc32(b.data(), 60));
}
int main(int argc, char** argv) {
  assert(mm_it::kernelSelfTest());
  assert(argc == 2);
  std::ifstream f(argv[1], std::ios::binary);
  std::vector<uint8_t> blob((std::istreambuf_iterator<char>(f)), {});
  assert(blob.size() > 6000000);
  mm_it::Model model;
  assert(mm_it::open(model, blob.data(), blob.size()) && model.dtype == 1);
  assert(!mm_it::open(model, blob.data(), 63));
  assert(!mm_it::open(model, blob.data() + 1, blob.size() - 1));
  {
    auto bad = blob;
    bad.back() ^= 1;
    assert(!mm_it::open(model, bad.data(), bad.size()));
  }
  {
    auto bad = blob;
    set32(bad.data() + 16, 65);
    checksums(bad);
    assert(!mm_it::open(model, bad.data(), bad.size()));
  }
  {
    auto bad = blob;
    set32(bad.data() + 64 + 28, 0xffffff00);
    checksums(bad);
    assert(!mm_it::open(model, bad.data(), bad.size()));
  }
  {
    auto bad = blob;
    set32(bad.data() + 64 + 64 + 4, 0xffffffff);
    checksums(bad);
    assert(!mm_it::open(model, bad.data(), bad.size()));
  }
  {
    auto bad = blob;
    uint32_t offset;
    memcpy(&offset, bad.data() + 64 + 36, 4);
    set32(bad.data() + offset, 0x7fc00000);
    checksums(bad);
    assert(!mm_it::open(model, bad.data(), bad.size()));
  }
  mm_it::Hooks loading;
  loading.service = [](void*, unsigned) { return false; };
  assert(!mm_it::open(model, blob.data(), blob.size(), true, loading));
  assert(mm_it::open(model, blob.data(), blob.size()));
  assert(mm_it::workspaceBytes(mm_it::Model{}, 16000) == 0 &&
         mm_it::workspaceBytes(model, 0) == 0 && mm_it::workspaceBytes(model, 160001) == 0);
  assert(mm_it::workspaceBytes(0) == 0 && mm_it::workspaceBytes(399) == 0 &&
         mm_it::workspaceBytes(160001) == 0);
  for (size_t samples : {size_t(400), size_t(401), size_t(160000)}) {
    size_t bytes = mm_it::workspaceBytes(model, samples);
    assert(bytes > 0 && bytes <= mm_it::workspaceBytes(samples));
    std::vector<uint8_t> guarded(bytes + 66, 0xab);
    std::vector<int16_t> pcm(samples, 0);
    char text[512];
    mm_it::Stats stats;
    assert(mm_it::transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text,
                             sizeof(text), stats) == mm_it::Ok);
    for (int i = 0; i < 33; i++)
      assert(guarded[i] == 0xab && guarded[guarded.size() - 1 - i] == 0xab);
    assert(!text[0] && stats.opsDone == 63 && stats.workspaceBytes == bytes);
    assert(mm_it::transcribe(model, pcm.data(), samples, nullptr, 0, text, sizeof(text), stats) ==
           mm_it::NoMemory);
    assert(mm_it::transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes - 1, text,
                             sizeof(text), stats) == mm_it::NoMemory);
    size_t reduced = mm_it::workspaceBytes(model, samples, true);
    assert(reduced + mm_it::ScratchBytes == bytes);
    std::vector<uint8_t> small(reduced + 66, 0xab);
    alignas(16) uint8_t scratch[mm_it::ScratchBytes + 32];
    memset(scratch, 0xab, sizeof(scratch));
    mm_it::Hooks external;
    external.quantScratch = reinterpret_cast<int8_t*>(scratch + 16);
    assert(mm_it::transcribe(model, pcm.data(), samples, small.data() + 33, reduced, text,
                             sizeof(text), stats, external) == mm_it::Ok &&
           !text[0]);
    assert(stats.workspaceBytes == reduced && stats.opsDone == 63);
    for (int i = 0; i < 33; i++)
      assert(small[i] == 0xab && small[small.size() - 1 - i] == 0xab);
    for (int i = 0; i < 16; i++)
      assert(scratch[i] == 0xab && scratch[sizeof(scratch) - 1 - i] == 0xab);
    assert(mm_it::transcribe(model, pcm.data(), samples, small.data() + 33, reduced - 1, text,
                             sizeof(text), stats, external) == mm_it::NoMemory);
    assert(mm_it::transcribe(model, pcm.data(), samples, small.data() + 33, reduced, text,
                             sizeof(text), stats) == mm_it::NoMemory);
    external.quantScratch++;
    assert(mm_it::transcribe(model, pcm.data(), samples, small.data() + 33, reduced, text,
                             sizeof(text), stats, external) == mm_it::NoMemory);
    external.quantScratch--;
    unsigned visits = 0;
    external.context = &visits;
    external.service = [](void* context, unsigned p) {
      return p != 95 || ++*static_cast<unsigned*>(context) < 2;
    };
    assert(mm_it::transcribe(model, pcm.data(), samples, small.data() + 33, reduced, text,
                             sizeof(text), stats, external) == mm_it::Cancelled &&
           !text[0] && visits == 2);
    mm_it::Hooks hooks;
    hooks.service = [](void*, unsigned p) { return p < 45; };
    assert(mm_it::transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text,
                             sizeof(text), stats, hooks) == mm_it::Cancelled &&
           !text[0]);
    std::cout << "PASS: " << samples
              << " samples; arena/scratch guards, bounded allocation, cancellation including "
                 "single-tap kernel, silence; arena "
              << bytes << " / external " << reduced << " bytes\n";
  }
  std::cout << "PASS: CRC/header/offset/dimension/nonfinite/unaligned model rejection\n";
}
