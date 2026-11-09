#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include "offline_it/quartznet.h"
// Two-core split must be bit-identical to the single-core engine and must
// always join the helper, including when the caller cancels mid-operation.
struct Helper {
  std::thread thread;
  int forks = 0, joins = 0;
};
static bool fork(void* context, void (*task)(void*), void* arg) {
  auto* h = static_cast<Helper*>(context);
  h->forks++;
  h->thread = std::thread(task, arg);
  return true;
}
static void join(void* context) {
  auto* h = static_cast<Helper*>(context);
  h->joins++;
  h->thread.join();
}
struct Cancel {
  int calls = 0, limit = 0;
};
static bool cancelAfter(void* context, unsigned) {
  auto* c = static_cast<Cancel*>(context);
  return ++c->calls < c->limit;
}
int main(int argc, char** argv) {
  assert(argc == 2);
  FILE* f = fopen(argv[1], "rb");
  assert(f);
  std::vector<uint8_t> raw;
  uint8_t buffer[65536];
  size_t n;
  while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
    raw.insert(raw.end(), buffer, buffer + n);
  fclose(f);
  void* aligned = nullptr;
  assert(!posix_memalign(&aligned, 16, raw.size()));
  memcpy(aligned, raw.data(), raw.size());
  mm_it::Model model;
  assert(mm_it::open(model, aligned, raw.size()));
  // 6 s deterministic speech-like signal: two moving formants plus noise.
  const size_t samples = 6 * mm_it::SampleRate;
  std::vector<int16_t> pcm(samples);
  uint32_t seed = 12345;
  for (size_t i = 0; i < samples; i++) {
    double t = double(i) / mm_it::SampleRate;
    seed = seed * 1664525u + 1013904223u;
    double v = 3000 * std::sin(2 * M_PI * (300 + 200 * std::sin(3 * t)) * t) +
               2000 * std::sin(2 * M_PI * (1200 + 400 * std::sin(5 * t)) * t) +
               int(seed >> 20) % 800 - 400;
    pcm[i] = int16_t(v * (0.5 + 0.5 * std::sin(2 * M_PI * 1.5 * t)));
  }
  const size_t frames = samples / 160 / 2 + 1;
  auto run = [&](mm_it::Hooks hooks, std::vector<float>& logits, std::string& text) {
    std::vector<uint8_t> arena(mm_it::workspaceBytes(model, samples));
    char out[512];
    mm_it::Stats stats;
    logits.assign(frames * mm_it::Classes, 0);
    int code = mm_it::transcribe(model, pcm.data(), samples, arena.data(), arena.size(), out,
                                 sizeof(out), stats, hooks, logits.data(), logits.size());
    text = out;
    return code;
  };
  std::vector<float> single, dual;
  std::string a, b;
  assert(run({}, single, a) == mm_it::Ok);
  Helper helper;
  alignas(16) static int8_t helperScratch[mm_it::ScratchBytes];
  mm_it::Hooks hooks;
  hooks.context = &helper;
  hooks.fork = fork;
  hooks.join = join;
  hooks.helperScratch = helperScratch;
  assert(run(hooks, dual, b) == mm_it::Ok);
  assert(helper.forks > 100 && helper.forks == helper.joins);
  assert(a == b && !memcmp(single.data(), dual.data(), single.size() * sizeof(float)));
  // Without a helper strip only pointwise splits; still identical.
  Helper pwOnly;
  hooks.context = &pwOnly;
  hooks.helperScratch = nullptr;
  std::vector<float> partial;
  std::string c;
  assert(run(hooks, partial, c) == mm_it::Ok && pwOnly.forks == pwOnly.joins &&
         !memcmp(single.data(), partial.data(), single.size() * sizeof(float)));
  assert(pwOnly.forks < helper.forks);
  // Cancellation inside a split operation still joins every fork.
  struct Both {
    Helper helper;
    Cancel cancel;
  } both;
  both.cancel.limit = 500;
  mm_it::Hooks cancelled;
  cancelled.context = &both;
  cancelled.helperScratch = helperScratch;
  cancelled.fork = [](void* c, void (*t)(void*), void* arg) {
    return fork(&static_cast<Both*>(c)->helper, t, arg);
  };
  cancelled.join = [](void* c) { join(&static_cast<Both*>(c)->helper); };
  cancelled.service = [](void* c, unsigned p) {
    return cancelAfter(&static_cast<Both*>(c)->cancel, p);
  };
  std::vector<float> stopped;
  std::string d;
  assert(run(cancelled, stopped, d) == mm_it::Cancelled && both.helper.forks == both.helper.joins &&
         both.helper.forks > 0);
  free(aligned);
  std::cout << "PASS: two-core split bit-identical logits/text (" << helper.forks
            << " forks), pointwise-only split, cancellation joins helper; text '" << a << "'\n";
}
