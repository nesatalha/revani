// Revani ekran arayüzü. Kart (src/) ve simülatör (simulator/) aynı API'yi
// kullanır. Tüm fonksiyonlar LVGL'in çalıştığı görevden çağrılmalı.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Manevra kodları, BLE protokolüyle aynı (docs/PLAN.md bölüm 4).
typedef enum {
  UI_MANEUVER_STRAIGHT = 0,
  UI_MANEUVER_SLIGHT_RIGHT = 1,
  UI_MANEUVER_RIGHT = 2,
  UI_MANEUVER_SHARP_RIGHT = 3,
  UI_MANEUVER_UTURN = 4,
  UI_MANEUVER_SLIGHT_LEFT = 5,
  UI_MANEUVER_LEFT = 6,
  UI_MANEUVER_SHARP_LEFT = 7,
  UI_MANEUVER_ROUNDABOUT = 8,
  UI_MANEUVER_ARRIVE = 9,
  UI_MANEUVER_COUNT
} ui_maneuver_t;

typedef struct {
  uint8_t maneuver;         // ui_maneuver_t
  uint8_t roundabout_exit;  // dönel kavşakta çıkış no, yoksa 0
  uint16_t distance_m;      // dönüşe kalan mesafe
  uint16_t eta_min;         // varışa kalan dakika
  const char *street;       // UTF-8, NULL olabilir
} ui_nav_t;

void ui_init(void);

// NAV mesajı: navigasyon ekranına geçer ve günceller.
void ui_set_nav(const ui_nav_t *nav);
// NAV_END: bekleme ekranına döner.
void ui_end_nav(void);
// GPS mesajı (~1 Hz). UI_GPS_TIMEOUT_MS boyunca gelmezse halka grileşir.
void ui_set_gps(uint8_t speed_kmh, uint16_t heading_deg);
void ui_set_time(uint8_t hour, uint8_t minute);
void ui_set_battery(uint8_t percent);
// Bağlantı koparsa navigasyon da biter.
void ui_set_connected(bool connected);

#ifdef __cplusplus
}
#endif
