# Moto Dash: Navigasyon Öncelikli Yol Haritası

> Alternatif plan. Ana plan (`moto-dash-plan.md`) değişmedi.
> Fikir: önce navigasyonu bitirip motorda kullanılabilir bir ürün çıkarmak, OBD ve devir göstergesini sonraya bırakmak.
> Oluşturulma: 20 Ekim 2026. Fiyatlar bu tarihte kontrol edildi.

---

## İçindekiler

1. [Neden bu sıra?](#1-neden-bu-sıra)
2. [Bu planda sistem nasıl çalışır?](#2-bu-planda-sistem-nasıl-çalışır)
3. [Ekran tasarımı](#3-ekran-tasarımı)
4. [Faz 0: Hazırlık](#faz-0-hazırlık)
5. [Faz 1: Navigasyon MVP (masada)](#faz-1-navigasyon-mvp-masada)
6. [Faz 2: Ortaya harita](#faz-2-ortaya-harita)
7. [Faz 3: Motora takma](#faz-3-motora-takma)
8. [Faz 4: Müzik ve arama (isteğe bağlı)](#faz-4-müzik-ve-arama-isteğe-bağlı)
9. [Faz 5: OBD ve devir göstergesi](#faz-5-obd-ve-devir-göstergesi)
10. [Faz faz alışveriş listesi](#faz-faz-alışveriş-listesi)
11. [Özet takvim ve bütçe](#özet-takvim-ve-bütçe)
12. [Riskler](#riskler)
13. [Açık sorular](#açık-sorular)

---

## 1. Neden bu sıra?

| Sebep | Açıklama |
|---|---|
| **Navigasyon tek başına işe yarıyor** | İlk ayın sonunda gidonda gerçekten kullanılan bir cihaz olur. |
| **Belirsizlikler OBD tarafında** | Soket tipi, CAN olup olmadığı, hangi kablonun alınacağı henüz belli değil. Bunlar netleşmeden para harcanmaz. |
| **Başlangıç maliyeti düşük** | Başlamak için tek bir kart yeterli. |
| **Hiçbir iş boşa gitmez** | Ekran, BLE bağlantısı ve Android uygulaması OBD eklendiğinde aynen kalır. Sadece yeni bir veri kaynağı eklenir. |

---

## 2. Bu planda sistem nasıl çalışır?

```
┌──────────────────────────────┐   BLE    ┌──────────────────────────┐
│ Android (cepte)              │ ───────▶ │ ESP32-S3 2.8" (gidonda)  │
│                              │          │                          │
│ Google Maps navigasyonu      │          │ dış halka: dönüşe kalan  │
│   └─ bildirimini okur        │          │            mesafe        │
│ GPS → hız, yön               │          │ orta: harita (Faz 2)     │
│ MapLibre → harita görüntüsü  │          │       ok, mesafe, hız    │
│ Müzik, arama (Faz 4)         │ ◀─────── │ butonlar (Faz 3)         │
└──────────────────────────────┘  buton   └──────────────────────────┘
```

**Adım adım bir sürüş:**

1. Telefonda **Google Maps**'i açıp navigasyonu başlatırsın, telefonu cebe koyarsın.
2. **Moto Dash** uygulaması arka planda çalışır ve Google Maps'in navigasyon bildirimini okur: "200 m sonra sağa dön, Bağdat Cd."
3. Uygulama GPS'ten **hızını** ve **yönünü** alır.
4. Bu bilgileri **Bluetooth (BLE)** ile gidondaki ekrana gönderir.
5. Ekran **oku, mesafeyi ve cadde adını** büyük yazıyla çizer. **Dış halka** dönüşe yaklaştıkça dolar.
6. Faz 2'den itibaren uygulama, bulunduğun yerin **küçük bir harita görüntüsünü** de saniyede 1–3 kez gönderir. Ekran onu ortaya koyar.
7. Navigasyon bitince veya telefon uzaklaşınca ekran saat ve pil durumunu gösterir. Bir süre sonra kararır.

---

## 3. Ekran tasarımı

### Dış halka: dönüşe kalan mesafe (OBD gelene kadar)

```
 Dönüşe 500 m    Dönüşe 200 m     Dönüşe 50 m      Dönüş anı
    ○ boş          ◔ dolmaya        ◕ neredeyse      ● tam dolu
                   başladı          dolu (sarı)      (yeşil, yanıp söner)
```

- Halka, **son 500 metrede** dolmaya başlar. Daha uzaktaysa ince ve gri durur.
- Son 100 metrede renk **sarıya** döner.
- Halka bir bakışta okunduğu için yazıyı okumana gerek kalmaz.
- **Faz 5'te** (OBD) bu halka **devir göstergesi** olur. Mesafe halkası ayrı bir ekran olarak kalabilir.

### Orta alan

| Faz | Orta alanda ne var |
|---|---|
| Faz 1 | Büyük dönüş oku, mesafe ("200 m"), cadde adı, küçük GPS hızı |
| Faz 2 | Arkada harita, üstünde ok, mesafe ve hız |

### Tasarım kuralları

- Siyah arka plan, beyaz ve sarı yazı. IPS ekranın güneşte okunması için yüksek kontrast.
- Mesafe en az 64 px, cadde adı en az 28 px.
- Ekran kenarındaki ~20 px kullanılmaz (yuvarlak ekranda kesilme riski).

---

## Faz 0: Hazırlık

**Süre:** 1–2 akşam · **Alışveriş:** yok (kartı sipariş et)

- [ ] Motorobit'ten 2.8" kartı **sipariş et** (kargo gelene kadar simülatörde çalışılır)
- [ ] Mac'e **VS Code + PlatformIO** kur
- [ ] **Android Studio** kur
- [ ] LVGL simülatörü için: `brew install sdl2 cmake`
- [ ] Android cihazda **geliştirici seçenekleri** ve **USB hata ayıklama**yı aç
- [ ] Repo yapısını oluştur:

```
moto-dash/
├── docs/
├── android/         # Kotlin uygulama
├── firmware/        # ESP32-S3 (PlatformIO)
│   └── simulator/   # LVGL Mac simülatörü
└── hardware/        # kasa, butonlar (Faz 3)
```

---

## Faz 1: Navigasyon MVP (masada)

**Süre:** ~2–3 hafta
**Hedef:** Google Maps'te rota başlatınca masadaki ekranda doğru ok ve azalan mesafe görünmesi.

### 1.1 Arayüz (simülatörde, kart gelmeden)

- [ ] LVGL simülatörünü 480×480, dairesel maskeyle kur
- [ ] **Navigasyon ekranı:** ok, mesafe, cadde adı, GPS hızı
- [ ] **Dış halka:** mesafe halkası (500 m → 0, sarıya dönüş)
- [ ] **Bekleme ekranı:** saat, pil, "telefon bekleniyor"
- [ ] Ok ikonları: düz, hafif/normal/keskin sağ ve sol, U dönüşü, dönel kavşak (çıkış numarasıyla), varış
- [ ] Sahte veri üreteci: mesafe 500'den 0'a insin, ok değişsin

### 1.2 Kart geldiğinde

- [ ] Waveshare'in ESP32-S3-LCD-2.8C demosunu yükle, ekranın çalıştığını gör
- [ ] Ekran sürücüsü (ST7701, RGB) ve pin ayarlarını kendi projene al
- [ ] Simülatördeki arayüzü karta taşı (aynı `ui/` klasörü)

### 1.3 BLE bağlantısı

- **ESP32-S3:** BLE peripheral (NimBLE-Arduino)
- **Android:** BLE central

| | Yön | Özellik |
|---|---|---|
| RX karakteristiği | Android → ESP32 | Write Without Response |
| TX karakteristiği | ESP32 → Android | Notify |

**Bu fazdaki mesajlar** (`[tip: 1 bayt][veri]`):

| Tip | Ad | Veri |
|---|---|---|
| `0x02` | NAV | `u8 manevra, u8 kavşak_çıkışı, u16 mesafe_m, u16 varış_dk, u8 uzunluk, utf8 cadde` |
| `0x08` | NAV_END | (boş) navigasyon bitti |
| `0x09` | GPS | `u8 hız_kmh, u16 yön_derece` |
| `0x07` | TIME | `u32 unix_zaman, i16 utc_farkı_dk` |
| `0x82` | STATUS (ESP32 → Android) | `u8 pil_yüzde` |

> Tip numaraları ana plandaki protokolle uyumlu tutuldu. Faz 5'te `0x01 TELEMETRY` (devir) eklenecek.

### 1.4 Android uygulaması

- [ ] Kotlin + Jetpack Compose, minSdk 29
- [ ] `DashService`: **foreground service** (kalıcı bildirimle), pil optimizasyonu muafiyeti
- [ ] `DashLink`: ESP32'yi servis UUID'si ile bul, bağlan, kopunca yeniden bağlan
- [ ] `NavNotificationReader` (`NotificationListenerService`):
  - [ ] `com.google.android.apps.maps` bildirimlerini dinle
  - [ ] Başlık ve metinden **mesafe** ve **cadde adını** ayrıştır
  - [ ] Bildirimin büyük simgesinden **manevra tipini** çıkar: simgeyi küçültüp bilinen ok şablonlarıyla karşılaştır
  - [ ] Navigasyon bildirimi kaybolunca `NAV_END` gönder
- [ ] `LocationSource`: GPS hızı ve yönü, saniyede 1 kez `GPS` mesajı
- [ ] Basit ana ekran: bağlantı durumu, son gönderilen NAV verisi, izin butonları

### 1.5 Gereken izinler

- Bluetooth (bağlanma, tarama)
- Konum (GPS hızı için)
- **Bildirim erişimi** (Ayarlar → Bildirim erişimi → Moto Dash)
- Pil optimizasyonundan muafiyet

### 1.6 Test

- [ ] Masada: Google Maps'te bir rota başlat, telefonu yürüyerek gezdir, ekranda ok ve mesafe değişsin
- [ ] Arabada (yolcu koltuğunda): 10 dönüşlük bir rotada okların doğruluğunu not et
- [ ] Telefon kilitliyken 30 dk çalışmaya devam ediyor mu?

**✅ Faz 1 bitti sayılır:** Arabada 10 dönüşün en az 9'u doğru gösteriliyorsa.

---

## Faz 2: Ortaya harita

**Süre:** ~1–2 hafta
**Hedef:** Ortadaki dairede yön yukarıda (heading-up) canlı harita.

### 2.1 Android tarafı

- [ ] **MapLibre Android SDK** ekle
- [ ] **MapTiler** ücretsiz hesabı aç, API anahtarı al (OpenStreetMap verisi)
- [ ] Koyu ve sade bir harita stili seç (sadece yollar, su, park; yazılar az)
- [ ] `MapSnapshotter` ile görüntü üret:
  - Boyut **240×240** (ekranda 2 kat büyütülür)
  - Merkez: GPS konumu, yön: GPS yönü
  - Zoom hıza göre: yavaşta 17, hızlıda 15
- [ ] JPEG kalite ~60, hedef 8–15 KB
- [ ] Parçalara bölüp gönder: `0x03 MAP_CHUNK` → `u16 kare_no, u16 parça_no, u16 parça_sayısı, veri`
- [ ] Bağlantı ayarları: `requestMtu(517)`, `PHY_LE_2M`, `CONNECTION_PRIORITY_HIGH`
- [ ] Kare hızı: normalde 1/sn, dönüşe 100 m kala 2–3/sn

### 2.2 ESP32 tarafı

- [ ] Parçaları PSRAM'de birleştir, JPEG'i çöz (`esp_jpeg` veya TJpgDec)
- [ ] Orta daireye dairesel kırpılmış görüntü olarak koy
- [ ] Konum işaretçisi, ok ve hızı haritanın üstüne **ESP32 çizsin** (harita yavaş yenilense bile bunlar anlık güncellenir)
- [ ] Eksik kare gelirse bir öncekini göstermeye devam et

### 2.3 Rota çizgisi

Google Maps rota geometrisini dışarıya vermez. İki seçenek var:

- **Şimdilik:** rota çizgisi yok, harita sadece konumu ve çevreyi gösterir. Ok ve mesafe zaten yön bilgisini veriyor.
- **İleride:** varış noktası uygulamada seçilir, rota OSRM veya GraphHopper ile hesaplanıp haritaya çizilir.

**✅ Faz 2 bitti sayılır:** Arabada harita saniyede en az 1 kez yenileniyor ve dönüşlerde doğru yöne çeviriyorsa.

---

## Faz 3: Motora takma

**Süre:** ~1–2 hafta
**Hedef:** 1 saatlik sürüşü sorunsuz tamamlayan, kullanılabilir bir ürün.

### 3.1 Pil

- [ ] Kartın pil konnektörünü pinout şemasından doğrula (büyük ihtimalle MX1.25, 2 pin)
- [ ] **Kutup yönünü kontrol etmeden takma**
- [ ] Pil süresini ölç: parlaklık %80, BLE bağlı, harita akıyor. Hedef ≥ 6 saat.
- [ ] Bağlantı yoksa 2 dk sonra ekranı karart, 10 dk sonra uyku moduna geç

### 3.2 Butonlar

- [ ] Karttaki SH1.0 konnektörlerinden boş GPIO'ları bul
- [ ] 3 buton: **Sonraki ekran**, **Seç**, **Geri**
- [ ] Dahili pull-up, 30 ms debounce, uzun basış (>800 ms) için ikinci fonksiyon
- [ ] Buton olaylarını `0x81 BUTTON` ile Android'e de gönder (Faz 4'te müzik kontrolü için)

### 3.3 Kasa

- [ ] Fusion 360 veya Tinkercad ile tasarla:
  - Ön: ekran yuvası ve contalı kapak
  - Arka: pil bölmesi, tıpalı USB-C erişimi
  - Alt: GoPro tipi bağlantı ayağı
  - Kenar: 3 buton yuvası
  - Üst: 2–3 mm güneşlik
- [ ] Malzeme **ASA** veya **PETG** (PLA güneşte yumuşar)

### 3.4 Montaj

- [ ] Hunter gidon çapı **22 mm**
- [ ] Gösterge paneline yakın, göz hizasına yakın yerleştir. Paneli kapatmasın.
- [ ] Titreşim için kauçuk ara parça, kasanın içinde pil ve kablolar sabit

### 3.5 Yol testi

- [ ] Öğlen güneşinde okunabilirlik
- [ ] Telefon farklı ceplerdeyken bağlantı kopuyor mu?
- [ ] 30 dk sürüşten sonra kasa ve montaj yerinde mi?
- [ ] 1 saatlik sürüşte pil ne kadar düştü?
- [ ] Butonlara eldivenle basılabiliyor mu?
- [ ] Hortumla kontrollü su testi

**✅ Faz 3 bitti sayılır:** 1 saatlik gerçek sürüşte navigasyon baştan sona sorunsuz çalışıyorsa.

> 🎉 **Bu noktada kullanılabilir bir ürünün var.**

---

## Faz 4: Müzik ve arama (isteğe bağlı)

**Süre:** ~1 hafta · **Alışveriş:** yok

- [ ] **Müzik:** `MediaSessionManager` ile çalan şarkı → `0x05 MEDIA`. Butonla oynat/duraklat/sonraki.
- [ ] **Arama:** gelen arama ve arayan kişi → `0x06 CALL`. Ekranda otomatik açılsın, butonla reddet.
- [ ] **Bildirim:** sadece seçili uygulamalardan, sadece başlık → `0x04 NOTIFICATION`

> **Ana telefonun iPhone ise:** bu fazı atla, ya da ESP32'ye iPhone ANCS/AMS desteği ekle (ayrı bir iş).

---

## Faz 5: OBD ve devir göstergesi

**Süre:** ~2–3 hafta
**Hedef:** Dış halkanın devir göstergesi olması.

### 5.1 Ön hazırlık (Faz 1–3 sürerken, bedava)

- [ ] Koltuğu açıp OBD soketini bul ve fotoğrafla (kapak rengi, pin sayısı, tam karşıdan görünüm)
- [ ] İstersen multimetreyle ölç: CAN High ~2.5–3.5 V, CAN Low ~1.5–2.5 V

### 5.2 Karar

| | A) ELM327 | B) Kendi CAN düğümümüz |
|---|---|---|
| Ayrıntılar | `moto-dash-plan.md` Faz 1–2 | `can-yolu-detayli.md` |
| Maliyet | ~900–2.100 TL | ~300–450 TL (+ konnektör) |
| Devir güncelleme | ~10–20 Hz | ~25–50 Hz |
| Veri yolu | Motor → Android → ekran | Motor → doğrudan ekran |
| Zorluk | Tak-çalıştır | Lehim ve kod |

### 5.3 Ekran tarafında değişen

- [ ] Dış halka → **devir göstergesi** (Hunter kırmızı bölgesi kılavuzdan doğrulanacak)
- [ ] Mesafe halkası → navigasyon sırasında ayrı bir ekran veya ince bir iç halka
- [ ] Yeni ekran: **Motor** (hız, sıcaklık, akü voltajı)
- [ ] Yeni mesaj: `0x01 TELEMETRY` → `u16 rpm, u8 hız, i8 sıcaklık, u8 gaz, u16 akü_mV, u8 bayraklar`

---

## Faz faz alışveriş listesi

> ✅ = sayfası kontrol edildi · 🔎 = arama linki, uygun ilanı sen seçeceksin

### Faz 0–1: Şimdi alınacaklar

| Parça | Nereden | Fiyat |
|---|---|---|
| **Waveshare ESP32-S3 2.8" 480×480 yuvarlak ekran kartı** (SKU 30254) | ✅ [Motorobit](https://www.motorobit.com/esp32-s3-28-480480-yuvarlak-ekranli-gelistirme-karti) | **4.339 TL** |
| USB-C veri kablosu (evde yoksa) | Herhangi bir yer | 100–200 TL |
| **Toplam** | | **~4.340–4.540 TL** |

### Faz 2: Ek alışveriş yok

- MapTiler ücretsiz API anahtarı yeterli.

### Faz 3: Motora takmak için

| Parça | Nereden | Fiyat |
|---|---|---|
| 3.7V LiPo pil, 1500–2000 mAh, koruma devreli (**kart gelip konnektör görüldükten sonra al**) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=3.7v+lipo+pil+2000mah) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=3.7v+lipo+2000mah) | 150–300 TL |
| Su geçirmez anlık buton ×3 (12–16 mm) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=su+ge%C3%A7irmez+buton+12mm) | 50–150 TL |
| GoPro tipi gidon bağlantı aparatı (22 mm gidon) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=gopro+gidon+ba%C4%9Flant%C4%B1+aparat%C4%B1) | 150–400 TL |
| 3D baskı kasa (ASA veya PETG) | Yerel 3D baskı servisi | 200–500 TL |
| USB-C port tıpası | 🔎 [Trendyol](https://www.trendyol.com/sr?q=usb+c+toz+t%C4%B1pas%C4%B1) | ~50 TL |
| Havya seti (yoksa) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=havya+seti) | 300–500 TL |
| **Toplam** | | **~600–1.900 TL** |

### Faz 4: Ek alışveriş yok

### Faz 5: OBD (seçeneğe göre)

**A) ELM327 yolu**

| Parça | Nereden | Fiyat |
|---|---|---|
| Vgate iCar Pro (BT 3.0 veya BLE 4.0) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=vgate+icar+pro) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=vgate+icar+pro) | 700–1.500 TL |
| Royal Enfield OBD → OBD2 kablo (**soket fotoğrafına göre**) | 🔎 [AliExpress](https://www.aliexpress.com/w/wholesale-royal-enfield-obd2-cable.html) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=royal+enfield+obd) | 200–600 TL |

**B) Kendi CAN düğümümüz**

| Parça | Nereden | Fiyat |
|---|---|---|
| ESP32-C3 SuperMini | 🔎 [Trendyol](https://www.trendyol.com/sr?q=esp32-c3+supermini) | 150–250 TL |
| SN65HVD230 CAN modülü | 🔎 [Trendyol](https://www.trendyol.com/sr?q=SN65HVD230) | 50–100 TL |
| Mini buck dönüştürücü (MP1584 vb.) | 🔎 [Trendyol](https://www.trendyol.com/sr?q=mp1584+voltaj+d%C3%BC%C5%9F%C3%BCr%C3%BCc%C3%BC) | 30–60 TL |
| Sigorta, konnektör, makaron | Elektronik mağazası | ~50 TL |

**İki seçenek için de**

| Parça | Fiyat |
|---|---|
| Multimetre (yoksa) | 300–600 TL |

---

## Özet takvim ve bütçe

| Faz | Süre | Harcama | Sonuç |
|---|---|---|---|
| 0–1 | ~2–3 hafta | ~4.340 TL | Masada çalışan navigasyon |
| 2 | ~1–2 hafta | 0 TL | Haritalı navigasyon |
| 3 | ~1–2 hafta | ~600–1.900 TL | **Motorda kullanılabilir ürün** |
| 4 | ~1 hafta | 0 TL | Müzik ve arama |
| 5 | ~2–3 hafta | ~300–2.100 TL | Devir göstergesi |
| **Toplam** | **~2–3 ay** | **~5.200–8.300 TL** | |

**Şimdi alınacak tek şey:** Motorobit'teki 2.8" kart.

---

## Riskler

| Risk | Etki | Çözüm |
|---|---|---|
| Google Maps bildirim formatı değişir | Ok veya mesafe yanlış okunur | Ayrıştırıcıyı esnek yaz, simge eşleştirmeyi güncelle. İleride OSRM ile kendi rota hesabı. |
| Android uygulamayı arka planda kapatır | Navigasyon ekrana gelmez | Foreground service, pil optimizasyonu muafiyeti, markaya özel ayarlar |
| IPS ekran güneşte zor okunur | Sürüşte bilgi kaçar | Yüksek kontrast, güneşlik. Gerekirse AMOLED karta geçiş. |
| BLE bağlantısı cepte kopar | Ekran donar | Otomatik yeniden bağlanma, "bağlantı yok" uyarısı |
| Harita kare hızı düşük kalır | Harita takılır | Ok ve hız ESP32'de çizildiği için asıl bilgi etkilenmez. Görüntü boyutunu küçült. |

---

## Açık sorular

- [ ] Cepteki Android cihaz ana telefonun mu, ikinci bir cihaz mı?
- [ ] Ana telefonun iPhone mu? (Faz 4'ü etkiler)
- [ ] Ekranın gidondaki yeri (panelin solu, sağı, üstü)
- [ ] OBD soketi fotoğrafı (Faz 5'ten önce)

---

## İlgili dokümanlar

- `moto-dash-plan.md`: ilk plan (ELM327 + Android, her şey birlikte)
- `can-yolu-basit.md`: kendi OBD okuyucumuz, basit anlatım
- `can-yolu-detayli.md`: kendi OBD okuyucumuz, teknik anlatım
