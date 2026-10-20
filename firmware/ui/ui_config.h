// Arayüz ayarları: renkler, ölçüler ve davranış eşikleri.
#pragma once

// İç mesafe halkası. Adım 1.6.5'te kaldırılabilir: 0 yapınca hiç oluşturulmaz.
#ifndef UI_INNER_RING_ENABLED
#define UI_INNER_RING_ENABLED 1
#endif

// Ekran
#define UI_SCREEN_SIZE 480
#define UI_EDGE_MARGIN 20  // yuvarlak ekranda kesilme riski olan kenar

// Renkler (siyah zemin, beyaz ve sarı yazı)
#define UI_COLOR_BG 0x000000
#define UI_COLOR_FG 0xFFFFFF
#define UI_COLOR_YELLOW 0xFFD000
#define UI_COLOR_GREEN 0x30D158
#define UI_COLOR_DIM 0x5A5A5A
#define UI_COLOR_TRACK 0x1C1C1C

// Retro hız göstergesi (analog kadran + LCD rakamlar)
#define UI_COLOR_RETRO 0xF2E3C2      // eski kadranların krem rengi
#define UI_COLOR_RETRO_DIM 0x8C826C
#define UI_COLOR_NEEDLE 0xFF1E1E     // canlı kırmızı ibre
#define UI_COLOR_LCD_BG 0x0B0B08
#define UI_COLOR_LCD_GHOST 0x24221A  // sönük segmentler

// Dış halka: GPS hızı
#define UI_SPEED_MAX_KMH 120
#define UI_SPEED_MIN_KMH 10        // altı titrer, halka 0'da kalır
#define UI_SPEED_ANIM_MS 1000      // GPS ~1 Hz; iki değer arası doğrusal dolar
#define UI_GPS_TIMEOUT_MS 3000     // bu süre GPS gelmezse halka gri
#define UI_NEEDLE_OUTER_R 224       // kama ibre: dış kenar (eğri) bu yarıçapta
#define UI_NEEDLE_INNER_R 182       // sivri uç
#define UI_NEEDLE_HALF_DEG 5.0f     // başın yarı açısı (~39 px genişlik)
#define UI_NEEDLE_HEAD 10           // kadran eğrisini izleyen baş bandının kalınlığı

// İç halka: dönüşe kalan mesafe
#define UI_INNER_RING_RANGE_M 500  // son 500 m'de dolar, daha uzaktaysa gizli
#define UI_INNER_RING_WARN_M 100   // sarıya döner
#define UI_INNER_RING_TURN_M 15    // dönüş anı: yeşil, yanıp söner
#define UI_INNER_RING_WIDTH 8
#define UI_INNER_RING_DIAMETER 344  // kadran sayılarının içinde
#define UI_INNER_RING_ANIM_MS 600

// Harita (Faz 2): telefondan gelen kare bu çapa büyütülüp daire içinde gösterilir
#define UI_MAP_DIAMETER 328
#define UI_MAP_TIMEOUT_MS 5000
// Konum işaretçisi merkezin bu kadar (harita çapının %'si) altında: önümüzdeki yol daha
// çok görünsün. Telefon kamerayı aynı oranda ileri kaydırır (MapMath.MARKER_OFFSET_FRAC).
#define UI_MAP_MARKER_OFFSET_PCT 8  // alttaki grubun hemen üstü
#define UI_MAP_BOTTOM_BOX_W 250  // alttaki kutu (dial sayıları 0 ve 120'ye değmesin)
#define UI_MAP_OVERLAY_OPA LV_OPA_50     // ok/mesafe kutusunun saydamlığı
#define UI_MAP_LAYOUT_DEFAULT UI_MAP_LAYOUT_BOTTOM  // bu süre kare gelmezse büyük oklu görünüme dön

// Halkaların açısı: alttan 90° boşluk, saat yönünde 270°
#define UI_RING_START_ANGLE 135
#define UI_RING_END_ANGLE 45
#define UI_RING_SWEEP 270
