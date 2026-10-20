# Revani: Proje Planı (Navigasyon Öncelikli)

> Royal Enfield Hunter 350 için yuvarlak gidon ekranı.
> Önce navigasyon, sonra OBD ve devir göstergesi.
>
> **Bu dosya yaşayan bir dokümandır.** Bir adım tamamlandığında `[ ]` → `[x]` yapılır ve [İlerleme](#ilerleme) tablosu güncellenir.
>
> Oluşturulma: 20 Ekim 2026. Fiyatlar bu tarihte kontrol edildi.

---

## İlerleme

| Faz | Durum | Tamamlanan |
|---|---|---|
| Faz 0: Hazırlık | ⬜ Başlamadı | 0 / 10 |
| Faz 1: Navigasyon MVP | ⬜ Başlamadı | 0 / 34 |
| Faz 2: Ortaya harita | ⬜ Başlamadı | 0 / 14 |
| Faz 3: Motora takma | ⬜ Başlamadı | 0 / 24 |
| Faz 4: Müzik ve arama | ⬜ Başlamadı | 0 / 7 |
| Faz 5: OBD ve devir | ⬜ Başlamadı | 0 / 10 |

Durum simgeleri: ⬜ Başlamadı · 🟨 Devam ediyor · ✅ Tamamlandı · ⏸️ Ertelendi

### Değişiklik günlüğü

| Tarih | Değişiklik |
|---|---|
| 2026-10-20 | Plan oluşturuldu. Mimari: Flutter arayüz + Kotlin servis. |

---

## İçindekiler

1. [Neden bu sıra?](#1-neden-bu-sıra)
2. [Sistem nasıl çalışır?](#2-sistem-nasıl-çalışır)
3. [Mobil uygulama mimarisi (Flutter + Kotlin)](#3-mobil-uygulama-mimarisi-flutter--kotlin)
4. [BLE protokolü](#4-ble-protokolü)
5. [Ekran tasarımı](#5-ekran-tasarımı)
6. [Faz 0: Hazırlık](#faz-0-hazırlık)
7. [Faz 1: Navigasyon MVP (masada)](#faz-1-navigasyon-mvp-masada)
8. [Faz 2: Ortaya harita](#faz-2-ortaya-harita)
9. [Faz 3: Motora takma](#faz-3-motora-takma)
10. [Faz 4: Müzik ve arama](#faz-4-müzik-ve-arama-isteğe-bağlı)
11. [Faz 5: OBD ve devir göstergesi](#faz-5-obd-ve-devir-göstergesi)
12. [Faz faz alışveriş listesi](#faz-faz-alışveriş-listesi)
13. [Özet takvim ve bütçe](#özet-takvim-ve-bütçe)
14. [Riskler](#riskler)
15. [Açık sorular](#açık-sorular)
16. [Kaynaklar](#kaynaklar)

---

## 1. Neden bu sıra?

| Sebep | Açıklama |
|---|---|
| **Navigasyon tek başına işe yarıyor** | İlk ayın sonunda gidonda gerçekten kullanılan bir cihaz olur. |
| **Belirsizlikler OBD tarafında** | Soket tipi, CAN olup olmadığı, hangi kablonun alınacağı henüz belli değil. Bunlar netleşmeden para harcanmaz. |
| **Başlangıç maliyeti düşük** | Başlamak için tek bir kart yeterli. |
| **Hiçbir iş boşa gitmez** | Ekran, BLE bağlantısı ve mobil uygulama OBD eklendiğinde aynen kalır. Sadece yeni bir veri kaynağı eklenir. |

---

## 2. Sistem nasıl çalışır?

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

**Bir sürüş, adım adım:**

1. Telefonda **Google Maps**'i açıp navigasyonu başlatırsın, telefonu cebe koyarsın.
2. **Revani** uygulamasının Kotlin servisi arka planda Google Maps'in navigasyon bildirimini okur: "200 m sonra sağa dön, Bağdat Cd."
3. Servis GPS'ten **hızını** ve **yönünü** alır.
4. Bu bilgileri **BLE** ile gidondaki ekrana gönderir.
5. Ekran **oku, mesafeyi ve cadde adını** büyük yazıyla çizer. **Dış halka** dönüşe yaklaştıkça dolar.
6. Faz 2'den itibaren servis, bulunduğun yerin **küçük bir harita görüntüsünü** saniyede 1–3 kez gönderir. Ekran onu ortaya koyar.
7. Navigasyon bitince veya telefon uzaklaşınca ekran saat ve pil durumunu gösterir, bir süre sonra kararır.

---

## 3. Mobil uygulama mimarisi (Flutter + Kotlin)

**Karar:** Arayüz **Flutter**, arka planda sürekli çalışan her şey **Kotlin**.

**Neden:** Uygulamanın kritik işleri (telefon cepteyken sürekli çalışma, Google Maps bildirimindeki ok simgesini okuma, ekran dışı harita görüntüsü üretme, müzik ve arama) Android sistem servislerine bağlı. Flutter paketleri bu konularda zayıf veya eksik. Kotlin servisi bunları güvenilir şekilde yapar, Flutter ise kontrol paneli olur.

```
┌───────────────────────── mobile/ (Flutter projesi) ─────────────────────────┐
│                                                                             │
│  FLUTTER (lib/)                           KOTLIN (android/app/src/main/kotlin)│
│  ┌──────────────────────────┐             ┌───────────────────────────────┐ │
│  │ • ana ekran / durum      │ MethodChannel│ RevaniService (foreground)   │ │
│  │ • cihaz eşleştirme       │ ───────────▶ │ • DashLink        (BLE)      │ │
│  │ • izin akışları          │  komutlar    │ • NavNotificationReader      │ │
│  │ • ayarlar                │              │ • LocationSource  (GPS)      │ │
│  │ • canlı debug ekranı     │ EventChannel │ • MapRenderer     (Faz 2)    │ │
│  │                          │ ◀─────────── │ • MediaReader     (Faz 4)    │ │
│  └──────────────────────────┘  durum akışı └───────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.1 Sorumluluklar

| Katman | Yapar | Yapmaz |
|---|---|---|
| **Flutter** | Arayüz, ayarlar, eşleştirme ekranı, izin isteme akışı, servisten gelen canlı durumu gösterme | BLE bağlantısı, arka plan işi |
| **Kotlin servisi** | BLE, bildirim okuma, GPS, harita görüntüsü, müzik/arama, yeniden bağlanma | Arayüz |

> Flutter arayüzü kapalıyken bile Kotlin servisi çalışmaya devam eder.

### 3.2 Kanallar

| Kanal | Tip | Yön | İçerik |
|---|---|---|---|
| `revani/control` | MethodChannel | Flutter → Kotlin | `startService`, `stopService`, `scanDevices`, `connect(deviceId)`, `disconnect`, `setSettings(map)`, `openNotificationAccessSettings`, `requestIgnoreBatteryOptimizations` |
| `revani/status` | EventChannel | Kotlin → Flutter | `{ serviceRunning, bleState, deviceName, displayBattery, lastNav, gpsSpeed, mapFps, errors }` (~2 Hz) |
| `revani/scan` | EventChannel | Kotlin → Flutter | Bulunan cihazlar listesi (tarama sırasında) |

### 3.3 Klasör yapısı

```
revani/
├── docs/
│   ├── PLAN.md                  # bu dosya
│   └── legacy/                  # eski plan ve fikir dokümanları
├── mobile/                      # Flutter projesi
│   ├── lib/
│   │   ├── main.dart
│   │   ├── channels/            # MethodChannel / EventChannel sarmalayıcıları
│   │   ├── screens/             # home, pairing, settings, debug
│   │   └── models/              # Status, NavInfo, Settings
│   └── android/app/src/main/kotlin/<paket>/
│       ├── MainActivity.kt      # kanalları kaydeder
│       ├── RevaniService.kt     # foreground service
│       ├── ble/DashLink.kt
│       ├── ble/Protocol.kt      # mesaj kodlama (bölüm 4)
│       ├── nav/NavNotificationReader.kt
│       ├── nav/ManeuverMatcher.kt
│       ├── location/LocationSource.kt
│       ├── map/MapRenderer.kt   # Faz 2
│       └── media/MediaReader.kt # Faz 4
├── firmware/                    # ESP32-S3 (PlatformIO)
│   ├── src/
│   ├── ui/                      # LVGL arayüzü (simülatörle ortak)
│   └── simulator/               # LVGL Mac simülatörü
└── hardware/                    # kasa (STL), buton kablolama, fotoğraflar
```

---

## 4. BLE protokolü

- **ESP32-S3:** BLE peripheral (NimBLE-Arduino)
- **Android (Kotlin servisi):** BLE central

| | Yön | Özellik |
|---|---|---|
| Servis | | UUID projede üretilecek |
| RX karakteristiği | Android → ESP32 | Write Without Response |
| TX karakteristiği | ESP32 → Android | Notify |

**Bağlantı ayarları:** `requestMtu(517)`, `PHY_LE_2M` (destekleniyorsa), `CONNECTION_PRIORITY_HIGH`

**Mesaj formatı:** `[tip: 1 bayt][veri]`, çok baytlı sayılar little-endian.

| Tip | Ad | Veri | Faz |
|---|---|---|---|
| `0x01` | TELEMETRY | `u16 rpm, u8 hız, i8 sıcaklık, u8 gaz, u16 akü_mV, u8 bayraklar` | 5 |
| `0x02` | NAV | `u8 manevra, u8 kavşak_çıkışı, u16 mesafe_m, u16 varış_dk, u8 uzunluk, utf8 cadde` | 1 |
| `0x03` | MAP_CHUNK | `u16 kare_no, u16 parça_no, u16 parça_sayısı, veri` | 2 |
| `0x04` | NOTIFICATION | `u8 uygulama, u8 len, başlık` | 4 |
| `0x05` | MEDIA | `u8 çalıyor_mu, u8 len, şarkı, u8 len, sanatçı` | 4 |
| `0x06` | CALL | `u8 durum, u8 len, arayan` | 4 |
| `0x07` | TIME | `u32 unix_zaman, i16 utc_farkı_dk` | 1 |
| `0x08` | NAV_END | (boş) | 1 |
| `0x09` | GPS | `u8 hız_kmh, u16 yön_derece` | 1 |
| `0x81` | BUTTON (ESP32 → Android) | `u8 buton_no, u8 tip (0 kısa, 1 uzun)` | 3 |
| `0x82` | STATUS (ESP32 → Android) | `u8 pil_yüzde, u8 ekran_no` | 1 |

**Manevra kodları:** 0 düz, 1 hafif sağ, 2 sağ, 3 keskin sağ, 4 U dönüşü, 5 hafif sol, 6 sol, 7 keskin sol, 8 dönel kavşak (çıkış no ile), 9 varış.

---

## 5. Ekran tasarımı

### Dış halka: dönüşe kalan mesafe (OBD gelene kadar)

```
 Dönüşe 500 m    Dönüşe 200 m     Dönüşe 50 m      Dönüş anı
    ○ boş          ◔ dolmaya        ◕ neredeyse      ● tam dolu
                   başladı          dolu (sarı)      (yeşil, yanıp söner)
```

- Halka **son 500 metrede** dolmaya başlar. Daha uzaktaysa ince ve gri durur.
- Son 100 metrede renk **sarıya** döner.
- **Faz 5'te** bu halka **devir göstergesi** olur.

### Orta alan

| Faz | İçerik |
|---|---|
| Faz 1 | Büyük dönüş oku, mesafe, cadde adı, küçük GPS hızı |
| Faz 2 | Arkada harita, üstünde ok, mesafe ve hız |

### Tasarım kuralları

- Siyah arka plan, beyaz ve sarı yazı (IPS ekranın güneşte okunması için)
- Mesafe en az 64 px, cadde adı en az 28 px
- Kenardaki ~20 px kullanılmaz (yuvarlak ekranda kesilme riski)

---

## Faz 0: Hazırlık

**Süre:** 1–2 akşam · **Alışveriş:** 2.8" kart

- [ ] 0.1 Motorobit'ten 2.8" kartı sipariş et
- [ ] 0.2 VS Code + PlatformIO kur
- [ ] 0.3 Android Studio kur (Kotlin tarafı ve Android SDK için)
- [ ] 0.4 Flutter SDK güncel mi kontrol et (`flutter doctor`)
- [ ] 0.5 LVGL simülatörü bağımlılıkları: `brew install sdl2 cmake`
- [ ] 0.6 Android cihazda geliştirici seçenekleri ve USB hata ayıklamayı aç
- [ ] 0.7 Repo klasör yapısını oluştur (bölüm 3.3)
- [ ] 0.8 `mobile/` altında Flutter projesini oluştur (`flutter create --org <org> --platforms android revani`)
- [ ] 0.9 `firmware/` altında PlatformIO projesini oluştur
- [ ] 0.10 BLE servis ve karakteristik UUID'lerini üret, bölüm 4'e yaz

---

## Faz 1: Navigasyon MVP (masada)

**Süre:** ~2–3 hafta
**Hedef:** Google Maps'te rota başlatınca masadaki ekranda doğru ok ve azalan mesafe görünmesi.

### 1.1 Ekran arayüzü (simülatörde, kart gelmeden)

- [ ] 1.1.1 LVGL simülatörünü 480×480, dairesel maskeyle kur
- [ ] 1.1.2 Navigasyon ekranı: ok, mesafe, cadde adı, GPS hızı
- [ ] 1.1.3 Dış halka: mesafe halkası (500 m → 0, son 100 m'de sarı)
- [ ] 1.1.4 Bekleme ekranı: saat, pil, "telefon bekleniyor"
- [ ] 1.1.5 Ok ikonları: düz, hafif/normal/keskin sağ-sol, U dönüşü, dönel kavşak, varış
- [ ] 1.1.6 Sahte veri üreteci (mesafe 500 → 0, ok değişimi)

### 1.2 Kart geldiğinde

- [ ] 1.2.1 Waveshare ESP32-S3-LCD-2.8C demosunu yükle, ekranın çalıştığını gör
- [ ] 1.2.2 ST7701 (RGB) sürücü ve pin ayarlarını kendi projene al
- [ ] 1.2.3 Simülatördeki arayüzü karta taşı (ortak `ui/` klasörü)

### 1.3 ESP32 BLE

- [ ] 1.3.1 NimBLE ile servis ve RX/TX karakteristikleri
- [ ] 1.3.2 Gelen mesajı tipine göre ayrıştır (NAV, NAV_END, GPS, TIME)
- [ ] 1.3.3 LVGL'i BLE callback'inden değil ana döngüden güncelle (kuyruk ile)
- [ ] 1.3.4 STATUS mesajı ile pil yüzdesini gönder
- [ ] 1.3.5 Bağlantı kopunca bekleme ekranına dön, yeniden yayına başla

### 1.4 Kotlin servisi

- [ ] 1.4.1 `RevaniService`: foreground service, kalıcı bildirim
- [ ] 1.4.2 Pil optimizasyonu muafiyeti isteği
- [ ] 1.4.3 `DashLink`: UUID ile tarama, bağlanma, MTU/PHY ayarları
- [ ] 1.4.4 `DashLink`: kopunca üstel beklemeyle yeniden bağlanma
- [ ] 1.4.5 `Protocol.kt`: mesaj kodlama (NAV, NAV_END, GPS, TIME)
- [ ] 1.4.6 `NavNotificationReader`: `com.google.android.apps.maps` bildirimlerini dinle
- [ ] 1.4.7 Bildirimden mesafe ve cadde adını ayrıştır
- [ ] 1.4.8 `ManeuverMatcher`: bildirimin büyük simgesinden manevra tipini çıkar (küçültme + şablon karşılaştırma)
- [ ] 1.4.9 Navigasyon bildirimi kaybolunca `NAV_END` gönder
- [ ] 1.4.10 `LocationSource`: GPS hızı ve yönü, 1 Hz `GPS` mesajı
- [ ] 1.4.11 Bağlanınca `TIME` gönder

### 1.5 Flutter arayüzü

- [ ] 1.5.1 `revani/control` MethodChannel ve `revani/status`, `revani/scan` EventChannel'ları (Dart + Kotlin tarafı)
- [ ] 1.5.2 İzin akışı ekranı: Bluetooth, konum, bildirim erişimi, pil optimizasyonu
- [ ] 1.5.3 Eşleştirme ekranı: tarama, cihaz seçme, bağlanma
- [ ] 1.5.4 Ana ekran: servis aç/kapat, bağlantı durumu, ekran pili
- [ ] 1.5.5 Debug ekranı: son NAV verisi, GPS hızı, hatalar

### 1.6 Test

- [ ] 1.6.1 Masada: rota başlat, telefonla yürü, ekranda ok ve mesafe değişsin
- [ ] 1.6.2 Telefon kilitliyken 30 dk kesintisiz çalışma
- [ ] 1.6.3 Arabada (yolcu koltuğunda) 10 dönüşlük rota: en az 9 doğru ok
- [ ] 1.6.4 Flutter arayüzü kapatılınca servis çalışmaya devam ediyor mu?

**✅ Faz 1 bitti sayılır:** Arabada 10 dönüşün en az 9'u doğru gösteriliyorsa.

---

## Faz 2: Ortaya harita

**Süre:** ~1–2 hafta
**Hedef:** Orta dairede yön yukarıda (heading-up) canlı harita.

### 2.1 Kotlin tarafı

- [ ] 2.1.1 MapLibre Android SDK ekle
- [ ] 2.1.2 MapTiler ücretsiz hesabı aç, API anahtarı al
- [ ] 2.1.3 Koyu, sade bir harita stili seç
- [ ] 2.1.4 `MapRenderer`: `MapSnapshotter` ile 240×240 görüntü (merkez GPS, yön GPS)
- [ ] 2.1.5 Hıza göre zoom (yavaşta 17, hızlıda 15)
- [ ] 2.1.6 JPEG kalite ~60, hedef 8–15 KB
- [ ] 2.1.7 Parçalara bölüp `MAP_CHUNK` ile gönder
- [ ] 2.1.8 Kare hızı: normalde 1/sn, dönüşe 100 m kala 2–3/sn

### 2.2 ESP32 tarafı

- [ ] 2.2.1 Parçaları PSRAM'de birleştir
- [ ] 2.2.2 JPEG çöz (`esp_jpeg` veya TJpgDec)
- [ ] 2.2.3 Dairesel kırpılmış görüntü olarak ortaya yerleştir
- [ ] 2.2.4 Konum işaretçisi, ok ve hızı haritanın üstüne çiz
- [ ] 2.2.5 Eksik karede bir öncekini göster

### 2.3 Test

- [ ] 2.3.1 Arabada harita ≥ 1 kare/sn ve dönüşlerde doğru yöne dönüyor

**✅ Faz 2 bitti sayılır:** 2.3.1 sağlanıyorsa.

> Rota çizgisi bu fazda yok. İleride varış noktası uygulamada seçilip OSRM/GraphHopper ile rota hesaplanabilir.

---

## Faz 3: Motora takma

**Süre:** ~1–2 hafta
**Hedef:** 1 saatlik sürüşü sorunsuz tamamlayan, kullanılabilir bir ürün.

### 3.1 Pil

- [ ] 3.1.1 Kartın pil konnektörünü pinout şemasından doğrula (büyük ihtimalle MX1.25, 2 pin)
- [ ] 3.1.2 Uygun LiPo'yu al, **kutup yönünü kontrol ederek** tak
- [ ] 3.1.3 Bağlantı yoksa 2 dk sonra ekranı karart, 10 dk sonra uyku
- [ ] 3.1.4 Pil süresi ölçümü (parlaklık %80, BLE bağlı, harita akıyor). Hedef ≥ 6 saat.

### 3.2 Butonlar

- [ ] 3.2.1 SH1.0 konnektörlerinden boş GPIO'ları bul
- [ ] 3.2.2 3 butonu bağla: Sonraki ekran, Seç, Geri
- [ ] 3.2.3 Dahili pull-up ve 30 ms debounce
- [ ] 3.2.4 Uzun basış (>800 ms) ikinci fonksiyon
- [ ] 3.2.5 Buton olaylarını `BUTTON` mesajı ile Android'e gönder

### 3.3 Kasa

- [ ] 3.3.1 Kasa tasarımı: ekran yuvası ve contalı kapak
- [ ] 3.3.2 Pil bölmesi ve tıpalı USB-C erişimi
- [ ] 3.3.3 GoPro tipi bağlantı ayağı
- [ ] 3.3.4 Buton yuvaları ve 2–3 mm güneşlik
- [ ] 3.3.5 ASA veya PETG ile baskı

### 3.4 Montaj

- [ ] 3.4.1 22 mm gidona kelepçe ve GoPro aparatı
- [ ] 3.4.2 Yer seçimi: panele yakın, paneli kapatmadan
- [ ] 3.4.3 Kauçuk ara parça, kasa içinde pil ve kabloların sabitlenmesi

### 3.5 Yol testi

- [ ] 3.5.1 Öğlen güneşinde okunabilirlik
- [ ] 3.5.2 Telefon farklı ceplerdeyken bağlantı
- [ ] 3.5.3 30 dk sürüş sonrası kasa ve montaj kontrolü
- [ ] 3.5.4 1 saatlik sürüşte pil tüketimi
- [ ] 3.5.5 Eldivenle buton kullanımı
- [ ] 3.5.6 Hortumla kontrollü su testi
- [ ] 3.5.7 1 saatlik gerçek navigasyonlu sürüş

**✅ Faz 3 bitti sayılır:** 3.5.7 sorunsuz tamamlanırsa. 🎉 **Kullanılabilir ürün.**

---

## Faz 4: Müzik ve arama (isteğe bağlı)

**Süre:** ~1 hafta · **Alışveriş:** yok

- [ ] 4.1 `MediaReader`: `MediaSessionManager` ile çalan şarkı → `MEDIA`
- [ ] 4.2 Butonla oynat/duraklat/sonraki
- [ ] 4.3 Gelen arama ve arayan → `CALL`
- [ ] 4.4 Ekranda arama ekranı otomatik açılsın, butonla reddet
- [ ] 4.5 Seçili uygulamalardan bildirim başlığı → `NOTIFICATION`
- [ ] 4.6 Flutter ayarlarında bildirim gösterilecek uygulamaları seçme
- [ ] 4.7 Ekranda müzik, arama ve bildirim ekranları

> **Ana telefon iPhone ise:** bu faz atlanabilir ya da ESP32'ye iPhone ANCS/AMS desteği eklenir (ayrı iş).

---

## Faz 5: OBD ve devir göstergesi

**Süre:** ~2–3 hafta
**Hedef:** Dış halkanın devir göstergesi olması.

### 5.1 Ön hazırlık (Faz 1–3 sürerken yapılabilir)

- [ ] 5.1.1 OBD soketini bul ve fotoğrafla (kapak rengi, pin sayısı, tam karşıdan görünüm)
- [ ] 5.1.2 Multimetre ile ölç (CAN High ~2.5–3.5 V, CAN Low ~1.5–2.5 V)
- [ ] 5.1.3 Hunter 350 kırmızı bölge devrini kılavuzdan doğrula

### 5.2 Karar

- [ ] 5.2.1 Yöntem seç: **A) ELM327** veya **B) Kendi CAN düğümü**

> Ayrıntılar: A için [legacy/moto-dash-plan.md](legacy/moto-dash-plan.md) (Faz 1–2), B için [legacy/can-yolu-basit.md](legacy/can-yolu-basit.md) ve [legacy/can-yolu-detayli.md](legacy/can-yolu-detayli.md).

| | A) ELM327 | B) Kendi CAN düğümü |
|---|---|---|
| Maliyet | ~900–2.100 TL | ~300–450 TL (+ konnektör) |
| Devir güncelleme | ~10–20 Hz | ~25–50 Hz |
| Veri yolu | Motor → Android (Kotlin) → ekran | Motor → doğrudan ekran |
| Zorluk | Tak-çalıştır, Kotlin'de klasik Bluetooth | Lehim, ESP32-C3 + SN65HVD230, kod |
| Dikkat | Ucuz v2.1 kopyalarından uzak dur | 120 Ω direnci sök, önce listen-only, uyku modu şart |

### 5.3 Uygulama

- [ ] 5.3.1 Seçilen yöntemle devir okuma (masada / motorda test)
- [ ] 5.3.2 `TELEMETRY` mesajı
- [ ] 5.3.3 Dış halka → devir göstergesi (yumuşatmalı animasyon)
- [ ] 5.3.4 Mesafe halkası → ayrı ekran veya ince iç halka
- [ ] 5.3.5 Yeni ekran: Motor (hız, sıcaklık, akü voltajı)
- [ ] 5.3.6 İlk hafta akü voltajı takibi

---

## Faz faz alışveriş listesi

> ✅ = sayfası kontrol edildi · 🔎 = arama linki, uygun ilanı sen seçeceksin
> Satın alınan parçayı `[x]` ile işaretle.

### Faz 0–1

- [ ] **Waveshare ESP32-S3 2.8" 480×480 yuvarlak ekran kartı** (SKU 30254) · ✅ [Motorobit](https://www.motorobit.com/esp32-s3-28-480480-yuvarlak-ekranli-gelistirme-karti) · **4.339 TL**
- [ ] USB-C veri kablosu (evde yoksa) · 100–200 TL

**Toplam:** ~4.340–4.540 TL

### Faz 2

Ek alışveriş yok. MapTiler ücretsiz API anahtarı yeterli.

### Faz 3

- [ ] 3.7V LiPo pil, 1500–2000 mAh, koruma devreli (**kart gelip konnektör görüldükten sonra**) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=3.7v+lipo+pil+2000mah) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=3.7v+lipo+2000mah) · 150–300 TL
- [ ] Su geçirmez anlık buton ×3 (12–16 mm) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=su+ge%C3%A7irmez+buton+12mm) · 50–150 TL
- [ ] GoPro tipi gidon bağlantı aparatı (22 mm) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=gopro+gidon+ba%C4%9Flant%C4%B1+aparat%C4%B1) · 150–400 TL
- [ ] 3D baskı kasa (ASA veya PETG) · yerel baskı servisi · 200–500 TL
- [ ] USB-C port tıpası · 🔎 [Trendyol](https://www.trendyol.com/sr?q=usb+c+toz+t%C4%B1pas%C4%B1) · ~50 TL
- [ ] Havya seti (yoksa) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=havya+seti) · 300–500 TL

**Toplam:** ~600–1.900 TL

### Faz 4

Ek alışveriş yok.

### Faz 5

**Her iki seçenek için**

- [ ] Multimetre (yoksa) · 300–600 TL

**A) ELM327**

- [ ] Vgate iCar Pro (BT 3.0 veya BLE 4.0) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=vgate+icar+pro) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=vgate+icar+pro) · 700–1.500 TL
- [ ] Royal Enfield OBD → OBD2 kablo (**soket fotoğrafına göre**) · 🔎 [AliExpress](https://www.aliexpress.com/w/wholesale-royal-enfield-obd2-cable.html) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=royal+enfield+obd) · 200–600 TL

**B) Kendi CAN düğümü**

- [ ] ESP32-C3 SuperMini · 🔎 [Trendyol](https://www.trendyol.com/sr?q=esp32-c3+supermini) · 150–250 TL
- [ ] SN65HVD230 CAN modülü · 🔎 [Trendyol](https://www.trendyol.com/sr?q=SN65HVD230) · 50–100 TL
- [ ] Mini buck dönüştürücü (MP1584 vb., giriş ≥ 24V) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=mp1584+voltaj+d%C3%BC%C5%9F%C3%BCr%C3%BCc%C3%BC) · 30–60 TL
- [ ] Sigorta, konnektör, makaron · ~50 TL

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

---

## Riskler

| Risk | Etki | Çözüm |
|---|---|---|
| Google Maps bildirim formatı değişir | Ok veya mesafe yanlış okunur | Ayrıştırıcıyı esnek yaz, şablonları güncelle. İleride kendi rota hesabı. |
| Android servisi arka planda kapatır | Navigasyon ekrana gelmez | Foreground service, pil optimizasyonu muafiyeti, markaya özel ayarlar |
| Flutter ↔ Kotlin köprüsünde hata ayıklama zorluğu | Geliştirme yavaşlar | Kanalları ince tut, iş mantığı tamamen Kotlin'de, Flutter sadece gösterir |
| IPS ekran güneşte zor okunur | Bilgi kaçar | Yüksek kontrast, güneşlik. Gerekirse AMOLED karta geçiş. |
| BLE cepte kopar | Ekran donar | Otomatik yeniden bağlanma, "bağlantı yok" uyarısı |
| Harita kare hızı düşük | Harita takılır | Ok ve hız ESP32'de çizildiği için asıl bilgi etkilenmez |

---

## Açık sorular

- [ ] Cepteki Android cihaz ana telefon mu, ikinci bir cihaz mı?
- [ ] Ana telefon iPhone mu? (Faz 4'ü etkiler)
- [ ] Ekranın gidondaki yeri (panelin solu, sağı, üstü)
- [ ] Flutter/Android paket adı ve organizasyon (`com.xxx.revani`)
- [ ] OBD soketi fotoğrafı (Faz 5'ten önce)

---

## Kaynaklar

**Donanım**
- [Motorobit – ESP32-S3 2.8" yuvarlak ekran kartı](https://www.motorobit.com/esp32-s3-28-480480-yuvarlak-ekranli-gelistirme-karti)
- [Waveshare – ESP32-S3-LCD-2.8C](https://waveshare.com/product/iot-communication/short-range-wireless/esp32-s3-touch-lcd-2.8c.htm)

**Yazılım**
- [LVGL dokümantasyonu](https://docs.lvgl.io/)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [MapLibre Native Android](https://maplibre.org/maplibre-native/android/api/)
- [Flutter – Platform channels](https://docs.flutter.dev/platform-integration/platform-channels)
- [Android – NotificationListenerService](https://developer.android.com/reference/android/service/notification/NotificationListenerService)
- [Android – Foreground services](https://developer.android.com/develop/background-work/services/fgs)

**OBD (Faz 5)**
- [ELM327 datasheet](https://www.scantool.net/scantool/downloads/103/elm327dsh.pdf)
- [Wikipedia – OBD-II PID listesi](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [Team-BHP – Bullet 350 BS6'ya OBD okuyucu bağlama](https://www.team-bhp.com/news/setting-obd-reader-indian-motorcycles)
- [Reddit r/royalenfield – Meteor 350 OBD soketleri](https://pol1.lr.ggtyler.dev/r/royalenfield/comments/1j0uor1/do_you_know_what_this_cable_is)
