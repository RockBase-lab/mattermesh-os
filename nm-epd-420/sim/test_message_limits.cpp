#include <cassert>
#include <iostream>
#include "mesh/message_limits.h"
#include "services/dictation_policy.h"
#include "services/quick_reply.h"
int main() {
  using namespace matter;
  using namespace device;
  assert(messageBodyLimit(true, 31) == 160);
  assert(messageBodyLimit(false, std::string("MatterMesh_Ridge").size()) == 142);
  for (auto name : {std::string(), std::string("x"), std::string("MatterMesh_Ridge"),
                    std::string(31, 'x'), std::string(u8"Città")}) {
    size_t limit = messageBodyLimit(false, name.size());
    assert(name.size() + 2 + limit == MESSAGE_MAX_TEXT_BYTES);
    bool trimmed = false;
    auto body = appendDictation("", std::string(limit, 'x'), limit, trimmed);
    assert(body.size() == limit && !trimmed);
    assert(appendDictation("", std::string(limit + 1, 'x'), limit, trimmed).size() == limit &&
           trimmed);
  }
  constexpr auto group = messageBodyLimit(false, 16);
  bool trimmed;
  auto prefix = std::string(group - 3, 'x');
  assert(appendDictation(prefix, u8"è", group, trimmed) == prefix + u8" è" && !trimmed);
  assert(appendDictation(std::string(group - 2, 'x'), u8"è", group, trimmed) ==
             std::string(group - 2, 'x') &&
         trimmed);
  assert(appendDictation(std::string(group - 5, 'x'), u8"🙂", group, trimmed).size() == group &&
         !trimmed);
  assert(appendDictation(std::string(group - 4, 'x'), u8"🙂", group, trimmed) ==
             std::string(group - 4, 'x') &&
         trimmed);
  assert(appendDictation(std::string(150, 'x'), "more", group, trimmed) == std::string(150, 'x') &&
         trimmed);
  assert(appendDictation("saved", std::string(170, 'a'), group, trimmed).size() == group &&
         trimmed);
  ReplyOriginal original;
  original.chat = 1;
  original.token = 1;
  original.sender = u8"André";
  for (size_t limit : {group, MESSAGE_MAX_TEXT_BYTES}) {
    QuickReply reply;
    assert(reply.open(original, limit));
    while (reply.text.size() < limit)
      reply.append('x');
    auto text = reply.text;
    reply.append('z');
    assert(reply.text == text && reply.error == messageLimitError(limit));
    int calls = 0;
    assert(reply.send([&](int, const std::string&, const std::string& body, std::string&) {
      calls++;
      return body.size() == limit;
    }));
    assert(calls == 1);
    reply.close();
    reply.open(original, limit);
    reply.text = std::string(limit + 1, 'x');
    assert(!reply.send([&](int, const std::string&, const std::string&, std::string&) {
      calls++;
      return true;
    }));
    assert(calls == 1 && reply.text.size() == limit + 1);
    reply.erase();
    assert(reply.text.size() == limit);
  }
  std::cout
      << "PASS: MeshCore 160-byte envelope, dynamic sender prefix, channel142/DM160 boundaries, "
         "UTF-8 accents/emoji, preserved drafts and Quick Reply rejection before send\n";
}
