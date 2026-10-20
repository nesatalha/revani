// BLE mesajlarını arayüze bağlar: protocol.c ayrıştırır, burası ui_* çağırır.
// Kart ve simülatör aynı yolu kullanır. LVGL görevinden çağrılmalı.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_proto_init(void);
proto_result_t ui_proto_handle(const uint8_t *data, size_t len);
// STATUS için ekran numarası: 0 bekleme, 1 navigasyon
uint8_t ui_proto_screen(void);

#ifdef __cplusplus
}
#endif
