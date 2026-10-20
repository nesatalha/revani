// Revani LVGL ayarları. Kart ve simülatör ortak kullanır; burada olmayan her
// ayar LVGL'in varsayılanıdır (lv_conf_internal.h).
#ifndef LV_CONF_H
#define LV_CONF_H

#ifdef REVANI_SIMULATOR
#define LV_COLOR_DEPTH 32
#define LV_USE_SDL 1
#define LV_SDL_INCLUDE_PATH <SDL.h>
#define LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_DIRECT
#define LV_SDL_BUF_COUNT 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1
#else
#define LV_COLOR_DEPTH 16
#endif

// Büyük tamponlar (ok tuvali) PSRAM'e düşsün diye libc malloc.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

#define LV_DEF_REFR_PERIOD 20

// Harita karesi JPEG (Faz 2); çözme ui/ui_map.c'de doğrudan TJpgDec ile
#define LV_USE_TJPGD 1

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_48 1

#endif // LV_CONF_H
