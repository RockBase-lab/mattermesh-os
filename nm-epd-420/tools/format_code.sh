#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Format owned C/C++ sources and tests. Keep generated assets and third-party code untouched.
shopt -s globstar nullglob
files=(src/**/*.{c,cpp,h} sim/**/*.{c,cpp,h})
for file in "${files[@]}"; do
  case "$file" in
    src/qr/qrcodegen.c|src/qr/qrcodegen.h|\
    src/ui/build_info.h|src/ui/*_font.h|src/ui/*_bitmap.h|\
    src/ui/emoji_picker.h|src/ui/lock_clock_glyphs.h|\
    src/ui/lock_wallpapers.h|src/ui/weather_icons.h)
      continue
      ;;
  esac
  clang-format -i "$file"
done
