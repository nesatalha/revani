// USB köprüsü (adım 1.7): telefondaki uygulama `adb reverse tcp:7878 tcp:7878`
// üzerinden TCP ile bağlanır. Çerçeve: [u16 uzunluk LE][mesaj]. Mesajlar karttakiyle
// aynı ayrıştırıcıdan (protocol/) ve arayüzden (ui/) geçer.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BRIDGE_DEFAULT_PORT 7878

bool bridge_start(uint16_t port);
// LVGL döngüsünden çağrılır: bağlantı kabul eder, gelen çerçeveleri işler, STATUS gönderir.
void bridge_poll(void);
