#include "ui.h"

#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui_arrows.h"
#include "ui_config.h"
#include "ui_fonts.h"
#include "ui_internal.h"

#define DIAL_SIZE (UI_SCREEN_SIZE - 2 * UI_EDGE_MARGIN)
#define CENTER (UI_SCREEN_SIZE / 2)

typedef struct {
  // Ortak: halkalar ve hız
  lv_obj_t *dial;    // retro analog kadran: çentikler ve 20'şer sayılar
  lv_obj_t *needle;  // turuncu kama ibre (kendisi çizer, bkz. needle_draw_cb)
  float needle_angle;  // derece
  lv_area_t needle_area;
  lv_obj_t *speed_panel;  // LCD rakamlı hız kutusu
  lv_obj_t *speed_ghost;  // sönük "888" segmentleri
  lv_obj_t *speed_value;
  lv_obj_t *speed_unit;
  int32_t speed_x10;  // halkada gösterilen değer (km/h × 10), animasyonla değişir
  bool gps_ever;
  bool gps_lost;
  uint32_t gps_last_tick;

  lv_obj_t *dist_ring;  // UI_INNER_RING_ENABLED 0 ise NULL
  int32_t dist_ring_value;
  bool dist_ring_blinking;

  // Navigasyon sayfası: harita yokken büyük ok (classic), harita varken map_view
  lv_obj_t *nav_page;
  lv_obj_t *classic;
  lv_obj_t *arrow;
  lv_obj_t *dist_value;
  lv_obj_t *dist_unit;
  lv_obj_t *street;
  bool nav_active;

  // Harita görünümü (Faz 2)
  lv_obj_t *map_view;
  lv_obj_t *map_img;
  lv_obj_t *map_arrow;
  lv_obj_t *map_dist;
  lv_obj_t *map_street;
  uint32_t map_last_tick;
  bool map_ever;
  bool map_mode;

  // Bekleme sayfası
  lv_obj_t *idle_page;
  lv_obj_t *clock;
  lv_obj_t *status;
  lv_obj_t *battery_row;
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

// İbre: dışta geniş, içe doğru sivrilen kama. Gövde tek üçgen, baş kadranın eğrisini
// izleyen kalın bir yay bandı ("eğri kenarlı üçgen"). Merkezden değil çentik bandından
// çizilir, ortadaki harita/ok kapanmasın.
static void needle_point(float deg, float r, lv_point_precise_t *p) {
  float a = deg * 3.14159265f / 180.0f;
  p->x = CENTER + (int32_t)lroundf(cosf(a) * r);
  p->y = CENTER + (int32_t)lroundf(sinf(a) * r);
}

static void draw_wedge(lv_layer_t *layer, float deg, float half_deg, float r_out, float r_in, lv_color_t c) {
  // Önce baş (yay bandı), sonra gövde: üçgenin dış kenarı bandın içinde kalır, dikiş görünmez.
  lv_draw_arc_dsc_t arc;
  lv_draw_arc_dsc_init(&arc);
  arc.color = c;
  arc.center.x = CENTER;
  arc.center.y = CENTER;
  arc.radius = (uint16_t)r_out;
  arc.width = UI_NEEDLE_HEAD;
  arc.start_angle = deg - half_deg;
  arc.end_angle = deg + half_deg;
  lv_draw_arc(layer, &arc);

  lv_draw_triangle_dsc_t t;
  lv_draw_triangle_dsc_init(&t);
  t.bg_color = c;
  t.bg_opa = LV_OPA_COVER;
  // Kenarlar bandın iç köşelerinden geçip dış yarıçapa kadar uzar: köşede basamak kalmaz.
  needle_point(deg, r_in, &t.p[0]);
  for (int i = 1; i <= 2; i++) {
    lv_point_precise_t corner;
    needle_point(i == 1 ? deg - half_deg : deg + half_deg, r_out - UI_NEEDLE_HEAD, &corner);
    float scale = (r_out - r_in) / (r_out - UI_NEEDLE_HEAD - r_in);
    t.p[i].x = t.p[0].x + (int32_t)lroundf((corner.x - t.p[0].x) * scale);
    t.p[i].y = t.p[0].y + (int32_t)lroundf((corner.y - t.p[0].y) * scale);
  }
  lv_draw_triangle(layer, &t);
}

static void needle_draw_cb(lv_event_t *e) {
  lv_layer_t *layer = lv_event_get_layer(e);
  float a = ui.needle_angle;
  // Önce siyah kontur (haritanın ve sayıların üstünde seçilsin), sonra renkli kama
  draw_wedge(layer, a, UI_NEEDLE_HALF_DEG + 1.0f, UI_NEEDLE_OUTER_R + 2, UI_NEEDLE_INNER_R - 4, color(UI_COLOR_BG));
  lv_color_t c = color(ui.gps_lost ? UI_COLOR_DIM : UI_COLOR_NEEDLE);
  draw_wedge(layer, a, UI_NEEDLE_HALF_DEG, UI_NEEDLE_OUTER_R, UI_NEEDLE_INNER_R, c);
}

// Kamanın kapladığı dikdörtgen (yalnız orası yeniden çizilsin).
static void needle_bounds(float deg, lv_area_t *out) {
  const float half = UI_NEEDLE_HALF_DEG + 1.0f, r_out = UI_NEEDLE_OUTER_R + 2, r_in = UI_NEEDLE_INNER_R - 4;
  lv_point_precise_t p[4];
  needle_point(deg, r_in, &p[0]);
  needle_point(deg - half, r_out, &p[1]);
  needle_point(deg + half, r_out, &p[2]);
  needle_point(deg, r_out, &p[3]);  // dış yayın ortası en dışta
  *out = (lv_area_t){p[0].x, p[0].y, p[0].x, p[0].y};
  for (int i = 1; i < 4; i++) {
    out->x1 = LV_MIN(out->x1, p[i].x);
    out->y1 = LV_MIN(out->y1, p[i].y);
    out->x2 = LV_MAX(out->x2, p[i].x);
    out->y2 = LV_MAX(out->y2, p[i].y);
  }
  lv_area_increase(out, 3, 3);
}

static void set_needle(int32_t speed_x10) {
  float frac = (float)speed_x10 / (UI_SPEED_MAX_KMH * 10);
  float deg = UI_RING_START_ANGLE + frac * UI_RING_SWEEP;
  if (deg == ui.needle_angle && ui.needle_area.x2 != 0) return;
  lv_obj_invalidate_area(ui.needle, &ui.needle_area);
  ui.needle_angle = deg;
  needle_bounds(deg, &ui.needle_area);
  lv_obj_invalidate_area(ui.needle, &ui.needle_area);
}

// LCD kutusu sağa dayalı 3 hane; "!" DSEG'de rakam genişliğinde boşluk.
static void set_speed_text(const char *digits) {
  char buf[4] = "!!!";
  size_t n = strlen(digits);
  if (n > 3) n = 3;
  memcpy(buf + 3 - n, digits, n);
  lv_label_set_text(ui.speed_value, buf);
}

static void speed_anim_cb(void *var, int32_t v) {
  LV_UNUSED(var);
  ui.speed_x10 = v;
  set_needle(v);
  if (ui.gps_ever) {
    char digits[8];
    snprintf(digits, sizeof digits, "%d", (int)((v + 5) / 10));
    set_speed_text(digits);
  }
}

static void apply_gps_style(void) {
  lv_obj_invalidate_area(ui.needle, &ui.needle_area);
  lv_obj_set_style_text_color(ui.speed_value, color(ui.gps_lost ? UI_COLOR_DIM : UI_COLOR_RETRO), 0);
}

static void gps_watchdog_cb(lv_timer_t *t) {
  LV_UNUSED(t);
  bool lost = !ui.gps_ever || lv_tick_elaps(ui.gps_last_tick) > UI_GPS_TIMEOUT_MS;
  if (lost != ui.gps_lost) {
    ui.gps_lost = lost;
    if (lost) lv_anim_delete(ui.needle, speed_anim_cb);  // son değer silik kalır
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

  lv_anim_delete(ui.needle, speed_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ui.needle);
  lv_anim_set_exec_cb(&a, speed_anim_cb);
  lv_anim_set_values(&a, ui.speed_x10, kmh * 10);
  lv_anim_set_duration(&a, UI_SPEED_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

// Retro analog kadran: 0–UI_SPEED_MAX_KMH, 5'erli ince, 20'şer kalın çentik ve sayılar.
static void build_dial(lv_obj_t *parent) {
  lv_obj_t *d = lv_scale_create(parent);
  ui.dial = d;
  lv_obj_set_size(d, DIAL_SIZE, DIAL_SIZE);
  lv_obj_center(d);
  lv_scale_set_mode(d, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_range(d, 0, UI_SPEED_MAX_KMH);
  lv_scale_set_angle_range(d, UI_RING_SWEEP);
  lv_scale_set_rotation(d, UI_RING_START_ANGLE);
  lv_scale_set_total_tick_count(d, UI_SPEED_MAX_KMH / 5 + 1);
  lv_scale_set_major_tick_every(d, 4);
  lv_scale_set_label_show(d, true);
  lv_obj_remove_flag(d, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_set_style_arc_width(d, 2, LV_PART_MAIN);
  lv_obj_set_style_arc_color(d, color(UI_COLOR_RETRO_DIM), LV_PART_MAIN);
  lv_obj_set_style_length(d, 8, LV_PART_ITEMS);
  lv_obj_set_style_line_width(d, 2, LV_PART_ITEMS);
  lv_obj_set_style_line_color(d, color(UI_COLOR_RETRO_DIM), LV_PART_ITEMS);
  lv_obj_set_style_length(d, 16, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(d, 4, LV_PART_INDICATOR);
  lv_obj_set_style_line_color(d, color(UI_COLOR_RETRO), LV_PART_INDICATOR);
  lv_obj_set_style_text_font(d, &lv_font_montserrat_20, LV_PART_INDICATOR);
  lv_obj_set_style_text_color(d, color(UI_COLOR_RETRO), LV_PART_INDICATOR);

  ui.needle = lv_obj_create(parent);
  lv_obj_remove_style_all(ui.needle);
  lv_obj_set_size(ui.needle, UI_SCREEN_SIZE, UI_SCREEN_SIZE);
  lv_obj_remove_flag(ui.needle, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui.needle, needle_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
  set_needle(0);
}

// Hunter'ın küçük LCD'si gibi: koyu kutu, sönük 888 segmentleri, üstünde açık rakamlar.
static void build_speed_panel(lv_obj_t *parent) {
  ui.speed_panel = lv_obj_create(parent);
  lv_obj_remove_style_all(ui.speed_panel);
  lv_obj_set_size(ui.speed_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(ui.speed_panel, color(UI_COLOR_LCD_BG), 0);
  lv_obj_set_style_bg_opa(ui.speed_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(ui.speed_panel, color(UI_COLOR_RETRO_DIM), 0);
  lv_obj_set_style_border_width(ui.speed_panel, 2, 0);
  lv_obj_set_style_radius(ui.speed_panel, 10, 0);
  lv_obj_set_style_pad_hor(ui.speed_panel, 10, 0);
  lv_obj_set_style_pad_ver(ui.speed_panel, 6, 0);
  lv_obj_align(ui.speed_panel, LV_ALIGN_BOTTOM_MID, 0, -50);

  ui.speed_ghost = make_label(ui.speed_panel, &revani_font_lcd, UI_COLOR_LCD_GHOST);
  lv_label_set_text(ui.speed_ghost, "888");
  ui.speed_value = make_label(ui.speed_panel, &revani_font_lcd, UI_COLOR_RETRO);
  lv_obj_align(ui.speed_value, LV_ALIGN_TOP_LEFT, 0, 0);
  set_speed_text("--");

  ui.speed_unit = make_label(parent, &lv_font_montserrat_20, UI_COLOR_RETRO_DIM);
  lv_label_set_text(ui.speed_unit, "km/h");
  lv_obj_set_style_bg_color(ui.speed_unit, color(UI_COLOR_BG), 0);
  lv_obj_set_style_radius(ui.speed_unit, 6, 0);
  lv_obj_set_style_pad_hor(ui.speed_unit, 6, 0);
  lv_obj_align(ui.speed_unit, LV_ALIGN_BOTTOM_MID, 0, -26);
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
  ui_arrow_set(ui.map_arrow, nav->maneuver, nav->roundabout_exit);

  char value[12];
  const char *unit;
  format_distance(nav->distance_m, value, sizeof(value), &unit);
  lv_label_set_text(ui.dist_value, value);
  lv_label_set_text(ui.dist_unit, unit);
  lv_label_set_text_fmt(ui.map_dist, "%s %s", value, unit);

  const char *street = nav->street ? nav->street : "";
  if (strcmp(lv_label_get_text(ui.street), street) != 0) {
    lv_label_set_text(ui.street, street);
    lv_label_set_text(ui.map_street, street);
  }
  if (street[0]) {
    lv_obj_remove_flag(ui.map_street, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(ui.map_street, LV_OBJ_FLAG_HIDDEN);
  }

  dist_ring_update(nav->maneuver != UI_MANEUVER_ARRIVE, nav->distance_m);
}

bool ui_nav_active(void) { return ui.nav_active; }

void ui_end_nav(void) {
  show_page(false);
  dist_ring_update(false, 0);
}

// ---------------------------------------------------------------- harita

static void set_map_mode(bool on) {
  if (on == ui.map_mode) return;
  ui.map_mode = on;
  if (on) {
    lv_obj_add_flag(ui.classic, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui.map_view, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_remove_flag(ui.classic, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.map_view, LV_OBJ_FLAG_HIDDEN);
  }
  // Hız haritanın üstüne biniyor: harita varken koyu zemin
  lv_opa_t opa = on ? LV_OPA_70 : LV_OPA_TRANSP;
  lv_obj_set_style_bg_opa(ui.speed_unit, opa, 0);
}

// Harita karesi UI_MAP_TIMEOUT_MS gelmezse büyük oklu görünüme dön.
static void map_watchdog_cb(lv_timer_t *t) {
  LV_UNUSED(t);
  set_map_mode(ui.map_ever && lv_tick_elaps(ui.map_last_tick) < UI_MAP_TIMEOUT_MS);
}

void ui_show_map(lv_draw_buf_t *buf) {
  lv_image_set_src(ui.map_img, buf);
  lv_image_set_scale(ui.map_img, (uint32_t)(UI_MAP_DIAMETER * 256 / buf->header.w));
  lv_obj_invalidate(ui.map_img);
  ui.map_ever = true;
  ui.map_last_tick = lv_tick_get();
  map_watchdog_cb(NULL);
}

// Konum işaretçisi: yukarı bakan sarı üçgen (harita yön yukarıda), merkezin biraz altında.
static lv_obj_t *make_position_marker(lv_obj_t *parent) {
  const int32_t s = 30;
  lv_obj_t *canvas = lv_canvas_create(parent);
  lv_canvas_set_draw_buf(canvas, lv_draw_buf_create(s, s, LV_COLOR_FORMAT_ARGB8888, 0));
  lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
  lv_layer_t layer;
  lv_canvas_init_layer(canvas, &layer);
  lv_draw_triangle_dsc_t t;
  lv_draw_triangle_dsc_init(&t);
  t.bg_opa = LV_OPA_COVER;
  for (int pass = 0; pass < 2; pass++) {  // önce siyah kontur, sonra sarı
    int32_t in = pass ? 4 : 0;
    t.bg_color = pass ? color(UI_COLOR_YELLOW) : color(UI_COLOR_BG);
    t.p[0].x = s / 2;
    t.p[0].y = in + (pass ? 2 : 0);
    t.p[1].x = in;
    t.p[1].y = s - 1 - in;
    t.p[2].x = s - 1 - in;
    t.p[2].y = s - 1 - in;
    lv_draw_triangle(&layer, &t);
  }
  lv_canvas_finish_layer(canvas, &layer);
  lv_obj_align(canvas, LV_ALIGN_CENTER, 0, UI_MAP_DIAMETER * UI_MAP_MARKER_OFFSET_PCT / 100);
  return canvas;
}

static void build_map_view(lv_obj_t *parent) {
  ui.map_view = make_page(parent);
  lv_obj_add_flag(ui.map_view, LV_OBJ_FLAG_HIDDEN);

  // Dairesel kırpılmış harita (2.2.3)
  lv_obj_t *circle = lv_obj_create(ui.map_view);
  lv_obj_remove_style_all(circle);
  lv_obj_set_size(circle, UI_MAP_DIAMETER, UI_MAP_DIAMETER);
  lv_obj_center(circle);
  lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(circle, true, 0);
  lv_obj_set_style_bg_color(circle, color(UI_COLOR_TRACK), 0);
  lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
  lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  ui.map_img = lv_image_create(circle);
  lv_obj_center(ui.map_img);
  make_position_marker(circle);

  // Üstte koyu kutu: küçük ok, mesafe, cadde (2.2.4)
  lv_obj_t *pill = lv_obj_create(ui.map_view);
  lv_obj_remove_style_all(pill);
  lv_obj_set_size(pill, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(pill, color(UI_COLOR_BG), 0);
  lv_obj_set_style_bg_opa(pill, LV_OPA_80, 0);
  lv_obj_set_style_radius(pill, 18, 0);
  lv_obj_set_style_pad_hor(pill, 14, 0);
  lv_obj_set_style_pad_ver(pill, 6, 0);
  lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 92);

  lv_obj_t *row = lv_obj_create(pill);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 10, 0);

  // Ok tuvali 160 px; küçültme yerleşimi etkilemediği için sabit boyutlu kutuya konur.
  const int32_t arrow_px = 64;
  lv_obj_t *arrow_box = lv_obj_create(row);
  lv_obj_remove_style_all(arrow_box);
  lv_obj_set_size(arrow_box, arrow_px, arrow_px);
  ui.map_arrow = ui_arrow_create(arrow_box);
  lv_obj_set_style_transform_scale(ui.map_arrow, arrow_px * 256 / UI_ARROW_SIZE, 0);
  lv_obj_set_style_transform_pivot_x(ui.map_arrow, 0, 0);
  lv_obj_set_style_transform_pivot_y(ui.map_arrow, 0, 0);

  ui.map_dist = make_label(row, &lv_font_montserrat_48, UI_COLOR_FG);

  ui.map_street = make_label(pill, &revani_font_30, UI_COLOR_YELLOW);
  lv_obj_set_width(ui.map_street, 250);
  lv_obj_set_style_text_align(ui.map_street, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(ui.map_street, LV_LABEL_LONG_DOT);
  lv_obj_set_height(ui.map_street, lv_font_get_line_height(&revani_font_30));

  lv_timer_create(map_watchdog_cb, 500, NULL);
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
  if (percent > 100) {  // bilinmiyor (ölçüm yok): pil hiç gösterilmez
    lv_obj_add_flag(ui.battery_row, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_remove_flag(ui.battery_row, LV_OBJ_FLAG_HIDDEN);
  int32_t inner = BATTERY_W - 2 * BATTERY_BORDER - 4;
  lv_obj_set_width(ui.battery_fill, LV_MAX(2, inner * percent / 100));
  uint32_t c = percent <= 20 ? UI_COLOR_YELLOW : UI_COLOR_FG;
  lv_obj_set_style_bg_color(ui.battery_fill, color(c), 0);
  lv_label_set_text_fmt(ui.battery_label, "%%%u", percent);  // Türkçede yüzde işareti önde
}

static void build_battery(lv_obj_t *parent) {
  lv_obj_t *row = lv_obj_create(parent);
  ui.battery_row = row;
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
  ui.classic = make_page(ui.nav_page);
  ui.arrow = ui_arrow_create(ui.classic);
  lv_obj_align(ui.arrow, LV_ALIGN_TOP_MID, 0, 62);

  lv_obj_t *dist_row = lv_obj_create(ui.classic);
  lv_obj_remove_style_all(dist_row);
  lv_obj_set_size(dist_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(dist_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(dist_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_column(dist_row, 8, 0);
  lv_obj_align(dist_row, LV_ALIGN_CENTER, 0, 30);
  ui.dist_value = make_label(dist_row, &revani_font_72, UI_COLOR_FG);
  ui.dist_unit = make_label(dist_row, &revani_font_30, UI_COLOR_FG);
  lv_obj_set_style_pad_bottom(ui.dist_unit, 14, 0);  // rakamların taban çizgisine hizala

  ui.street = make_label(ui.classic, &revani_font_30, UI_COLOR_YELLOW);
  lv_obj_set_width(ui.street, 270);  // iç halkanın içinde kalsın
  lv_obj_set_style_text_align(ui.street, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(ui.street, LV_LABEL_LONG_DOT);
  lv_obj_set_height(ui.street, lv_font_get_line_height(&revani_font_30));  // tek satır
  lv_obj_align(ui.street, LV_ALIGN_CENTER, 0, 92);

  build_map_view(ui.nav_page);

  // Halkalar ve hız her iki sayfada da görünür
  build_dial(scr);
#if UI_INNER_RING_ENABLED
  ui.dist_ring = make_ring(scr, UI_INNER_RING_DIAMETER, UI_INNER_RING_WIDTH, UI_INNER_RING_RANGE_M);
  lv_obj_set_style_arc_color(ui.dist_ring, color(UI_COLOR_TRACK), LV_PART_MAIN);
  lv_obj_add_flag(ui.dist_ring, LV_OBJ_FLAG_HIDDEN);
#endif

  build_speed_panel(scr);

  ui.gps_lost = true;
  apply_gps_style();
  lv_timer_create(gps_watchdog_cb, 250, NULL);

  ui_set_battery(0xFF);  // ölçülene kadar gizli
  ui_set_connected(false);
  show_page(false);
}
