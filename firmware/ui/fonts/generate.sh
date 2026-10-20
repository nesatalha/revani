#!/usr/bin/env bash
# Arayüz fontlarını üretir (lv_font_conv). Montserrat TTF'i LVGL paketinden alınır,
# bu yüzden önce firmware/ içinde `pio pkg install` çalışmış olmalı.
set -euo pipefail
cd "$(dirname "$0")"

TTF=../../.pio/libdeps/revani/lvgl/scripts/built_in_font/Montserrat-Medium.ttf
if [[ ! -f $TTF ]]; then
  echo "Font bulunamadı: $TTF (firmware/ içinde 'pio pkg install' çalıştır)" >&2
  exit 1
fi

CONV="npx --yes lv_font_conv@1.5.3"
TURKCE=0xC2,0xC7,0xCE,0xD6,0xDB,0xDC,0xE2,0xE7,0xEE,0xF6,0xFB,0xFC,0x11E,0x11F,0x130,0x131,0x15E,0x15F

# Büyük rakamlar: mesafe ve saat
$CONV --font "$TTF" --size 72 --bpp 4 --no-compress --format lvgl --lv-include lvgl.h \
  --symbols " ,.:-0123456789km" -o revani_font_72.c
# Cadde adı ve durum yazıları (Türkçe karakterli)
$CONV --font "$TTF" --size 30 --bpp 4 --no-compress --format lvgl --lv-include lvgl.h \
  -r 0x20-0x7E -r "$TURKCE" -o revani_font_30.c

# Retro LCD hız rakamları: DSEG7 Classic (OFL-1.1, keshikan). "!" rakam genişliğinde boşluk.
$CONV --font DSEG7Classic-Bold.ttf --size 60 --bpp 4 --no-compress --format lvgl --lv-include lvgl.h \
  --symbols "0123456789-!" -o revani_font_lcd.c
