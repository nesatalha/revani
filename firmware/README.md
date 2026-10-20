# firmware

ESP32-S3 gidon ekranı (Waveshare ESP32-S3-LCD-2.8C) ve LVGL simülatörü.

```
firmware/
├── platformio.ini   # kart derlemesi
├── config/lv_conf.h # LVGL ayarları (kart + simülatör)
├── src/             # karta özel kod (Arduino, NimBLE: ble_link.*)
├── protocol/        # BLE mesaj ayrıştırıcı (kart + simülatör + test)
├── ui/              # arayüz, kart ve simülatörde ortak
│   ├── ui.h         # BLE'nin çağırdığı API: ui_set_nav, ui_set_gps, ...
│   ├── ui_config.h  # renkler, eşikler, UI_INNER_RING_ENABLED
│   ├── ui_arrows.*  # dönüş okları
│   ├── ui_fake.*    # sahte veri üreteci
│   ├── ui_proto.*   # BLE mesajı → ui_* çağrıları, saat
│   └── fonts/       # Türkçe karakterli fontlar (generate.sh)
└── simulator/       # macOS SDL2 simülatörü
```

## Simülatör

Gerekenler: `brew install sdl2 cmake`

```bash
cmake -S simulator -B simulator/build
cmake --build simulator/build -j
simulator/build/revani_sim            # pencerede sahte veriyle
simulator/build/revani_sim --speed 4  # dört kat hızlı
```

Penceresiz ekran görüntüsü (BMP):

```bash
simulator/build/revani_sim --shot nav.bmp --scene nav
```

Sahneler: `idle`, `connected`, `nav`, `near`, `turn`, `far`, `gps-lost`, `arrows`, `fake` (`--ms` ile sahte veri süresi), `ble` ve `ble-end` (Android'in göndereceği ham paketler ayrıştırıcıdan geçer).

Protokol testleri (Kotlin `ProtocolTest` ile aynı bayt örnekleri):

```bash
ctest --test-dir simulator/build --output-on-failure
```

## Kart

```bash
pio run              # derle
pio run -t upload    # yükle (kart gelince)
```

Ekran sürücüsü (ST7701) adım 1.2.2'de eklenecek. BLE servisi hazır: `Revani` adıyla yayın yapar, gelen mesajlar kuyruktan ana döngüde işlenir.

## Fontlar

`ui/fonts/generate.sh` Montserrat'ı LVGL paketinden alıp `lv_font_conv` ile üretir. Önce `pio pkg install` gerekir.
