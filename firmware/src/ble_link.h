// NimBLE peripheral: Revani servisi, RX (yazma) ve TX (bildirim) karakteristikleri.
// NimBLE görevinden gelen her şey kuyruğa girer; LVGL'i ana döngü günceller (1.3.3).
#pragma once

#include <stddef.h>
#include <stdint.h>

enum class BleEventKind : uint8_t { Connected, Disconnected, Subscribed, Data };

struct BleEvent {
  BleEventKind kind;
  uint16_t len;
  uint8_t data[512];  // en uzun: MAP_CHUNK 7 + 480 bayt
};

void ble_link_begin(const char *name);
// Kuyruktan bir olay alır, yoksa false. Ana döngüden çağrılır.
bool ble_link_poll(BleEvent *ev);
bool ble_link_connected();
// TX karakteristiğinden bildirim gönderir (abone yoksa sessizce düşer).
void ble_link_notify(const uint8_t *data, size_t len);
