#include <cassert>
#include <iostream>
#include "services/keyboard_layout.h"
using namespace mm_kbd;
int main() {
  // Russian ЙЦУКЕН: every QWERTY letter key.
  const char* qwerty = "qwertyuiopasdfghjklzxcvbnm";
  const char* ru[] = {"й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "ф", "ы", "в",
                      "а", "п", "р", "о", "л", "д", "я", "ч", "с", "м", "и", "т", "ь"};
  for (int i = 0; i < 26; i++)
    assert(letter(Lang::Russian, qwerty[i], false) == ru[i]);
  assert(letter(Lang::Russian, 'q', true) == "Й" && letter(Lang::Russian, 'z', true) == "Я" &&
         letter(Lang::Russian, 'F', false) == "а");
  // Ukrainian: і on the ы key, the rest as Russian.
  assert(letter(Lang::Ukrainian, 's', false) == "і" && letter(Lang::Ukrainian, 's', true) == "І" &&
         letter(Lang::Ukrainian, 'f', false) == "а");
  // Latin layouts type the key itself; non-letters are left to the caller.
  assert(letter(Lang::Italian, 'a', false) == "a" && letter(Lang::German, 'z', true) == "Z" &&
         letter(Lang::Russian, '1', false).empty());
  // Accent key: letters without a key of their own, successive presses cycle.
  assert(variant(Lang::Russian, 'p', 0, false) == "х" &&
         variant(Lang::Russian, 'p', 1, false) == "ъ" &&
         variant(Lang::Russian, 'p', 2, false) == "х");
  assert(variant(Lang::Russian, 'l', 0, false) == "ж" &&
         variant(Lang::Russian, 'l', 1, true) == "Э");
  assert(variant(Lang::Russian, 'm', 0, false) == "б" &&
         variant(Lang::Russian, 'm', 1, false) == "ю" &&
         variant(Lang::Russian, 't', 0, true) == "Ё");
  assert(variant(Lang::Ukrainian, 'p', 1, false) == "ї" &&
         variant(Lang::Ukrainian, 'l', 1, true) == "Є" &&
         variant(Lang::Ukrainian, 'u', 0, true) == "Ґ");
  assert(variant(Lang::Ukrainian, 't', 0, false).empty() &&
         variants(Lang::Russian, 'q').count == 0);
  assert(variant(Lang::German, 's', 0, true) == "ß" && variant(Lang::German, 'a', 0, true) == "Ä");
  assert(variant(Lang::French, 'e', 3, false) == "ë" &&
         variant(Lang::Spanish, 'n', 0, true) == "Ñ" &&
         variant(Lang::Portuguese, 'a', 3, false) == "ã");
  assert(variant(Lang::Italian, 'e', 0, false) == "è" &&
         variant(Lang::Italian, 'e', 1, false) == "é" &&
         variant(Lang::English, 'e', 0, false).empty());
  // Accent bar: the letters one / two taps give, in key order; nothing for English.
  assert(accentBar(Lang::Italian, 1, " ") == "à è ì ò ù" &&
         accentBar(Lang::Italian, 0, " ") == "à è ì ò ù" &&
         accentBar(Lang::Italian, 2, " ") == "é");
  assert(accentBar(Lang::Russian, 1, " ") == "ж б х ё" &&
         accentBar(Lang::Russian, 2, " ") == "э ю ъ" && accentBar(Lang::English, 1, " ").empty() &&
         accentBar(Lang::German, 5, " ").empty());
  // Enabled languages: Alt + Space order, a mask never ends up empty.
  uint32_t mask =
      (1u << int(Lang::Italian)) | (1u << int(Lang::Russian)) | (1u << int(Lang::Ukrainian));
  assert(next(mask, Lang::Italian) == Lang::Russian &&
         next(mask, Lang::Russian) == Lang::Ukrainian &&
         next(mask, Lang::Ukrainian) == Lang::Italian);
  assert(next(DefaultMask, Lang::Italian) == Lang::Italian && sanitize(0) == DefaultMask &&
         enabledCount(mask) == 3);
  assert(active(mask, Lang::German) == Lang::Italian &&
         active(DefaultMask, Lang::Russian) == Lang::Italian);
  // Search on code, native and English names.
  assert(matches(Lang::Russian, "rus") && matches(Lang::Russian, "RU") &&
         matches(Lang::Ukrainian, "ukr") && matches(Lang::German, "deu"));
  assert(!matches(Lang::French, "rus") && matches(Lang::Spanish, ""));
  for (int i = 0; i < LangCount; i++)
    assert(info(Lang(i)).code[0] && info(Lang(i)).name[0]);
  // Display / Alt + Space order: alphabetical by native name, every language once.
  const char* names[] = {"Deutsch",  "English",   "Español", "Français",
                         "Italiano", "Português", "Русский", "Українська"};
  uint32_t seen = 0;
  for (int i = 0; i < LangCount; i++) {
    assert(std::string(info(Order[i]).name) == names[i]);
    seen |= 1u << int(Order[i]);
  }
  assert(seen == (1u << LangCount) - 1);
  uint32_t de_en_it =
      (1u << int(Lang::German)) | (1u << int(Lang::English)) | (1u << int(Lang::Italian));
  assert(next(de_en_it, Lang::German) == Lang::English &&
         next(de_en_it, Lang::English) == Lang::Italian &&
         next(de_en_it, Lang::Italian) == Lang::German);
  std::cout << "PASS: keyboard layouts (Russian/Ukrainian ЙЦУКЕН, Latin), accent variants and "
               "cycling, accent bar, upper case, enabled-language cycle, search\n";
}
