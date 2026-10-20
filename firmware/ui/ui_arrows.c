#include "ui_arrows.h"

#include <math.h>

#include "ui.h"
#include "ui_config.h"

#define S UI_ARROW_SIZE
#define LINE_W 22
#define HEAD_LEN 40
#define HEAD_HALF_W 30

typedef struct {
  lv_obj_t *canvas;
  lv_obj_t *exit_label;
  int16_t maneuver;
  int16_t exit;
} arrow_t;

typedef struct {
  int32_t x, y;
} pt_t;

static void draw_line(lv_layer_t *layer, pt_t a, pt_t b, lv_color_t color, int32_t width) {
  lv_draw_line_dsc_t d;
  lv_draw_line_dsc_init(&d);
  d.color = color;
  d.width = width;
  d.round_start = 1;
  d.round_end = 1;
  d.p1.x = a.x;
  d.p1.y = a.y;
  d.p2.x = b.x;
  d.p2.y = b.y;
  lv_draw_line(layer, &d);
}

static void draw_circle(lv_layer_t *layer, pt_t c, int32_t r, lv_color_t color) {
  lv_draw_rect_dsc_t d;
  lv_draw_rect_dsc_init(&d);
  d.bg_color = color;
  d.bg_opa = LV_OPA_COVER;
  d.radius = LV_RADIUS_CIRCLE;
  lv_area_t a = {c.x - r, c.y - r, c.x + r, c.y + r};
  lv_draw_rect(layer, &d, &a);
}

// Çoklu çizgi + son noktada, son parçanın yönünde dolu ok başı.
static void draw_path(lv_layer_t *layer, const pt_t *p, int n, int32_t head_len) {
  lv_color_t fg = lv_color_hex(UI_COLOR_FG);
  for (int i = 0; i + 1 < n; i++) draw_line(layer, p[i], p[i + 1], fg, LINE_W);

  float dx = (float)(p[n - 1].x - p[n - 2].x);
  float dy = (float)(p[n - 1].y - p[n - 2].y);
  float len = sqrtf(dx * dx + dy * dy);
  dx /= len;
  dy /= len;

  lv_draw_triangle_dsc_t t;
  lv_draw_triangle_dsc_init(&t);
  t.bg_color = fg;
  t.bg_opa = LV_OPA_COVER;
  pt_t e = p[n - 1];
  t.p[0].x = e.x + (int32_t)lroundf(dx * head_len);
  t.p[0].y = e.y + (int32_t)lroundf(dy * head_len);
  t.p[1].x = e.x + (int32_t)lroundf(-dy * HEAD_HALF_W);
  t.p[1].y = e.y + (int32_t)lroundf(dx * HEAD_HALF_W);
  t.p[2].x = e.x + (int32_t)lroundf(dy * HEAD_HALF_W);
  t.p[2].y = e.y + (int32_t)lroundf(-dx * HEAD_HALF_W);
  lv_draw_triangle(layer, &t);
}

// Çeyrek/yarım daire yayını nokta dizisine çevirir (açılar derece, ekran koordinatı).
static int arc_points(pt_t *out, pt_t c, int32_t r, float from_deg, float to_deg, int steps) {
  for (int i = 0; i <= steps; i++) {
    float a = (from_deg + (to_deg - from_deg) * i / steps) * (float)M_PI / 180.0f;
    out[i].x = c.x + (int32_t)lroundf(cosf(a) * r);
    out[i].y = c.y + (int32_t)lroundf(sinf(a) * r);
  }
  return steps + 1;
}

static void mirror(pt_t *p, int n) {
  for (int i = 0; i < n; i++) p[i].x = S - p[i].x;
}

// Sağa dönüşlerin yolları; sol dönüşler bunların aynası.
static int right_path(uint8_t maneuver, pt_t *p) {
  int n = 0;
  switch (maneuver) {
    case UI_MANEUVER_SLIGHT_RIGHT:
    case UI_MANEUVER_SLIGHT_LEFT:
      p[n++] = (pt_t){62, 150};
      p[n++] = (pt_t){62, 106};
      p[n++] = (pt_t){108, 60};
      break;
    case UI_MANEUVER_RIGHT:
    case UI_MANEUVER_LEFT:
      p[n++] = (pt_t){50, 150};
      n += arc_points(&p[n], (pt_t){80, 98}, 30, 180, 270, 8);
      p[n++] = (pt_t){108, 68};
      break;
    case UI_MANEUVER_SHARP_RIGHT:
    case UI_MANEUVER_SHARP_LEFT:
      p[n++] = (pt_t){52, 150};
      n += arc_points(&p[n], (pt_t){74, 66}, 22, 180, 315, 8);
      p[n++] = (pt_t){120, 80};
      break;
    default:
      break;
  }
  return n;
}

static void redraw(arrow_t *ar) {
  lv_obj_t *canvas = ar->canvas;
  lv_canvas_fill_bg(canvas, lv_color_hex(UI_COLOR_BG), LV_OPA_TRANSP);

  lv_layer_t layer;
  lv_canvas_init_layer(canvas, &layer);

  pt_t p[24];
  int n = 0;
  uint8_t m = (uint8_t)ar->maneuver;
  lv_obj_add_flag(ar->exit_label, LV_OBJ_FLAG_HIDDEN);

  switch (m) {
    case UI_MANEUVER_STRAIGHT:
      p[n++] = (pt_t){80, 150};
      p[n++] = (pt_t){80, 58};
      draw_path(&layer, p, n, HEAD_LEN);
      break;

    case UI_MANEUVER_SLIGHT_RIGHT:
    case UI_MANEUVER_RIGHT:
    case UI_MANEUVER_SHARP_RIGHT:
    case UI_MANEUVER_SLIGHT_LEFT:
    case UI_MANEUVER_LEFT:
    case UI_MANEUVER_SHARP_LEFT:
      n = right_path(m, p);
      if (m >= UI_MANEUVER_SLIGHT_LEFT) mirror(p, n);
      draw_path(&layer, p, n, HEAD_LEN);
      break;

    case UI_MANEUVER_UTURN:  // sağdan trafikte U dönüşü sola doğru
      p[n++] = (pt_t){110, 150};
      n += arc_points(&p[n], (pt_t){80, 72}, 30, 0, -180, 12);
      p[n++] = (pt_t){50, 92};
      draw_path(&layer, p, n, HEAD_LEN);
      break;

    case UI_MANEUVER_ROUNDABOUT: {
      lv_draw_arc_dsc_t a;
      lv_draw_arc_dsc_init(&a);
      a.color = lv_color_hex(UI_COLOR_DIM);
      a.width = 12;
      a.center.x = 80;
      a.center.y = 78;
      a.radius = 44;
      a.start_angle = 0;
      a.end_angle = 360;
      lv_draw_arc(&layer, &a);
      // Giriş ve çıkış çizgileri halkanın üstüne biner, böylece kopuk durmaz.
      draw_line(&layer, (pt_t){80, 152}, (pt_t){80, 120}, lv_color_hex(UI_COLOR_FG), LINE_W);
      p[n++] = (pt_t){106, 52};
      p[n++] = (pt_t){116, 42};
      draw_path(&layer, p, n, 30);
      if (ar->exit > 0) {
        lv_label_set_text_fmt(ar->exit_label, "%d", ar->exit);
        lv_obj_align(ar->exit_label, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_update_layout(ar->exit_label);
        int32_t w = lv_obj_get_width(ar->exit_label);
        int32_t h = lv_obj_get_height(ar->exit_label);
        lv_obj_set_pos(ar->exit_label, 80 - w / 2, 78 - h / 2);
        lv_obj_remove_flag(ar->exit_label, LV_OBJ_FLAG_HIDDEN);
      }
      break;
    }

    case UI_MANEUVER_ARRIVE: {  // konum iğnesi
      lv_draw_triangle_dsc_t t;
      lv_draw_triangle_dsc_init(&t);
      t.bg_color = lv_color_hex(UI_COLOR_FG);
      t.bg_opa = LV_OPA_COVER;
      t.p[0].x = 44;
      t.p[0].y = 82;
      t.p[1].x = 116;
      t.p[1].y = 82;
      t.p[2].x = 80;
      t.p[2].y = 150;
      lv_draw_triangle(&layer, &t);
      draw_circle(&layer, (pt_t){80, 62}, 42, lv_color_hex(UI_COLOR_FG));
      draw_circle(&layer, (pt_t){80, 62}, 16, lv_color_hex(UI_COLOR_BG));
      break;
    }

    default:
      break;
  }

  lv_canvas_finish_layer(canvas, &layer);
}

static void delete_cb(lv_event_t *e) {
  arrow_t *ar = lv_event_get_user_data(e);
  lv_draw_buf_t *buf = lv_canvas_get_draw_buf(ar->canvas);
  if (buf) lv_draw_buf_destroy(buf);
  lv_free(ar);
}

lv_obj_t *ui_arrow_create(lv_obj_t *parent) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, S, S);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  arrow_t *ar = lv_malloc_zeroed(sizeof(arrow_t));
  ar->maneuver = -1;
  ar->canvas = lv_canvas_create(box);
  lv_canvas_set_draw_buf(ar->canvas, lv_draw_buf_create(S, S, LV_COLOR_FORMAT_ARGB8888, 0));

  ar->exit_label = lv_label_create(box);
  lv_obj_set_style_text_font(ar->exit_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(ar->exit_label, lv_color_hex(UI_COLOR_FG), 0);
  lv_obj_add_flag(ar->exit_label, LV_OBJ_FLAG_HIDDEN);

  lv_obj_set_user_data(box, ar);
  lv_obj_add_event_cb(box, delete_cb, LV_EVENT_DELETE, ar);
  return box;
}

void ui_arrow_set(lv_obj_t *arrow, uint8_t maneuver, uint8_t roundabout_exit) {
  arrow_t *ar = lv_obj_get_user_data(arrow);
  if (ar->maneuver == maneuver && ar->exit == roundabout_exit) return;
  ar->maneuver = maneuver;
  ar->exit = roundabout_exit;
  redraw(ar);
}
