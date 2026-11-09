// Host test of the MMQIT002 engine + blob prefetch: model validation, arena guards,
// cancellation, blob failure, two-core parity and streamed-vs-direct parity.
#include "offline_it/asr2.h"
#include "offline_it/blob_stream.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace mm_asr;
struct Buf {
  const uint8_t* d;
  size_t n;
};
static bool readBuf(void* c, uint32_t o, void* dst, uint32_t n) {
  auto* b = static_cast<Buf*>(c);
  if (size_t(o) + n > b->n)
    return false;
  memcpy(dst, b->d + o, n);
  return true;
}
static Source sourceOf(Buf& b) {
  Source s;
  s.context = &b;
  s.read = readBuf;
  return s;
}
static void set32(std::vector<uint8_t>& v, size_t at, uint32_t x) {
  memcpy(v.data() + at, &x, 4);
}
static uint32_t get32(const std::vector<uint8_t>& v, size_t at) {
  uint32_t x;
  memcpy(&x, v.data() + at, 4);
  return x;
}
static void reseal(std::vector<uint8_t>& v) {
  uint32_t vo = get32(v, 8 + 13 * 4), vb = get32(v, 8 + 14 * 4), bs = (vo + vb + 15) & ~15u;
  set32(v, 80, crc32(v.data() + 128, bs - 128));
  set32(v, 124, crc32(v.data(), 124));
}
static Error openFrom(std::vector<uint8_t>& v, Model& m) {
  Buf b{v.data(), v.size()};
  return open(m, sourceOf(b));
}
// direct blobs from memory
struct Direct {
  const uint8_t* d;
  size_t n;
  unsigned served = 0;
  std::vector<uint8_t> staging;
};
static const uint8_t* directBlob(void* c, unsigned, const Op& o) {
  auto* x = static_cast<Direct*>(c);
  x->served++;
  if (size_t(o.blobOffset) + o.storedBytes > x->n)
    return nullptr;
  if (!packed(o))
    return x->d + o.blobOffset;
  if (x->staging.size() < o.blobBytes)
    x->staging.resize(o.blobBytes);
  memcpy(x->staging.data() + (o.blobBytes - o.storedBytes), x->d + o.blobOffset, o.storedBytes);
  unpackBlob(o, x->staging.data());
  return x->staging.data();
}
// synchronous "SD" platform for BlobStream with failure injection and CRC checks
struct FakeSd {
  const uint8_t* d;
  size_t n;
  int failAt = -1;
  unsigned starts = 0, waits = 0, residentBelow = 0;
  bool pendingOk = false;
  int pendingIndex = -1;
  std::vector<unsigned> order;
};
static const uint8_t* fakeResident(void* c, const Op& o) {
  auto* x = static_cast<FakeSd*>(c);
  return !packed(o) && o.blobOffset + o.storedBytes <= x->residentBelow ? x->d + o.blobOffset
                                                                        : nullptr;
}
static bool fakeStart(void* c, unsigned i, const Op& o, uint8_t* dst) {
  auto* x = static_cast<FakeSd*>(c);
  x->starts++;
  x->order.push_back(i);
  if (int(i) == x->failAt)
    return false;
  uint8_t* tail = dst + (o.blobBytes - o.storedBytes);
  memcpy(tail, x->d + o.blobOffset, o.storedBytes);
  x->pendingOk = crc32(tail, o.storedBytes) == o.crc;
  if (x->pendingOk)
    unpackBlob(o, dst);
  x->pendingIndex = int(i);
  return true;
}
static bool fakeWait(void* c, unsigned i) {
  auto* x = static_cast<FakeSd*>(c);
  x->waits++;
  return x->pendingIndex == int(i) && x->pendingOk;
}
struct Threads {
  std::thread t;
};
static bool forkT(void* c, void (*job)(void*), void* arg) {
  static_cast<Threads*>(c)->t = std::thread(job, arg);
  return true;
}
static void joinT(void* c) {
  auto* t = static_cast<Threads*>(c);
  if (t->t.joinable())
    t->t.join();
}
int main(int argc, char** argv) {
  assert(argc == 2 && kernelSelfTest());
  std::ifstream f(argv[1], std::ios::binary);
  std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), {});
  assert(file.size() > 10000000);
  Model model;
  assert(openFrom(file, model) == Ok && model.dtype == 1 && model.ops > 100 &&
         model.classes == 39 && model.blank == 38);
  // every op record carries the CRC of its blob
  for (unsigned i = 0; i < model.ops; i++) {
    const Op& o = model.op[i];
    assert(crc32(file.data() + o.blobOffset, o.storedBytes) == o.crc);
  }
  {
    auto bad = file;
    bad[5] ^= 1;
    assert(openFrom(bad, model) != Ok);
  } // magic
  {
    auto bad = file;
    bad[100] ^= 1;
    assert(openFrom(bad, model) != Ok);
  } // header CRC
  {
    auto bad = file;
    bad[128 + 64 * 3 + 8] ^= 1;
    assert(openFrom(bad, model) != Ok);
  } // op table CRC (co field)
  {
    auto bad = file;
    bad[128 + 64 * 3 + 8] ^= 1;
    reseal(bad);
    assert(openFrom(bad, model) != Ok);
  } // resealed but inconsistent channels
  {
    auto bad = file;
    set32(bad, 128 + 64 * 5, 7);
    reseal(bad);
    assert(openFrom(bad, model) != Ok);
  } // unknown kind
  {
    auto bad = file;
    set32(bad, 128 + 64 * 5 + 28, 0x7ffffff0u);
    reseal(bad);
    assert(openFrom(bad, model) != Ok);
  } // blob offset out of file
  {
    auto bad = file;
    uint32_t vo = get32(bad, 8 + 13 * 4);
    bad[vo + 3] ^= 1;
    assert(openFrom(bad, model) != Ok);
  } // vocabulary covered by table CRC
  {
    std::vector<uint8_t> small(file.begin(), file.begin() + 127);
    Model m;
    assert(openFrom(small, m) != Ok);
  }
  assert(openFrom(file, model) == Ok);
  Model none;
  assert(workspaceBytes(none, 16000) == 0 && workspaceBytes(model, 0) == 0 &&
         workspaceBytes(model, 399) == 0 && workspaceBytes(model, 160001) == 0);
  // synthetic speech-like signal (two tones with a slow envelope) so the network emits something
  const size_t samples = 16000 * 4;
  std::vector<int16_t> pcm(samples);
  for (size_t i = 0; i < samples; i++) {
    double t = i / 16000.0;
    double env = 0.5 + 0.5 * std::sin(2 * M_PI * 3 * t);
    pcm[i] = int16_t(6000 * env *
                     (std::sin(2 * M_PI * 180 * t) + 0.6 * std::sin(2 * M_PI * 950 * t) +
                      0.3 * std::sin(2 * M_PI * 2400 * t)));
  }
  size_t bytes = workspaceBytes(model, samples);
  assert(bytes > 0 && bytes < 4u * 1024 * 1024);
  std::vector<uint8_t> guarded(bytes + 66, 0xab);
  alignas(16) static int8_t scratch[ScratchBytes + 32];
  memset(scratch, 0xab, sizeof(scratch));
  Direct direct{file.data(), file.size()};
  Hooks hooks;
  hooks.context = &direct;
  hooks.blob = directBlob;
  hooks.quantScratch = scratch + 16;
  char text[512];
  Stats stats;
  std::vector<float> logits(size_t(workspaceBytes(model, samples)) / 4, 0.f);
  unsigned frames = ((samples / 160 + 1 + 15) / 16 * 16 - 1) / 2 + 1;
  std::vector<float> ref(size_t(frames) * model.classes), two(ref.size()), streamed(ref.size());
  assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                    stats, hooks, ref.data(), ref.size()) == Ok);
  assert(stats.opsDone == model.ops && stats.frames == frames && direct.served == model.ops &&
         stats.workspaceBytes == bytes);
  for (int i = 0; i < 33; i++)
    assert(guarded[i] == 0xab && guarded[guarded.size() - 1 - i] == 0xab);
  for (int i = 0; i < 16; i++)
    assert(uint8_t(scratch[i]) == 0xab && uint8_t(scratch[sizeof(scratch) - 1 - i]) == 0xab);
  const std::string directText = text;
  std::cout << "direct: '" << directText << "' macs " << stats.macs << " arena " << bytes << "\n";
  assert(!directText.empty());
  // arena / scratch guards
  assert(transcribe(model, pcm.data(), samples, nullptr, 0, text, sizeof(text), stats, hooks) ==
         NoMemory);
  assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes - 1, text, sizeof(text),
                    stats, hooks) == NoMemory);
  {
    Hooks h = hooks;
    h.quantScratch++;
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == NoMemory);
  }
  {
    Hooks h = hooks;
    h.quantScratch = nullptr;
    size_t internal = bytes + ScratchBytes;
    std::vector<uint8_t> big(internal + 66, 0xab);
    assert(transcribe(model, pcm.data(), samples, big.data() + 33, internal - 1, text, sizeof(text),
                      stats, h) == NoMemory);
    assert(transcribe(model, pcm.data(), samples, big.data() + 33, internal, text, sizeof(text),
                      stats, h) == Ok);
    for (int i = 0; i < 33; i++)
      assert(big[i] == 0xab && big[big.size() - 1 - i] == 0xab);
  }
  assert(transcribe(model, pcm.data(), 399, guarded.data() + 33, bytes, text, sizeof(text), stats,
                    hooks) == InvalidAudio);
  assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, 1, stats,
                    hooks) == OutputTooSmall);
  // cancellation during features and during operators
  {
    Hooks h = hooks;
    h.service = [](void*, unsigned) { return false; };
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == Cancelled &&
           !text[0]);
  }
  {
    Hooks h = hooks;
    h.service = [](void*, unsigned p) { return p < 40; };
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == Cancelled &&
           !text[0] && stats.opsDone > 0 && stats.opsDone < model.ops);
  }
  // blob failure is reported, not ignored
  {
    Hooks h = hooks;
    h.blob = [](void*, unsigned i, const Op&) -> const uint8_t* {
      static Direct* d = nullptr;
      (void)d;
      return i == 50 ? nullptr : reinterpret_cast<const uint8_t*>(1);
    };
    Direct d2{file.data(), file.size()};
    h.context = &d2;
    h.blob = [](void* c, unsigned i, const Op& o) -> const uint8_t* {
      return i == 50 ? nullptr : directBlob(c, i, o);
    };
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == BlobUnavailable &&
           stats.opsDone == 50);
  }
  // two "cores" give the same logits
  {
    Threads th;
    alignas(16) static int8_t helper[ScratchBytes];
    Hooks h = hooks;
    h.fork = forkT;
    h.join = joinT;
    h.helperScratch = helper;
    Direct d2{file.data(), file.size()};
    struct Ctx {
      Threads* t;
      Direct* d;
    } ctx{&th, &d2};
    h.context = &ctx;
    h.blob = [](void* c, unsigned i, const Op& o) {
      return directBlob(static_cast<Ctx*>(c)->d, i, o);
    };
    h.fork = [](void* c, void (*job)(void*), void* arg) {
      return forkT(static_cast<Ctx*>(c)->t, job, arg);
    };
    h.join = [](void* c) { joinT(static_cast<Ctx*>(c)->t); };
    char text2[512];
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text2, sizeof(text2),
                      stats, h, two.data(), two.size()) == Ok);
    assert(directText == text2);
    float worst = 0;
    for (size_t i = 0; i < ref.size(); i++)
      worst = std::max(worst, std::fabs(ref[i] - two[i]));
    assert(worst < 1e-4f);
    std::cout << "two-core max |dlogit| " << worst << "\n";
  }
  // streamed through BlobStream: identical output, alternating buffers, one-ahead prefetch,
  // resident prefix
  for (unsigned residentBelow : {0u, 8u * 1024 * 1024}) {
    FakeSd sd{file.data(), file.size()};
    sd.residentBelow = residentBelow;
    BlobStream stream;
    BlobStream::Platform p;
    p.context = &sd;
    p.resident = fakeResident;
    p.start = fakeStart;
    p.wait = fakeWait;
    std::vector<uint8_t> a(model.maxBlob + 32, 0xcd), b(model.maxBlob + 32, 0xcd);
    stream.begin(model, p, a.data() + 16, b.data() + 16, model.maxBlob);
    Hooks h = hooks;
    h.context = &stream;
    h.blob = [](void* c, unsigned i, const Op& o) {
      return static_cast<BlobStream*>(c)->blob(i, o);
    };
    char text3[512];
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text3, sizeof(text3),
                      stats, h, streamed.data(), streamed.size()) == Ok);
    stream.finish();
    assert(directText == text3 && !memcmp(ref.data(), streamed.data(), ref.size() * sizeof(float)));
    for (int i = 0; i < 16; i++)
      assert(a[i] == 0xcd && a[a.size() - 1 - i] == 0xcd && b[i] == 0xcd &&
             b[b.size() - 1 - i] == 0xcd);
    unsigned residentOps = 0;
    for (unsigned i = 0; i < model.ops; i++)
      if (fakeResident(&sd, model.op[i]))
        residentOps++;
    assert(stream.residentHits == residentOps && stream.reads == model.ops - residentOps &&
           stream.prefetchHits + 1 >= stream.reads - residentOps * 0 && sd.starts == stream.reads);
    for (size_t i = 1; i < sd.order.size(); i++)
      assert(sd.order[i] > sd.order[i - 1]); // strictly increasing reads
    std::cout << "streamed (resident below " << residentBelow << "): resident "
              << stream.residentHits << " reads " << stream.reads << " prefetch hits "
              << stream.prefetchHits << "\n";
    if (!residentBelow)
      assert(stream.prefetchHits == stream.reads - 1); // only the first read is synchronous
  }
  {
    FakeSd sd{file.data(), file.size()};
    sd.failAt = 70;
    BlobStream stream;
    BlobStream::Platform p;
    p.context = &sd;
    p.start = fakeStart;
    p.wait = fakeWait;
    std::vector<uint8_t> a(model.maxBlob), b(model.maxBlob);
    stream.begin(model, p, a.data(), b.data(), model.maxBlob);
    Hooks h = hooks;
    h.context = &stream;
    h.blob = [](void* c, unsigned i, const Op& o) {
      return static_cast<BlobStream*>(c)->blob(i, o);
    };
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == BlobUnavailable &&
           stats.opsDone <= 70);
    stream.finish();
    assert(stream.failed);
  }
  {
    FakeSd sd{file.data(), file.size()};
    BlobStream stream;
    BlobStream::Platform p;
    p.context = &sd;
    p.start = fakeStart;
    p.wait = fakeWait;
    std::vector<uint8_t> a(model.maxBlob - 1), b(model.maxBlob - 1);
    stream.begin(model, p, a.data(), b.data(),
                 model.maxBlob - 1); // buffers too small for the largest blob
    Hooks h = hooks;
    h.context = &stream;
    h.blob = [](void* c, unsigned i, const Op& o) {
      return static_cast<BlobStream*>(c)->blob(i, o);
    };
    assert(transcribe(model, pcm.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, h) == BlobUnavailable);
    stream.finish();
  }
  // silence yields an empty hypothesis
  {
    std::vector<int16_t> quiet(samples, 0);
    assert(transcribe(model, quiet.data(), samples, guarded.data() + 33, bytes, text, sizeof(text),
                      stats, hooks) == Ok &&
           !text[0]);
  }
  std::cout << "PASS: MMQIT002 validation, guards, cancellation, blob failure, two-core and "
               "streamed parity\n";
}
