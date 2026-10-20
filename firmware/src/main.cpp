// Revani — ESP32-S3 ana programı.
//
// BLE olayları kuyruktan ana döngüde işlenir, LVGL yalnız buradan güncellenir.
// Ekran sürücüsü (ST7701) henüz yok: adım 1.2.2.

#include <Arduino.h>
#include <lvgl.h>

#include "ble_link.h"
#include "protocol.h"
#include "ui.h"
#include "ui_proto.h"

static constexpr uint32_t kScreenSize = 480;
static constexpr uint32_t kBufLines = 48;
static constexpr uint32_t kStatusPeriodMs = 10000;

static uint32_t tick_cb() { return millis(); }

// Geçici: ST7701 sürücüsü gelene kadar çizilen alanı hiçbir yere göndermiyor.
static void flush_cb(lv_display_t *disp, const lv_area_t *, uint8_t *) {
  lv_display_flush_ready(disp);
}

// Pil ölçümü kart gelince (pil konnektörü ve ADC pini, adım 3.1).
static uint8_t battery_percent() { return PROTO_BATTERY_UNKNOWN; }

static void send_status() {
  uint8_t msg[3];
  size_t len = proto_encode_status(msg, battery_percent(), ui_proto_screen());
  ble_link_notify(msg, len);
}

static void handle_ble_events() {
  static BleEvent ev;
  while (ble_link_poll(&ev)) {
    switch (ev.kind) {
      case BleEventKind::Connected:
        ui_set_connected(true);
        break;
      case BleEventKind::Disconnected:
        ui_set_connected(false);  // navigasyon da biter, bekleme ekranı (1.3.5)
        break;
      case BleEventKind::Subscribed:
        send_status();
        break;
      case BleEventKind::Data: {
        proto_result_t r = ui_proto_handle(ev.data, ev.len);
        if (r != PROTO_OK) Serial.printf("Mesaj 0x%02x: %s\n", ev.len ? ev.data[0] : 0, proto_result_str(r));
        break;
      }
    }
  }
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
  ui_set_battery(battery_percent());
  ui_proto_init();
  ble_link_begin("Revani");
  Serial.println("Revani hazir");
}

void loop() {
  handle_ble_events();

  static uint32_t last_status = 0;
  if (ble_link_connected() && millis() - last_status > kStatusPeriodMs) {
    last_status = millis();
    send_status();
  }

  uint32_t wait_ms = lv_timer_handler();
  delay(wait_ms < 5 ? wait_ms : 5);
}
