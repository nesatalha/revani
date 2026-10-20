#include "ui_fake.h"

#include <lvgl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ui.h"

typedef struct {
  uint8_t maneuver;
  uint8_t exit;
  uint16_t length_m;
  const char *street;
} step_t;

// Tüm manevra tiplerinden geçen örnek rota.
static const step_t kRoute[] = {
    {UI_MANEUVER_RIGHT, 0, 620, "Bağdat Caddesi"},
    {UI_MANEUVER_SLIGHT_LEFT, 0, 380, "Ethem Efendi Caddesi"},
    {UI_MANEUVER_ROUNDABOUT, 2, 450, "Kozyatağı Kavşağı"},
    {UI_MANEUVER_STRAIGHT, 0, 1800, "D-100 Karayolu"},
    {UI_MANEUVER_SLIGHT_RIGHT, 0, 300, "Söğütlüçeşme Caddesi"},
    {UI_MANEUVER_LEFT, 0, 520, "Kadıköy İskele Sokağı"},
    {UI_MANEUVER_SHARP_RIGHT, 0, 260, "Moda Caddesi"},
    {UI_MANEUVER_SHARP_LEFT, 0, 240, "Şair Nefi Sokak"},
    {UI_MANEUVER_UTURN, 0, 180, "Moda Caddesi"},
    {UI_MANEUVER_ARRIVE, 0, 350, "Moda Sahili"},
};
#define ROUTE_LEN ((int)(sizeof(kRoute) / sizeof(kRoute[0])))

typedef enum { PH_DISCONNECTED, PH_CONNECTED, PH_NAV, PH_ARRIVED } phase_t;

static struct {
  lv_timer_t *timer;
  phase_t phase;
  int phase_s;   // aşamada geçen saniye
  int step;
  float dist_m;
  float t_s;     // toplam sahte süre
  float speed;
} f;

static void send_time(void) {
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  ui_set_time((uint8_t)tm.tm_hour, (uint8_t)tm.tm_min);
}

static void send_nav(void) {
  ui_nav_t nav = {
      .maneuver = kRoute[f.step].maneuver,
      .roundabout_exit = kRoute[f.step].exit,
      .distance_m = 0,
      .eta_min = 12,
      .street = kRoute[f.step].street,
  };
  // Google Maps gibi yuvarla: 100 m altı 5'e, üstü 10'a
  int d = (int)f.dist_m;
  d = d < 100 ? (d + 2) / 5 * 5 : (d + 5) / 10 * 10;
  nav.distance_m = (uint16_t)(d < 0 ? 0 : d);
  ui_set_nav(&nav);
}

static void set_phase(phase_t p) {
  f.phase = p;
  f.phase_s = 0;
}

// Her sahte saniyede bir: telefonun 1 Hz GPS ve bildirim akışını taklit eder.
static void tick_cb(lv_timer_t *t) {
  LV_UNUSED(t);
  f.t_s += 1;
  f.phase_s += 1;
  send_time();

  // Hız: yavaş dalgalanma + titreşim, ara ara trafikte durma (10 km/h altı)
  float wave = 45 + 30 * sinf(f.t_s / 9.0f) + 12 * sinf(f.t_s / 2.3f);
  float jitter = (float)(rand() % 7 - 3);
  f.speed = wave + jitter;
  if (fmodf(f.t_s, 40) > 34) f.speed = (float)(rand() % 8);
  if (f.speed < 0) f.speed = 0;
  if (f.speed > 120) f.speed = 120;

  switch (f.phase) {
    case PH_DISCONNECTED:
      if (f.phase_s >= 4) {
        ui_set_connected(true);
        set_phase(PH_CONNECTED);
      }
      break;

    case PH_CONNECTED:
      ui_set_gps((uint8_t)f.speed, 0);
      if (f.phase_s >= 3) {
        f.step = 0;
        f.dist_m = kRoute[0].length_m;
        set_phase(PH_NAV);
        send_nav();
      }
      break;

    case PH_NAV: {
      // Dördüncü adımda 5 sn "tünel": GPS gelmez, halka grileşir.
      bool tunnel = f.step == 3 && f.phase_s > 10 && f.phase_s <= 15;
      if (!tunnel) ui_set_gps((uint8_t)f.speed, 0);

      // Masada izlerken adımlar çok uzamasın diye mesafe hızdan biraz hızlı azalır.
      float v = f.speed < 15 ? 15 : f.speed;
      f.dist_m -= v / 3.6f * 3.0f;
      if (f.dist_m <= 0) {
        if (kRoute[f.step].maneuver == UI_MANEUVER_ARRIVE) {
          f.dist_m = 0;
          send_nav();
          set_phase(PH_ARRIVED);
          break;
        }
        f.step++;
        f.dist_m = kRoute[f.step].length_m;
        f.phase_s = 0;
      }
      send_nav();
      break;
    }

    case PH_ARRIVED:
      ui_set_gps(0, 0);
      if (f.phase_s >= 4) {
        ui_end_nav();
        set_phase(PH_CONNECTED);
        f.phase_s = -6;  // biraz beklemede kalsın
      }
      break;
  }
}

void ui_fake_start(int speedup) {
  if (speedup < 1) speedup = 1;
  ui_fake_stop();
  memset(&f, 0, sizeof(f));
  srand(42);
  send_time();
  ui_set_battery(76);
  ui_set_connected(false);
  set_phase(PH_DISCONNECTED);
  f.timer = lv_timer_create(tick_cb, 1000 / speedup, NULL);
}

void ui_fake_stop(void) {
  if (f.timer) lv_timer_delete(f.timer);
  f.timer = NULL;
}
