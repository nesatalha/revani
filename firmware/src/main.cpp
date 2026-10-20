// Revani — ESP32-S3 ana programı.
//
// Şimdilik derlenebilir iskelet: LVGL ve arayüz başlatılıyor, ekran sürücüsü
// (ST7701, adım 1.2.2) ve BLE servisi (adım 1.3.1) henüz yok.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <lvgl.h>

#include "ble_uuids.h"
#include "ui.h"

static constexpr uint32_t kScreenSize = 480;
static constexpr uint32_t kBufLines = 48;

static uint32_t tick_cb() { return millis(); }

// Geçici: ST7701 sürücüsü gelene kadar çizilen alanı hiçbir yere göndermiyor.
static void flush_cb(lv_display_t *disp, const lv_area_t *, uint8_t *) {
  lv_display_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);

  lv_init();
  lv_tick_set_cb(tick_cb);

  lv_display_t *disp = lv_display_create(kScreenSize, kScreenSize);
  size_t buf_size = kScreenSize * kBufLines * sizeof(lv_color16_t);
  void *buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  lv_display_set_buffers(disp, buf, nullptr, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, flush_cb);

  ui_init();

  NimBLEDevice::init("Revani");
  Serial.printf("Revani hazir, servis %s\n", REVANI_BLE_SERVICE_UUID);
}

void loop() {
  uint32_t wait_ms = lv_timer_handler();
  delay(wait_ms < 5 ? wait_ms : 5);
}
