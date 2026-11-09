#include <cassert>
#include <iostream>
#include <vector>
#include "board/device_model.h"
#include "services/lock_messages.h"
#include "ui/theme.h"

#ifndef MM_BOARD_NM_EPD
#error This regression test must use the NM board configuration
#endif

static void onScreen(mm::Rect r) {
  assert(r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0);
  assert(r.x + r.w <= mm::W && r.y + r.h <= mm::H);
}

int main() {
  static_assert(mm::W == 400 && mm::H == 300, "Use the physical NM panel dimensions");
  for (int row = 0; row < mm::ContactRows; row++) {
    auto r = mm::contactRowRect(row);
    onScreen(r);
    assert(r.y + r.h < mm::ContactNoticeY);
  }
  assert(mm::ContactNoticeY + 14 <= mm::ContactPrevious.y);
  for (auto r : {mm::ContactPrevious, mm::ContactNext, mm::LockMessage, mm::LockRepeater,
                 mm::QuickReplyPanel, mm::QuickReplyComposer, mm::QuickReplySend,
                 mm::QuickReplyCancel, mm::ScopeAdd, mm::ScopeApply, mm::ScopeCancel,
                 mm::LoRaApply, mm::LoRaAdvert, mm::DateSave, mm::DateCancel})
    onScreen(r);
  assert(mm::QuickReplyComposer.y + mm::QuickReplyComposer.h < mm::QuickReplySend.y);
  for (int i = 0; i < mm::MenuHomeRows; i++) {
    auto r = mm::menuHomeRowRect(i);
    onScreen(r);
    assert(r.y + r.h < mm::MenuHomeTabY);
  }
  for (int i = 0; i < mm::MenuHomeTabCount; i++)
    onScreen(mm::menuHomeTabRect(i));

  // BOOT browses older unread messages; USER returns towards newer messages,
  // including from the empty end state and after records are evicted/read.
  device::LockMessages messages;
  std::vector<uint32_t> unread = {11, 22, 33};
  messages.reset(unread);
  assert(messages.next(unread) == 22);
  assert(messages.previous(unread) == 33);
  assert(messages.previous(unread) == 33);
  assert(messages.next(unread) == 22);
  assert(messages.next(unread) == 11);
  assert(messages.next(unread) == 0 && messages.ended);
  assert(messages.previous(unread) == 33 && !messages.ended);
  unread = {11, 22};
  assert(messages.previous(unread) == 22);
  unread.clear();
  assert(messages.previous(unread) == 0 && messages.ended);
  unread = {44};
  messages.receive(unread, 44);
  assert(messages.current == 44 && !messages.browsing);
  std::cout << "PASS: " MM_DEVICE_MODEL " landscape geometry and two-button unread browsing\n";
}
