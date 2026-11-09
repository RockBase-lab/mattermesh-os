#include <cassert>
#include <sstream>
#include <iostream>
#include "services/channel_catalog_policy.h"
#include "mesh/journal.h"
int main() {
  fs::FS fs;
  matter::Journal journal;
  journal.begin(fs);
  matter::Record r;
  r.chat = 0;
  r.token = 1;
  r.body = "Keep Public";
  assert(journal.append(r));
  r.chat = 1;
  r.token = 2;
  r.body = "Delete channel";
  assert(journal.append(r));
  r.chat = 2;
  r.token = 3;
  r.body = "Old kept channel";
  assert(journal.append(r));
  assert(journal.clearChat(2));
  r.token = 4;
  r.body = "Keep after clear";
  r.path = "flood:abcd,ef12";
  assert(journal.append(r));
  assert(journal.markRead(2, 4));
  r.mine = true;
  r.token = 5;
  r.body = "Keep own message";
  assert(journal.append(r));
  assert(journal.repeats(5, 2, "abcd,ef12"));
  assert(journal.update(5, "Sent"));
  int mapping[3] = {0, -1, 1};
  auto& bytes = fs.files["/mattermesh/messages.tsv"];
  std::stringstream in(*bytes);
  std::string out, line;
  while (std::getline(in, line)) {
    auto result = device::remapChatRow(line, mapping, 3);
    if (result == device::RemapRow::Keep)
      out += line + "\n";
  }
  *bytes = out;
  matter::Journal boot;
  boot.begin(fs);
  assert(boot.records.size() == 3 && boot.records[0].token == 1 && boot.records[1].token == 4 &&
         boot.records[2].token == 5);
  assert(boot.records[1].chat == 1 && boot.records[1].path == "flood:abcd,ef12");
  assert(boot.records[2].repeaters == 2 && boot.records[2].heard == "abcd,ef12" &&
         boot.records[2].status == "Sent");
  assert(boot.readTokens[1] == 4 && (boot.clearedChats & 2));
  line = "R2\ttoken\tx\tbad";
  assert(device::remapChatRow(line, mapping, 3) == device::RemapRow::Drop);
  line = "D1\t-1\t100\tCLEAR";
  assert(device::remapChatRow(line, mapping, 3) == device::RemapRow::Drop);
  // rows cut short by a failed write are dropped, not an error that blocks every channel delete
  for (const char* cut : {"R2\t1a2b", "R2\t1a2b\t", "V1\t", "D1\t"}) {
    line = cut;
    assert(device::remapChatRow(line, mapping, 3) == device::RemapRow::Drop);
  }
  int empty[1] = {-1};
  line = "V1\t0\t1\tREAD";
  assert(device::remapChatRow(line, empty, 1) == device::RemapRow::Drop);
  std::cout << "PASS: deletion remap/replay preserves other RX/TX/path/Heard/read/clear state; "
               "removes target references; malformed rows rejected; empty catalog mapping\n";
}
