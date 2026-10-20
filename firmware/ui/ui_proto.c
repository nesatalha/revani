#include "ui_proto.h"

#include <lvgl.h>

#include "ui.h"
#include "ui_map.h"

static struct {
  bool time_valid;
  int64_t local_base_s;  // TIME ile gelen yerel saat (unix + UTC farkı)
  uint32_t tick_base;
  int last_minute;
} s;

static void on_nav(const proto_nav_t *n, void *ctx) {
  LV_UNUSED(ctx);
  ui_nav_t nav = {
      .maneuver = n->maneuver < UI_MANEUVER_COUNT ? n->maneuver : UI_MANEUVER_STRAIGHT,
      .roundabout_exit = n->roundabout_exit,
      .distance_m = n->distance_m,
      .eta_min = n->eta_min,
      .street = n->street,
  };
  ui_set_nav(&nav);
}

static void on_nav_end(void *ctx) {
  LV_UNUSED(ctx);
  ui_end_nav();
}

static void on_gps(uint8_t speed, uint16_t heading, void *ctx) {
  LV_UNUSED(ctx);
  ui_set_gps(speed, heading);
}

static void clock_cb(lv_timer_t *t) {
  LV_UNUSED(t);
  if (!s.time_valid) return;
  int64_t now = s.local_base_s + lv_tick_elaps(s.tick_base) / 1000;
  int minute = (int)((now / 60) % (24 * 60));
  if (minute == s.last_minute) return;
  s.last_minute = minute;
  ui_set_time((uint8_t)(minute / 60), (uint8_t)(minute % 60));
}

static void on_time(uint32_t unix_time, int16_t offset_min, void *ctx) {
  LV_UNUSED(ctx);
  s.local_base_s = (int64_t)unix_time + offset_min * 60;
  s.tick_base = lv_tick_get();
  s.time_valid = true;
  s.last_minute = -1;
  clock_cb(NULL);
}

static void on_map_chunk(uint16_t frame, uint16_t chunk, uint16_t count, const uint8_t *data, size_t len,
                         void *ctx) {
  LV_UNUSED(ctx);
  ui_map_chunk(frame, chunk, count, data, len);
}

static const proto_handlers_t handlers = {on_nav, on_nav_end, on_gps, on_time, on_map_chunk, NULL};

void ui_proto_init(void) {
  lv_timer_create(clock_cb, 1000, NULL);
}

proto_result_t ui_proto_handle(const uint8_t *data, size_t len) {
  return proto_parse(data, len, &handlers);
}

uint8_t ui_proto_screen(void) { return ui_nav_active() ? 1 : 0; }
