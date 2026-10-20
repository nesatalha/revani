// Revani LVGL simülatörü (macOS, SDL2).
//
//   revani_sim [--speed N]                      pencerede sahte veriyle çalışır
//   revani_sim --shot out.bmp [--scene S] [--ms N]
//                                               penceresiz çizer, ekran görüntüsü yazar
//
// Sahneler: idle, connected, nav, near, turn, far, gps-lost, arrows, fake

#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "screenshot.h"
#include "ui.h"
#include "ui_arrows.h"
#include "ui_config.h"
#include "ui_fake.h"

#define W UI_SCREEN_SIZE
#define H UI_SCREEN_SIZE
#define DESK_COLOR 0x262626  // yuvarlak ekranın dışı

// Gerçek panel yuvarlak: köşeleri kalın bir çember kenarlığıyla örter.
static void add_round_mask(void) {
  const int32_t border = 110;  // köşeler merkezden 240·√2 ≈ 340 px uzakta
  lv_obj_t *mask = lv_obj_create(lv_layer_sys());
  lv_obj_remove_style_all(mask);
  lv_obj_set_size(mask, W + 2 * border, H + 2 * border);
  lv_obj_center(mask);
  lv_obj_set_style_radius(mask, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(mask, border, 0);
  lv_obj_set_style_border_color(mask, lv_color_hex(DESK_COLOR), 0);
  lv_obj_set_style_border_opa(mask, LV_OPA_COVER, 0);
  lv_obj_remove_flag(mask, LV_OBJ_FLAG_CLICKABLE);
}

// ---------------------------------------------------------------- penceresiz mod

static uint32_t fake_ms;
static uint32_t fake_tick(void) { return fake_ms; }
static uint32_t framebuffer[W * H];

static void headless_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px) {
  lv_display_flush_ready(disp);
}

static void run_for(uint32_t ms) {
  for (uint32_t t = 0; t < ms; t += 5) {
    fake_ms += 5;
    lv_timer_handler();
  }
}

static void host_time(void) {
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  ui_set_time((uint8_t)tm.tm_hour, (uint8_t)tm.tm_min);
}

static void scene_nav(uint8_t maneuver, uint8_t exit, uint16_t dist, const char *street, uint8_t speed) {
  ui_set_connected(true);
  ui_set_gps(speed, 0);
  ui_nav_t nav = {.maneuver = maneuver, .roundabout_exit = exit, .distance_m = dist, .street = street};
  ui_set_nav(&nav);
}

static void scene_arrows(void) {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COLOR_BG), 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  // Varsayılan font Türkçe karakter içermiyor, etiketler ASCII.
  static const char *names[] = {"0 duz",       "1 hafif sag", "2 sag", "3 keskin sag", "4 U donusu",
                                "5 hafif sol", "6 sol",       "7 keskin sol", "8 donel (2)", "9 varis"};
  for (int i = 0; i < UI_MANEUVER_COUNT; i++) {
    int col = i % 4, row = i / 4;
    lv_obj_t *a = ui_arrow_create(scr);
    ui_arrow_set(a, (uint8_t)i, i == UI_MANEUVER_ROUNDABOUT ? 2 : 0);
    lv_obj_set_style_transform_scale(a, 170, 0);  // 160 px → ~106 px
    lv_obj_set_style_transform_pivot_x(a, 0, 0);
    lv_obj_set_style_transform_pivot_y(a, 0, 0);
    lv_obj_set_pos(a, col * 120 + 7, row * 160 + 8);
    lv_obj_t *l = lv_label_create(scr);
    lv_label_set_text(l, names[i]);
    lv_obj_set_style_text_color(l, lv_color_hex(UI_COLOR_DIM), 0);
    lv_obj_set_pos(l, col * 120 + 10, row * 160 + 120);
  }
  lv_screen_load(scr);
}

static int headless(const char *out, const char *scene, uint32_t ms) {
  lv_tick_set_cb(fake_tick);
  lv_display_t *disp = lv_display_create(W, H);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_XRGB8888);
  lv_display_set_buffers(disp, framebuffer, NULL, sizeof(framebuffer), LV_DISPLAY_RENDER_MODE_DIRECT);
  lv_display_set_flush_cb(disp, headless_flush);

  ui_init();
  host_time();
  ui_set_battery(76);
  bool mask = true;

  if (!strcmp(scene, "idle")) {
  } else if (!strcmp(scene, "connected")) {
    ui_set_connected(true);
    ui_set_gps(38, 0);
  } else if (!strcmp(scene, "nav")) {
    scene_nav(UI_MANEUVER_RIGHT, 0, 180, "Bağdat Caddesi", 47);
  } else if (!strcmp(scene, "near")) {
    scene_nav(UI_MANEUVER_LEFT, 0, 60, "Kadıköy İskele Sokağı", 31);
  } else if (!strcmp(scene, "turn")) {
    scene_nav(UI_MANEUVER_ROUNDABOUT, 2, 10, "Kozyatağı Kavşağı", 24);
  } else if (!strcmp(scene, "far")) {
    scene_nav(UI_MANEUVER_STRAIGHT, 0, 1800, "D-100 Karayolu", 92);
  } else if (!strcmp(scene, "gps-lost")) {
    scene_nav(UI_MANEUVER_SLIGHT_RIGHT, 0, 320, "Söğütlüçeşme Caddesi", 64);
    run_for(1200);
    ms += UI_GPS_TIMEOUT_MS;
  } else if (!strcmp(scene, "arrows")) {
    scene_arrows();
    mask = false;
  } else if (!strcmp(scene, "fake")) {
    ui_fake_start(1);
  } else {
    fprintf(stderr, "Bilinmeyen sahne: %s\n", scene);
    return 2;
  }
  if (mask) add_round_mask();

  run_for(ms);
  lv_refr_now(disp);
  if (!screenshot_write_bmp(out, framebuffer, W, H)) {
    perror(out);
    return 1;
  }
  printf("%s yazıldı (sahne %s, %u ms)\n", out, scene, (unsigned)ms);
  return 0;
}

// ---------------------------------------------------------------- pencere modu

static int windowed(int speedup) {
  lv_display_t *disp = lv_sdl_window_create(W, H);
  lv_sdl_window_set_title(disp, "Revani");
  ui_init();
  add_round_mask();
  ui_fake_start(speedup);
  for (;;) {
    uint32_t wait = lv_timer_handler();
    usleep((wait < 5 ? wait : 5) * 1000);
  }
  return 0;
}

int main(int argc, char **argv) {
  const char *shot = NULL, *scene = "nav";
  uint32_t ms = 2000;
  int speedup = 1;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
    else if (!strcmp(argv[i], "--scene") && i + 1 < argc) scene = argv[++i];
    else if (!strcmp(argv[i], "--ms") && i + 1 < argc) ms = (uint32_t)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--speed") && i + 1 < argc) speedup = atoi(argv[++i]);
    else {
      fprintf(stderr, "Kullanım: %s [--speed N] | --shot out.bmp [--scene S] [--ms N]\n", argv[0]);
      return 2;
    }
  }

  lv_init();
  return shot ? headless(shot, scene, ms) : windowed(speedup);
}
