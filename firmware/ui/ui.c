#include "ui.h"

#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "ui_arrows.h"
#include "ui_config.h"
#include "ui_fonts.h"

#define OUTER_RING_SIZE (UI_SCREEN_SIZE - 2 * UI_EDGE_MARGIN)
#define INNER_RING_SIZE (OUTER_RING_SIZE - 2 * (UI_OUTER_RING_WIDTH + UI_INNER_RING_GAP))

typedef struct {
  // Ortak: halkalar ve hız
  lv_obj_t *speed_ring;
  lv_obj_t *speed_value;
  lv_obj_t *speed_unit;
  int32_t speed_x10;  // halkada gösterilen değer (km/h × 10), animasyonla değişir
  bool gps_ever;
  bool gps_lost;
  uint32_t gps_last_tick;

  lv_obj_t *dist_ring;  // UI_INNER_RING_ENABLED 0 ise NULL
  int32_t dist_ring_value;
  bool dist_ring_blinking;

  // Navigasyon sayfası
  lv_obj_t *nav_page;
  lv_obj_t *arrow;
  lv_obj_t *dist_value;
  lv_obj_t *dist_unit;
  lv_obj_t *street;
  bool nav_active;

  // Bekleme sayfası
  lv_obj_t *idle_page;
  lv_obj_t *clock;
  lv_obj_t *status;
  lv_obj_t *battery_fill;
  lv_obj_t *battery_label;
  bool connected;
} ui_t;

static ui_t ui;

static lv_color_t color(uint32_t hex) { return lv_color_hex(hex); }

static lv_obj_t *make_page(lv_obj_t *parent) {
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_remove_style_all(page);
  lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
  lv_obj_remove_flag(page, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  return page;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, uint32_t hex) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color(hex), 0);
  lv_label_set_text(l, "");
  return l;
}

static lv_obj_t *make_ring(lv_obj_t *parent, int32_t size, int32_t width, int32_t max) {
  lv_obj_t *arc = lv_arc_create(parent);
  lv_obj_set_size(arc, size, size);
  lv_obj_center(arc);
  lv_arc_set_bg_angles(arc, UI_RING_START_ANGLE, UI_RING_END_ANGLE);
  lv_arc_set_range(arc, 0, max);
  lv_arc_set_value(arc, 0);
  lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_all(arc, 0, 0);
  lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, color(UI_COLOR_TRACK), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, color(UI_COLOR_FG), LV_PART_INDICATOR);
  return arc;
}

// ---------------------------------------------------------------- dış halka

static void speed_anim_cb(void *var, int32_t v) {
  LV_UNUSED(var);
  ui.speed_x10 = v;
  lv_arc_set_value(ui.speed_ring, v);
  if (ui.gps_ever) lv_label_set_text_fmt(ui.speed_value, "%d", (int)((v + 5) / 10));
}

static void apply_gps_style(void) {
  uint32_t c = ui.gps_lost ? UI_COLOR_DIM : UI_COLOR_FG;
  lv_obj_set_style_arc_color(ui.speed_ring, color(c), LV_PART_INDICATOR);
  lv_obj_set_style_text_color(ui.speed_value, color(c), 0);
}

static void gps_watchdog_cb(lv_timer_t *t) {
  LV_UNUSED(t);
  bool lost = !ui.gps_ever || lv_tick_elaps(ui.gps_last_tick) > UI_GPS_TIMEOUT_MS;
  if (lost != ui.gps_lost) {
    ui.gps_lost = lost;
    if (lost) lv_anim_delete(ui.speed_ring, speed_anim_cb);  // son değer silik kalır
    apply_gps_style();
  }
}

void ui_set_gps(uint8_t speed_kmh, uint16_t heading_deg) {
  LV_UNUSED(heading_deg);  // Faz 2'de harita yönü için
  int32_t kmh = speed_kmh < UI_SPEED_MIN_KMH ? 0 : speed_kmh;
  if (kmh > UI_SPEED_MAX_KMH) kmh = UI_SPEED_MAX_KMH;

  ui.gps_last_tick = lv_tick_get();
  if (!ui.gps_ever) {
    ui.gps_ever = true;
    speed_anim_cb(NULL, ui.speed_x10);
  }
  gps_watchdog_cb(NULL);

  lv_anim_delete(ui.speed_ring, speed_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ui.speed_ring);
  lv_anim_set_exec_cb(&a, speed_anim_cb);
  lv_anim_set_values(&a, ui.speed_x10, kmh * 10);
  lv_anim_set_duration(&a, UI_SPEED_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

// ---------------------------------------------------------------- iç halka

#if UI_INNER_RING_ENABLED
static void dist_ring_anim_cb(void *var, int32_t v) {
  ui.dist_ring_value = v;
  lv_arc_set_value((lv_obj_t *)var, v);
}

static void blink_anim_cb(void *var, int32_t v) {
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_INDICATOR);
}

static void dist_ring_blink(bool on) {
  if (on == ui.dist_ring_blinking) return;
  ui.dist_ring_blinking = on;
  lv_anim_delete(ui.dist_ring, blink_anim_cb);
  lv_obj_set_style_opa(ui.dist_ring, LV_OPA_COVER, LV_PART_INDICATOR);
  if (!on) return;
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ui.dist_ring);
  lv_anim_set_exec_cb(&a, blink_anim_cb);
  lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_20);
  lv_anim_set_duration(&a, 300);
  lv_anim_set_playback_duration(&a, 300);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void dist_ring_update(bool visible, uint16_t distance_m) {
  if (!visible || distance_m > UI_INNER_RING_RANGE_M) {
    lv_obj_add_flag(ui.dist_ring, LV_OBJ_FLAG_HIDDEN);
    lv_anim_delete(ui.dist_ring, dist_ring_anim_cb);
    dist_ring_anim_cb(ui.dist_ring, 0);
    dist_ring_blink(false);
    return;
  }
  lv_obj_remove_flag(ui.dist_ring, LV_OBJ_FLAG_HIDDEN);

  uint32_t c = distance_m <= UI_INNER_RING_TURN_M   ? UI_COLOR_GREEN
               : distance_m <= UI_INNER_RING_WARN_M ? UI_COLOR_YELLOW
                                                    : UI_COLOR_FG;
  lv_obj_set_style_arc_color(ui.dist_ring, color(c), LV_PART_INDICATOR);
  dist_ring_blink(distance_m <= UI_INNER_RING_TURN_M);

  int32_t target = UI_INNER_RING_RANGE_M - distance_m;
  if (distance_m <= UI_INNER_RING_TURN_M) target = UI_INNER_RING_RANGE_M;
  lv_anim_delete(ui.dist_ring, dist_ring_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ui.dist_ring);
  lv_anim_set_exec_cb(&a, dist_ring_anim_cb);
  lv_anim_set_values(&a, ui.dist_ring_value, target);
  lv_anim_set_duration(&a, UI_INNER_RING_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}
#else
static void dist_ring_update(bool visible, uint16_t distance_m) {
  LV_UNUSED(visible);
  LV_UNUSED(distance_m);
}
#endif

// ---------------------------------------------------------------- navigasyon

static void format_distance(uint16_t m, char *value, size_t vlen, const char **unit) {
  if (m < 1000) {
    // Google Maps 100 m üstünü zaten 10'a, altını 5'e yuvarlıyor; aynen göster.
    snprintf(value, vlen, "%u", (unsigned)m);
    *unit = "m";
  } else if (m < 10000) {
    unsigned tenths = (m + 50) / 100;
    snprintf(value, vlen, "%u,%u", tenths / 10, tenths % 10);
    *unit = "km";
  } else {
    snprintf(value, vlen, "%u", (unsigned)((m + 500) / 1000));
    *unit = "km";
  }
}

static void show_page(bool nav) {
  ui.nav_active = nav;
  if (nav) {
    lv_obj_remove_flag(ui.nav_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.idle_page, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(ui.nav_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui.idle_page, LV_OBJ_FLAG_HIDDEN);
  }
}

void ui_set_nav(const ui_nav_t *nav) {
  show_page(true);
  ui_arrow_set(ui.arrow, nav->maneuver, nav->roundabout_exit);

  char value[12];
  const char *unit;
  format_distance(nav->distance_m, value, sizeof(value), &unit);
  lv_label_set_text(ui.dist_value, value);
  lv_label_set_text(ui.dist_unit, unit);

  const char *street = nav->street ? nav->street : "";
  if (strcmp(lv_label_get_text(ui.street), street) != 0) lv_label_set_text(ui.street, street);

  dist_ring_update(nav->maneuver != UI_MANEUVER_ARRIVE, nav->distance_m);
}

void ui_end_nav(void) {
  show_page(false);
  dist_ring_update(false, 0);
}

// ---------------------------------------------------------------- bekleme

static void status_pulse_cb(void *var, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0); }

void ui_set_connected(bool connected) {
  ui.connected = connected;
  lv_anim_delete(ui.status, status_pulse_cb);
  lv_obj_set_style_opa(ui.status, LV_OPA_COVER, 0);
  if (connected) {
    lv_label_set_text(ui.status, "Bağlı");
    lv_obj_set_style_text_color(ui.status, color(UI_COLOR_FG), 0);
    return;
  }
  lv_label_set_text(ui.status, "Telefon bekleniyor");
  lv_obj_set_style_text_color(ui.status, color(UI_COLOR_YELLOW), 0);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ui.status);
  lv_anim_set_exec_cb(&a, status_pulse_cb);
  lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_30);
  lv_anim_set_duration(&a, 1200);
  lv_anim_set_playback_duration(&a, 1200);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  if (ui.nav_active) ui_end_nav();
}

void ui_set_time(uint8_t hour, uint8_t minute) {
  lv_label_set_text_fmt(ui.clock, "%02u:%02u", hour, minute);
}

#define BATTERY_W 44
#define BATTERY_H 22
#define BATTERY_BORDER 3

void ui_set_battery(uint8_t percent) {
  if (percent > 100) percent = 100;
  int32_t inner = BATTERY_W - 2 * BATTERY_BORDER - 4;
  lv_obj_set_width(ui.battery_fill, LV_MAX(2, inner * percent / 100));
  uint32_t c = percent <= 20 ? UI_COLOR_YELLOW : UI_COLOR_FG;
  lv_obj_set_style_bg_color(ui.battery_fill, color(c), 0);
  lv_label_set_text_fmt(ui.battery_label, "%%%u", percent);  // Türkçede yüzde işareti önde
}

static void build_battery(lv_obj_t *parent) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 10, 0);
  lv_obj_align(row, LV_ALIGN_CENTER, 0, 96);

  lv_obj_t *body = lv_obj_create(row);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, BATTERY_W, BATTERY_H);
  lv_obj_set_style_border_width(body, BATTERY_BORDER, 0);
  lv_obj_set_style_border_color(body, color(UI_COLOR_FG), 0);
  lv_obj_set_style_radius(body, 4, 0);
  lv_obj_set_style_pad_all(body, 2, 0);

  ui.battery_fill = lv_obj_create(body);
  lv_obj_remove_style_all(ui.battery_fill);
  lv_obj_set_height(ui.battery_fill, LV_PCT(100));
  lv_obj_set_style_bg_opa(ui.battery_fill, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(ui.battery_fill, 1, 0);

  lv_obj_t *nub = lv_obj_create(row);
  lv_obj_remove_style_all(nub);
  lv_obj_set_size(nub, 4, 10);
  lv_obj_set_style_bg_color(nub, color(UI_COLOR_FG), 0);
  lv_obj_set_style_bg_opa(nub, LV_OPA_COVER, 0);
  lv_obj_set_style_margin_left(nub, -10, 0);  // gövdeye yapışık

  ui.battery_label = make_label(row, &lv_font_montserrat_20, UI_COLOR_FG);
}

// ---------------------------------------------------------------- kurulum

void ui_init(void) {
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, color(UI_COLOR_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // Bekleme sayfası
  ui.idle_page = make_page(scr);
  ui.clock = make_label(ui.idle_page, &revani_font_72, UI_COLOR_FG);
  lv_label_set_text(ui.clock, "--:--");
  lv_obj_align(ui.clock, LV_ALIGN_CENTER, 0, -44);
  ui.status = make_label(ui.idle_page, &revani_font_30, UI_COLOR_YELLOW);
  lv_obj_align(ui.status, LV_ALIGN_CENTER, 0, 36);
  build_battery(ui.idle_page);

  // Navigasyon sayfası
  ui.nav_page = make_page(scr);
  ui.arrow = ui_arrow_create(ui.nav_page);
  lv_obj_align(ui.arrow, LV_ALIGN_TOP_MID, 0, 62);

  lv_obj_t *dist_row = lv_obj_create(ui.nav_page);
  lv_obj_remove_style_all(dist_row);
  lv_obj_set_size(dist_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(dist_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(dist_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_column(dist_row, 8, 0);
  lv_obj_align(dist_row, LV_ALIGN_CENTER, 0, 30);
  ui.dist_value = make_label(dist_row, &revani_font_72, UI_COLOR_FG);
  ui.dist_unit = make_label(dist_row, &revani_font_30, UI_COLOR_FG);
  lv_obj_set_style_pad_bottom(ui.dist_unit, 14, 0);  // rakamların taban çizgisine hizala

  ui.street = make_label(ui.nav_page, &revani_font_30, UI_COLOR_YELLOW);
  lv_obj_set_width(ui.street, 290);
  lv_obj_set_style_text_align(ui.street, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(ui.street, LV_LABEL_LONG_DOT);
  lv_obj_set_height(ui.street, lv_font_get_line_height(&revani_font_30));  // tek satır
  lv_obj_align(ui.street, LV_ALIGN_CENTER, 0, 92);

  // Halkalar ve hız her iki sayfada da görünür
  ui.speed_ring = make_ring(scr, OUTER_RING_SIZE, UI_OUTER_RING_WIDTH, UI_SPEED_MAX_KMH * 10);
#if UI_INNER_RING_ENABLED
  ui.dist_ring = make_ring(scr, INNER_RING_SIZE, UI_INNER_RING_WIDTH, UI_INNER_RING_RANGE_M);
  lv_obj_set_style_arc_color(ui.dist_ring, color(UI_COLOR_TRACK), LV_PART_MAIN);
  lv_obj_add_flag(ui.dist_ring, LV_OBJ_FLAG_HIDDEN);
#endif

  ui.speed_value = make_label(scr, &lv_font_montserrat_48, UI_COLOR_FG);
  lv_label_set_text(ui.speed_value, "--");
  lv_obj_align(ui.speed_value, LV_ALIGN_BOTTOM_MID, 0, -58);
  ui.speed_unit = make_label(scr, &lv_font_montserrat_20, UI_COLOR_DIM);
  lv_label_set_text(ui.speed_unit, "km/h");
  lv_obj_align(ui.speed_unit, LV_ALIGN_BOTTOM_MID, 0, -36);

  ui.gps_lost = true;
  apply_gps_style();
  lv_timer_create(gps_watchdog_cb, 250, NULL);

  ui_set_battery(100);
  ui_set_connected(false);
  show_page(false);
}
