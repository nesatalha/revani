# Moto Dash: Royal Enfield Hunter 350 için Yuvarlak Gidon Ekranı

> Proje planı ve yol haritası. Oluşturulma: 20 Ekim 2026.
> Fiyatlar bu tarihte kontrol edildi, satın almadan önce tekrar bak.

---

## İçindekiler

1. [Proje özeti](#1-proje-özeti)
2. [Sistem mimarisi](#2-sistem-mimarisi)
3. [Alışveriş listesi](#3-alışveriş-listesi)
4. [Faz 0: Hazırlık ve araçlar](#faz-0-hazırlık-ve-araçlar)
5. [Faz 1: Motoru tanıma (OBD testi)](#faz-1-motoru-tanıma-obd-testi)
6. [Faz 2: Android uygulama iskeleti ve OBD okuma](#faz-2-android-uygulama-iskeleti-ve-obd-okuma)
7. [Faz 3: ESP32 firmware ve arayüz](#faz-3-esp32-firmware-ve-arayüz)
8. [Faz 4: Android ↔ ESP32 BLE bağlantısı](#faz-4-android--esp32-ble-bağlantısı)
9. [Faz 5: Harita](#faz-5-harita)
10. [Faz 6: Navigasyon (dönüş bilgisi)](#faz-6-navigasyon-dönüş-bilgisi)
11. [Faz 7: Bildirimler ve müzik](#faz-7-bildirimler-ve-müzik)
12. [Faz 8: Donanım, kasa ve montaj](#faz-8-donanım-kasa-ve-montaj)
13. [Faz 9: Yol testi](#faz-9-yol-testi)
14. [Riskler ve çözümler](#riskler-ve-çözümler)
15. [Açık sorular](#açık-sorular)
16. [Kaynaklar](#kaynaklar)

---

## 1. Proje özeti

**Amaç:** Royal Enfield Hunter 350 (2024) gidonuna takılan, **2.8" yuvarlak** bir ekran.

- **Dış halka:** motordan OBD ile okunan **devir göstergesi**
- **Orta daire:** **harita**, üstünde **dönüş oku, mesafe ve hız**
- **Ek ekranlar:** müzik, gelen arama, bildirim, motor bilgileri (sıcaklık, akü voltajı), yatış açısı

**Temel fikir:** Cepteki **Android cihaz veri istasyonu** olur. OBD verisini okur, haritayı ve navigasyonu hazırlar, hepsini Bluetooth ile gidondaki ekrana gönderir. Gidondaki ESP32 **sadece bir ekran** gibi davranır.

**Hedefler:**

- Gidonda **kablo olmaması** (ekran kendi pilinden çalışır)
- Motor üzerinde tek ek parça: OBD soketine takılı ELM327 (koltuk altında gizli)
- Güneşte okunabilir, eldivenle kullanılabilir (fiziksel butonlar)

---

## 2. Sistem mimarisi

```
 Hunter 350                  Cepteki Android                         Gidonda ESP32-S3 2.8"
 ┌──────────────┐ Bluetooth  ┌──────────────────────────────┐  BLE   ┌─────────────────────┐
 │ OBD soketi   │ ─────────▶ │ ObdService                   │ ─────▶ │ LVGL arayüz         │
 │ + adaptör    │            │  • devir, hız, sıcaklık, V   │        │  • devir halkası    │
 │   kablo      │            │ LocationService (GPS)        │        │  • harita (JPEG)    │
 │ + ELM327     │            │ MapRenderer (MapLibre/OSM)   │        │  • ok, mesafe, hız  │
 └──────────────┘            │ NavReader (Google Maps       │        │  • müzik / arama    │
                             │   bildirimini okur)          │        │  • yatış açısı (IMU)│
                             │ NotificationReader, Media    │ ◀───── │  • buton olayları   │
                             │ DashLink (BLE central)       │        └─────────────────────┘
                             └──────────────────────────────┘
```

| Bileşen | Görevi | Teknoloji |
|---|---|---|
| ELM327 | Motorun CAN hattını Bluetooth'a çevirir | OBD2 / ISO 15765-4 CAN |
| Android uygulama | Veri toplar, harita ve navigasyonu hazırlar, ekrana gönderir | Kotlin, foreground service, MapLibre, BLE |
| ESP32-S3 2.8" | Gelen veriyi gösterir, buton olaylarını geri gönderir | PlatformIO, Arduino/ESP-IDF, LVGL 9, NimBLE |

### Önerilen repo yapısı

```
moto-dash/
├── docs/            # bu plan, BLE protokolü, kablolama şemaları
├── android/         # Android Studio projesi (Kotlin)
├── firmware/        # PlatformIO projesi (ESP32-S3)
│   └── simulator/   # LVGL Mac simülatörü (karta gerek kalmadan UI geliştirme)
└── hardware/        # kasa STL dosyaları, buton kablolama, fotoğraflar
```

---

## 3. Alışveriş listesi

> ✅ = sayfası kontrol edildi, ürün ve fiyat doğrulandı.
> 🔎 = arama linki, uygun ilanı sen seçeceksin.

### Zorunlu parçalar

| # | Parça | Neden | Nereden | Fiyat (yaklaşık) |
|---|---|---|---|---|
| 1 | **Waveshare ESP32-S3 2.8" 480×480 yuvarlak ekran kartı** (ESP32-S3-LCD-2.8C, SKU 30254) | Ana ekran ve işlemci | ✅ [Motorobit](https://www.motorobit.com/esp32-s3-28-480480-yuvarlak-ekranli-gelistirme-karti) | **4.339 TL** (KDV dahil), stokta 9 adet vardı |
| 2 | **3.7V LiPo pil, 1500–2000 mAh**, ince tip, koruma devreli | Ekranın kablosuz çalışması | 🔎 [Trendyol](https://www.trendyol.com/sr?q=3.7v+lipo+pil+2000mah) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=3.7v+lipo+2000mah) | 150–300 TL |
| 3 | **ELM327 OBD2 adaptör.** Önerilen: **Vgate iCar Pro** (BT 3.0 veya BLE 4.0) | Motordan veri okuma | 🔎 [Trendyol](https://www.trendyol.com/sr?q=vgate+icar+pro) · 🔎 [Hepsiburada](https://www.hepsiburada.com/ara?q=vgate+icar+pro) | 700–1.500 TL |
| 4 | **Royal Enfield OBD → OBD2 (16 pin) dönüştürücü kablo** | RE soketini ELM327'ye bağlamak | 🔎 [AliExpress](https://www.aliexpress.com/w/wholesale-royal-enfield-obd2-cable.html) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=royal+enfield+obd) · referans ilan: [Newegg](https://www.newegg.com/p/0FB-0ACC-000K7) | 200–600 TL |
| 5 | **Su geçirmez anlık buton** (12–16 mm), 3 adet | Eldivenle ekran kontrolü | 🔎 [Trendyol](https://www.trendyol.com/sr?q=su+ge%C3%A7irmez+buton+12mm) · 🔎 [Motorobit](https://www.motorobit.com/arama?q=buton) | 50–150 TL |
| 6 | **GoPro tipi gidon bağlantı aparatı** | 3D baskı kasayı gidona takmak | 🔎 [Trendyol](https://www.trendyol.com/sr?q=gopro+gidon+ba%C4%9Flant%C4%B1+aparat%C4%B1) · 🔎 [Decathlon](https://www.decathlon.com.tr/search?Ntt=gidon+aksiyon+kamera) | 150–400 TL |
| 7 | **3D baskı kasa** (PETG veya ASA) | Ekranı korumak | Yerel 3D baskı servisi veya Trendyol'daki baskı hizmetleri | 200–500 TL |

### ⚠️ ELM327 ve adaptör kablo için uyarılar

- **ELM327:** ucuz "v2.1" kopyalarından uzak dur, CAN'da yavaş veya hatalı olabiliyorlar. Android kullanacağımız için klasik Bluetooth (BT 3.0) modeli de olur. İleride iPhone'dan da okumak istersen **BLE 4.0** versiyonunu al.
- **Adaptör kablo:** Royal Enfield 350 serisinde **iki farklı OBD soketi** görülüyor:
  - **Siyah kapaklı:** eski Euro 4 / BS4 soketi (6 pin)
  - **Kırmızı kapaklı:** Euro 5 / BS6 soketi

  Piyasadaki 6 pin kablolar genellikle **siyah** sokete uyuyor. 2024 Hunter'da büyük ihtimalle Euro 5 soket var. **Faz 1'de soketi bulup fotoğraflamadan kablo alma.**

### İsteğe bağlı ve yedek parçalar

| Parça | Neden | Nereden |
|---|---|---|
| 12V → 5V USB-C dönüştürücü (kontağa bağlı, su geçirmez) | Uzun yolda ekranı veya Android'i şarj etmek | 🔎 [Trendyol](https://www.trendyol.com/sr?q=motosiklet+usb+c+%C5%9Farj+su+ge%C3%A7irmez) |
| Su geçirmez USB-C port tıpası | Yağmurda şarj girişini korumak | 🔎 [Trendyol](https://www.trendyol.com/sr?q=usb+c+toz+t%C4%B1pas%C4%B1) |
| SN65HVD230 CAN modülü | ELM327'siz, doğrudan CAN okuma (ileri seviye, plan B) | 🔎 [Motorobit](https://www.motorobit.com/arama?q=SN65HVD230) · 🔎 [Trendyol](https://www.trendyol.com/sr?q=SN65HVD230) |
| Waveshare ESP32-S3 2.06" AMOLED saat | Alternatif, güneşte çok daha iyi okunur ama yuvarlak değil | Motorobit (benzer ürünler bölümünde, ~3.535 TL + KDV) |

### Elinde olması gerekenler

- Multimetre (OBD pinlerini doğrulamak için)
- Havya ve lehim (butonlar için)
- USB-C veri kablosu
- Mac (firmware ve Android geliştirme)
- Android cihaz, Android 10 veya üstü önerilir

### Tahmini toplam maliyet

| | Tutar |
|---|---|
| Zorunlu parçalar | **~6.000 – 8.000 TL** |
| İsteğe bağlı parçalarla | ~7.000 – 9.500 TL |

---

## Faz 0: Hazırlık ve araçlar

**Süre:** 1 akşam

### Bilgisayar kurulumu

- [ ] **VS Code** ve **PlatformIO** eklentisi
- [ ] **Android Studio**, en güncel sürüm
- [ ] **Git** ile `moto-dash` reposunu oluştur (yapısı yukarıda)
- [ ] LVGL simülatörü için: `brew install sdl2 cmake`

### Android cihaz hazırlığı

- [ ] Geliştirici seçeneklerini aç: Ayarlar → Telefon hakkında → Yapı numarasına 7 kez dokun
- [ ] USB hata ayıklamayı aç
- [ ] **Car Scanner ELM OBD2** uygulamasını yükle (Faz 1'deki test için)

---

## Faz 1: Motoru tanıma (OBD testi)

**Amaç:** Kod yazmadan önce Hunter'ın gerçekten veri verdiğini doğrulamak.
**Süre:** 1 hafta (kargo dahil)

### 1.1 OBD soketini bul

- [ ] Hunter'ın OBD soketini bul. Genelde koltuk altında veya yan kapakların arkasında olur.
- [ ] Soketin **rengini** (siyah veya kırmızı kapak), **pin sayısını** ve **şeklini** fotoğrafla.
- [ ] Birden fazla soket varsa hepsini fotoğrafla.

### 1.2 Pinleri doğrula (kablo almadan önce)

Kontak **açıkken**, multimetre ile:

| Ölçüm | Beklenen |
|---|---|
| 12V pini ile şase arası | ~12–13 V |
| Toprak (GND) pini | Şase ile süreklilik var |
| CAN High pini (şaseye göre) | ~2.5–3.5 V |
| CAN Low pini (şaseye göre) | ~1.5–2.5 V |

> CAN hatlarında yüksek voltajlı olan **CAN High**, düşük olan **CAN Low**.
> Standart OBD2 (16 pin) tarafında: pin 6 = CAN High, pin 14 = CAN Low, pin 4/5 = toprak, pin 16 = +12V.

### 1.3 Kabloyu al ve test et

- [ ] Fotoğrafa göre doğru dönüştürücü kabloyu sipariş et.
- [ ] ELM327'yi kabloya, kabloyu motora tak, kontağı aç.
- [ ] Android'de Car Scanner uygulamasını aç, ELM327'yi eşleştir ve bağlan.
- [ ] Şunları kontrol et:
  - [ ] Bağlantı kuruluyor mu? Hangi protokolü buldu? (Beklenen: ISO 15765-4 CAN, 11 bit, 500 kbps)
  - [ ] **Devir** (PID 0C) okunuyor mu? Gaz verince değişiyor mu?
  - [ ] **Hız** (PID 0D) okunuyor mu?
  - [ ] **Motor sıcaklığı** (PID 05) var mı? Hunter hava/yağ soğutmalı, bu PID desteklenmeyebilir veya yağ sıcaklığını gösterebilir.
  - [ ] **Akü voltajı** (PID 42), **gaz kelebeği** (PID 11), **emme havası sıcaklığı** (PID 0F) var mı?
  - [ ] Saniyede kaç kez güncelleniyor? (Car Scanner'ın performans ekranında görünür)
- [ ] Desteklenen PID listesini not et (Car Scanner → "Supported PIDs").

**✅ Faz 1 bitti sayılır:** Android'de motordan canlı devir okunuyorsa.

> ❗ Okunmuyorsa: protokolü elle seçmeyi dene (Car Scanner ayarları → ISO 15765-4 CAN 11/500). Hâlâ olmuyorsa plan B'ye geçeriz: ELM327 yerine doğrudan CAN modülü ile okuma.

---

## Faz 2: Android uygulama iskeleti ve OBD okuma

**Amaç:** Kendi uygulamanla motordan devir okumak.
**Süre:** 1 hafta

### 2.1 Proje kurulumu

- [ ] Android Studio → New Project → **Empty Activity (Compose)**, Kotlin, minSdk 29
- [ ] Paket adı: `com.<sen>.motodash`
- [ ] Gerekli izinler (`AndroidManifest.xml`):

```xml
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.FOREGROUND_SERVICE" />
<uses-permission android:name="android.permission.FOREGROUND_SERVICE_CONNECTED_DEVICE" />
<uses-permission android:name="android.permission.FOREGROUND_SERVICE_LOCATION" />
<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
<uses-permission android:name="android.permission.REQUEST_IGNORE_BATTERY_OPTIMIZATIONS" />
```

### 2.2 Uygulama yapısı

```
app/src/main/java/.../motodash/
├── MainActivity.kt            # basit kontrol ekranı (bağlan/başlat, canlı değerler)
├── DashService.kt             # foreground service, her şeyi yönetir
├── obd/
│   ├── Elm327Connection.kt    # Bluetooth soket (BT 3.0) veya BLE bağlantısı
│   ├── ObdPoller.kt           # PID'leri döngüyle sorgular
│   └── PidParser.kt           # yanıtları sayıya çevirir
├── location/LocationSource.kt # GPS hızı ve yönü
├── map/MapRenderer.kt         # Faz 5
├── nav/NavNotificationReader.kt  # Faz 6
├── media/MediaReader.kt       # Faz 7
└── link/DashLink.kt           # ESP32'ye BLE bağlantısı (Faz 4)
```

### 2.3 ELM327 başlatma komutları

Bağlandıktan sonra sırayla gönder. Her komut `\r` ile biter, yanıt `>` ile biter.

| Komut | Anlamı |
|---|---|
| `ATZ` | Sıfırla |
| `ATE0` | Echo kapalı |
| `ATL0` | Satır sonları kapalı |
| `ATS0` | Boşluklar kapalı |
| `ATH0` | Başlıklar kapalı |
| `ATSP6` | Protokol: ISO 15765-4 CAN 11 bit 500 kbps (Faz 1'de bulunan protokol). Bilinmiyorsa `ATSP0` (otomatik) |
| `0100` | Desteklenen PID'leri sor (bağlantı testi) |

### 2.4 Okunacak PID'ler ve formüller

| PID | Veri | Formül | Sorgu sıklığı |
|---|---|---|---|
| `010C` | Devir (rpm) | `(A*256 + B) / 4` | Mümkün olduğunca sık |
| `010D` | Hız (km/h) | `A` | 5 Hz |
| `0111` | Gaz kelebeği (%) | `A * 100 / 255` | 5 Hz |
| `0105` | Motor sıcaklığı (°C) | `A - 40` | 1 Hz |
| `010F` | Emme havası sıcaklığı (°C) | `A - 40` | 0.5 Hz |
| `0142` | Akü voltajı (V) | `(A*256 + B) / 1000` | 0.5 Hz |

**Örnek:** `010C` sorgusuna yanıt `410C1AF8` gelirse: A = 0x1A = 26, B = 0xF8 = 248 → (26×256 + 248) / 4 = **1726 rpm**.

**Hız ipucu:** Komutun sonuna `1` ekle (`010C1`). ELM327 tek yanıt bekleyeceğini bilir, diğer yanıtları beklemeden döner. Bu, CAN'da sorgu hızını ciddi şekilde artırır.

**Sorgu stratejisi:** Devir her döngüde sorgulanır. Diğer PID'ler sırayla araya serpiştirilir. Örnek döngü: `0C, 0D, 0C, 11, 0C, 0D, 0C, 05 ...`

### 2.5 Foreground service

- [ ] `DashService`, kalıcı bir bildirimle ("Moto Dash çalışıyor") foreground service olarak çalışsın.
- [ ] İlk açılışta kullanıcıdan **pil optimizasyonunu kapatma** izni iste.
- [ ] Bağlantı koparsa üstel bekleme ile (1 sn, 2 sn, 4 sn ... en fazla 10 sn) yeniden bağlansın.

**✅ Faz 2 bitti sayılır:** Telefon ekranı kilitliyken bile servis devir okumaya devam ediyor, log'da görünüyorsa.

---

## Faz 3: ESP32 firmware ve arayüz

**Amaç:** Yuvarlak ekranda devir halkası ve orta alan.
**Süre:** 1–2 hafta (Faz 2 ile paralel yürüyebilir)

### 3.1 Karta ilk merhaba

- [ ] Waveshare wiki'den **ESP32-S3-LCD-2.8C** demo kodlarını indir.
- [ ] Arduino veya ESP-IDF demosunu PlatformIO'da derleyip yükle. Ekranda demo görünmeli.
- [ ] Ekran sürücüsü (**ST7701**, RGB arayüz) ayarlarını ve pin tanımlarını demodan kendi projene al.

### 3.2 PlatformIO projesi

`firmware/platformio.ini` (taslak):

```ini
[env:esp32s3-round]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.arduino.memory_type = qio_opi   ; 8MB PSRAM
board_build.flash_size = 16MB
build_flags =
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
lib_deps =
    lvgl/lvgl@^9.2
    h2zero/NimBLE-Arduino@^2.1
monitor_speed = 115200
```

> Kart tanımı ve bellek ayarları Waveshare demosuna göre düzeltilecek.

### 3.3 Ekranlar (LVGL)

| Ekran | İçerik | Geçiş |
|---|---|---|
| **Ana** | Dış halka: devir göstergesi (0–8000 rpm, kırmızı bölge). Orta: harita, ok, mesafe, hız | Varsayılan |
| **Motor** | Büyük hız, sıcaklık, akü voltajı, gaz % | Buton 1 |
| **Müzik** | Şarkı, sanatçı, oynat/duraklat | Buton 1 |
| **Arama** | Arayan kişi, kabul/red | Arama gelince otomatik açılır |
| **Yatış** | IMU'dan anlık ve maksimum yatış açısı | Buton 1 |
| **Bağlantı yok** | "Telefon bekleniyor", saat, pil | Bağlantı koparsa |

> Hunter 350'nin kırmızı bölgesi yaklaşık 6.500 rpm civarında. Faz 1'deki ölçümlerle ve kullanım kılavuzundan kesinleştir.

**Tasarım kuralları:**

- Siyah arka plan, beyaz/sarı büyük yazı (IPS ekranın güneşte okunması için)
- Ana bilgi en az 48 px, yardımcı bilgi en az 24 px
- Ekran kenarındaki ~20 px'lik alan kullanılmaz (yuvarlak ekranda kesilme riski)
- Devir göstergesinde değer değişimleri 50–80 ms içinde yumuşatılır. ELM327 saniyede ~10–20 değer gönderir, araları animasyonla doldurulur.

### 3.4 LVGL simülatörü (kart gelmeden)

- [ ] `firmware/simulator/` altına LVGL'in SDL tabanlı simülatörünü kur.
- [ ] Ekran boyutu 480×480, dairesel maske.
- [ ] Arayüz kodunu (`ui/` klasörü) hem simülatör hem kart kullansın. Böylece tasarımı Mac'te yapıp karta aynen taşırsın.
- [ ] Sahte veri üreteci: devir 800 → 6500 arası dalgalansın, hız ve ok değişsin.

### 3.5 Butonlar ve IMU

- [ ] Karttaki SH1.0 konnektörlerinden boş GPIO'ları bul (Waveshare pinout şeması).
- [ ] 3 buton: **Sonraki ekran**, **Seç / oynat-duraklat**, **Geri / arama reddet**.
- [ ] Dahili pull-up ve yazılımsal debounce (30 ms).
- [ ] Uzun basış (>800 ms) için ikinci fonksiyon.
- [ ] QMI8658 IMU'dan yatış açısı: ivmeölçer ve jiroskop verisini tamamlayıcı filtre (complementary filter) ile birleştir.

**✅ Faz 3 bitti sayılır:** Kartta sahte veriyle tüm ekranlar dönüyor, butonlarla geçiş çalışıyorsa.

---

## Faz 4: Android ↔ ESP32 BLE bağlantısı

**Amaç:** Android'deki gerçek verilerin ekrana gelmesi.
**Süre:** 1 hafta

### 4.1 Roller

- **ESP32:** BLE **peripheral** (yayın yapar, NimBLE)
- **Android:** BLE **central** (bulur ve bağlanır)

### 4.2 GATT servisi

| | UUID (örnek, kendin üret) | Yön | Özellik |
|---|---|---|---|
| Servis | `6d6f746f-6461-7368-0000-000000000000` | | |
| **RX** | `...0001` | Android → ESP32 | Write Without Response |
| **TX** | `...0002` | ESP32 → Android | Notify |

**Bağlantı ayarları (Android tarafı):**

- `requestMtu(517)`
- `setPreferredPhy(PHY_LE_2M)` (desteklenirse)
- `requestConnectionPriority(CONNECTION_PRIORITY_HIGH)`

### 4.3 Mesaj formatı

Her mesaj: `[tip: 1 bayt][veri]`. Çok baytlı sayılar little-endian.

| Tip | Ad | Veri | Sıklık |
|---|---|---|---|
| `0x01` | TELEMETRY | `u16 rpm, u8 hız_kmh, i8 sıcaklık_c, u8 gaz_yüzde, u16 akü_mV, u8 bayraklar` | ~20 Hz |
| `0x02` | NAV | `u8 manevra, u16 mesafe_m, u16 varış_dk, u8 uzunluk, utf8 cadde_adı` | Değişince |
| `0x03` | MAP_CHUNK | `u16 kare_no, u16 parça_no, u16 parça_sayısı, payload` | 1–3 kare/sn |
| `0x04` | NOTIFICATION | `u8 uygulama, u8 başlık_uzunluk, başlık, u8 metin_uzunluk, metin` | Geldikçe |
| `0x05` | MEDIA | `u8 çalıyor_mu, u8 len, şarkı, u8 len, sanatçı` | Değişince |
| `0x06` | CALL | `u8 durum (0 yok, 1 çalıyor, 2 görüşmede), u8 len, arayan` | Değişince |
| `0x07` | TIME | `u32 unix_zaman, i16 utc_farkı_dk` | Bağlanınca |
| `0x81` | BUTTON (ESP32 → Android) | `u8 buton_no, u8 tip (0 kısa, 1 uzun)` | Basınca |
| `0x82` | STATUS (ESP32 → Android) | `u8 pil_yüzde, u8 ekran_no` | 0.2 Hz |

**Manevra kodları (`NAV.manevra`):** 0 düz, 1 hafif sağ, 2 sağ, 3 keskin sağ, 4 U dönüşü, 5 hafif sol, 6 sol, 7 keskin sol, 8 dönel kavşak (+ çıkış no), 9 varış.

### 4.4 Yapılacaklar

- [ ] ESP32: NimBLE ile servis, RX'te gelen mesajı tipine göre ayrıştır, LVGL'i ana döngüde güncelle (BLE callback içinden LVGL çağırma).
- [ ] Android: `DashLink` cihazı servis UUID'sine göre tarasın, bağlansın, kopunca yeniden bağlansın.
- [ ] Android: `ObdPoller`'dan gelen veriyi 20 Hz `TELEMETRY` olarak göndersin.
- [ ] ESP32 → Android buton olaylarını işle (müzik oynat/duraklat, arama reddet).

**✅ Faz 4 bitti sayılır:** Motorda gaz verince gidondaki devir halkası gecikmesiz hareket ediyorsa.

---

## Faz 5: Harita

**Amaç:** Ortadaki dairede canlı, yön yukarıda (heading-up) bir harita.
**Süre:** 1–2 hafta

### 5.1 Android tarafı

- [ ] **MapLibre Android SDK** ekle (OpenStreetMap verisi, lisans sorunu yok).
  - Ücretsiz veya düşük maliyetli bir stil kaynağı: MapTiler (ücretsiz katman, API anahtarı gerekir) ya da kendi stilin.
- [ ] `MapSnapshotter` ile ekranda göstermeden harita görüntüsü üret:
  - Boyut: **240×240** (ESP32'de 2 kat büyütülür) veya **300×300**
  - Merkez: GPS konumu, harita yönü: GPS yönü (heading-up)
  - Zoom: hıza göre değişsin (yavaşta 17, hızlıda 15)
  - Sade, yüksek kontrastlı, koyu tema bir stil
- [ ] Rota çizgisini haritaya ekle (Faz 6'daki navigasyondan).
- [ ] Görüntüyü **JPEG kalite ~60** ile sıkıştır. Hedef boyut 8–15 KB.
- [ ] Parçalara böl (MTU − 7 bayt) ve `MAP_CHUNK` olarak gönder.
- [ ] Kare hızı: varsayılan 1 kare/sn, dönüşe yaklaşırken 2–3 kare/sn.

### 5.2 ESP32 tarafı

- [ ] Parçaları PSRAM'deki bir tampona topla. Tüm parçalar gelince JPEG'i çöz (`esp_jpeg` veya TJpgDec).
- [ ] Çözülen görüntüyü LVGL image olarak orta daireye yerleştir. Dairesel kırpma için `radius` ve `clip_corner` stilini kullan.
- [ ] Konum işaretçisini, oku ve hızı haritanın **üstüne ESP32 kendisi çizsin**. Böylece harita yavaş yenilense bile bunlar anlık güncellenir.
- [ ] Eksik kare gelirse bir önceki görüntüyü göstermeye devam et.

**✅ Faz 5 bitti sayılır:** Sürüşte harita saniyede en az 1 kez yenileniyor, yön dönüşlerde doğru çeviriyorsa.

---

## Faz 6: Navigasyon (dönüş bilgisi)

**Amaç:** Google Maps ile navigasyon başlatıldığında dönüş okunun ekrana gelmesi.
**Süre:** 1 hafta

### 6.1 Yöntem: Google Maps bildirimini okumak

Google Maps navigasyon sırasında kalıcı bir bildirim gösterir. Bu bildirimde sonraki dönüşün **ok simgesi**, **mesafesi** ve **cadde adı** bulunur.

- [ ] `NotificationListenerService` ile `com.google.android.apps.maps` bildirimlerini dinle.
- [ ] Bildirim metninden **mesafe** ve **cadde adını** ayrıştır.
- [ ] **Manevra tipi:** bildirimdeki büyük simge (bitmap) ok şeklini gösterir. İki yöntem var:
  1. Simgeyi küçültüp bilinen ok şablonlarıyla karşılaştır (basit hash eşleştirme).
  2. Simgeyi doğrudan küçük bir görüntü olarak ESP32'ye gönder.
  > Başlangıç için 2. yöntem daha kolay, 1. yöntem daha temiz görünür.
- [ ] Kullanıcı ilk açılışta "Bildirim erişimi" iznini vermeli (Ayarlar → Bildirim erişimi).

### 6.2 Rota çizgisi için

Google Maps rotanın geometrisini dışarıya vermez. Haritada rota çizgisi göstermek için:

- **Seçenek A:** Rota çizgisi olmadan, sadece ok ve mesafe ile başla.
- **Seçenek B:** Varış noktasını uygulamada seç, rotayı OSRM veya GraphHopper (ücretsiz/açık kaynak) ile kendin hesaplayıp çiz.

> Önerim: Önce A. B'yi sonra ekleriz.

**✅ Faz 6 bitti sayılır:** Google Maps'te navigasyon başlatınca gidonda doğru ok ve azalan mesafe görünüyorsa.

---

## Faz 7: Bildirimler ve müzik

**Süre:** 3–5 gün

- [ ] **Müzik:** `MediaSessionManager.getActiveSessions()` ile çalan şarkıyı al, `MEDIA` olarak gönder. Butondan gelen olayla `transportControls.play()/pause()/skipToNext()` çağır.
- [ ] **Arama:** `TelephonyManager` / `TelecomManager` ile gelen arama durumunu ve arayanı al. Reddetmek için `TelecomManager.endCall()` (izin gerekir).
- [ ] **Bildirimler:** sadece seçili uygulamalardan (WhatsApp, SMS) ve sadece **başlık** gönder. Sürüşte uzun metin okunmaz.

> **Not:** Ana telefonun iPhone ise bu fazdaki bildirim ve aramalar iPhone'dan gelmez. İki seçenek var:
> - Bu fazı atla, sürüşte bildirim gösterme.
> - ESP32'ye ikinci bir bağlantı olarak iPhone ANCS (bildirim) ve AMS (müzik) desteği ekle.

---

## Faz 8: Donanım, kasa ve montaj

**Süre:** 1–2 hafta

### 8.1 Pil

- [ ] Kartın pil konnektörünün tipini Waveshare pinout şemasından doğrula (büyük ihtimalle MX1.25, 2 pin). **Kutup yönünü kontrol etmeden takma.**
- [ ] Pil süresini ölç: ekran %80 parlaklıkta, BLE bağlı, harita akarken. Hedef: ≥ 6 saat.
- [ ] Bağlantı yoksa 2 dakika sonra ekranı karart, 10 dakika sonra uyku moduna geç.

### 8.2 Kasa (3D baskı)

- [ ] Fusion 360 veya Tinkercad ile tasarla:
  - Ön: ekran camı için yuva, contalı kapak (O-ring veya silikon)
  - Arka: pil bölmesi, USB-C erişimi (tıpalı)
  - Alt: GoPro tipi bağlantı ayağı
  - Yan veya alt: 3 buton yuvası
- [ ] Malzeme: **ASA** (UV ve ısıya dayanıklı) veya **PETG**. PLA güneşte yumuşar, kullanma.
- [ ] Ekranın üstüne güneşlik çıkıntısı ekle (2–3 mm siperlik).

### 8.3 Montaj

- [ ] Hunter'ın gidon çapı: **22 mm (7/8")**. Kelepçeyi buna göre seç.
- [ ] Ekranı gösterge paneline yakın, sürüş pozisyonunda göz hizasına yakın yerleştir.
- [ ] Titreşime karşı: kauçuk ara parça ve kontra somun kullan, kasanın içindeki pil ve kablolar sabitlensin (sünger veya sıcak silikon).

### 8.4 OBD tarafı

- [ ] ELM327'yi koltuk altında sabitle (cırt cırt veya kelepçe).
- [ ] Su ve tozdan korumak için ziplock poşet veya ısıyla daralan makaron kullan.
- [ ] **Akü kontrolü:** ELM327 kontak kapalıyken de akım çekebilir. Bir hafta kullanmayınca aküyü ölç. Gerekirse ELM327'yi kontakla açılan bir hatta al ya da uyku modu olan bir model kullan (Vgate iCar Pro'nun otomatik uyku modu var).

---

## Faz 9: Yol testi

### Test listesi

- [ ] **Okunabilirlik:** öğlen güneşinde, güneş arkadan ve önden gelirken
- [ ] **Bağlantı:** telefon ceket cebinde, pantolon cebinde, çantada. Kopma oluyor mu?
- [ ] **Gecikme:** gaz verince devir halkası ne kadar gecikiyor? (hedef < 150 ms)
- [ ] **Titreşim:** 30 dk sürüş sonrası kasa, pil, butonlar yerinde mi?
- [ ] **Pil:** 2 saatlik sürüşte ekran ve Android pil yüzdesi ne kadar düştü?
- [ ] **Isı:** güneşte park halinde 1 saat. Ekran kararıyor mu, kasa deforme oluyor mu?
- [ ] **Yağmur:** önce hortumla kontrollü test, sonra gerçek yağmur
- [ ] **Navigasyon:** 10 dönüşlük bir rotada okların doğruluğu
- [ ] **Eldiven:** butonlara kalın eldivenle basılabiliyor mu?

### Güvenlik kuralları

- Sürüş sırasında ekranla etkileşimi en aza indir. Butonlar sadece temel işlemler için olsun.
- Ekran asla gerçek gösterge panelini kapatmasın.
- İlk testleri trafiksiz bir alanda yap.

---

## Riskler ve çözümler

| Risk | Olasılık | Etki | Çözüm |
|---|---|---|---|
| Hunter OBD soketi farklı, kablo uymuyor | Orta | Yüksek | Faz 1'de fotoğraf ve multimetre ölçümü, gerekirse kabloyu kendin yap |
| Motor standart PID vermiyor | Düşük | Yüksek | Protokolü elle seç. Olmazsa SN65HVD230 ile doğrudan CAN dinleme |
| ELM327 yavaş, devir göstergesi takılıyor | Orta | Orta | `010C1` hilesi, kaliteli ELM327, ESP32'de animasyonla yumuşatma |
| Android servisi arka planda öldürüyor | Orta | Yüksek | Foreground service, pil optimizasyonu muafiyeti, markaya özel ayarlar |
| IPS ekran güneşte okunmuyor | Orta | Orta | Yüksek kontrastlı tema, güneşlik. Gerekirse AMOLED karta geçiş |
| BLE bağlantısı cepte kopuyor | Düşük | Orta | 2M PHY yerine 1M / Coded PHY, otomatik yeniden bağlanma |
| ELM327 aküyü boşaltıyor | Orta | Orta | Uyku modlu model veya kontakla açılan besleme |
| Google Maps bildirim formatı değişiyor | Orta | Düşük | Ayrıştırıcıyı esnek yaz, OSRM ile kendi rotana geçiş seçeneği |

---

## Açık sorular

- [ ] Hunter 350'deki OBD soketi hangisi? (renk, pin sayısı, fotoğraf)
- [ ] Cepteki Android cihaz ana telefonun mu, ikinci bir cihaz mı?
- [ ] Ana telefonun iPhone mu? (Faz 7'yi etkiler)
- [ ] Hunter 350'nin kırmızı bölge devri (kılavuzdan doğrulanacak)
- [ ] Ekranın gidondaki tam yeri (gösterge panelinin solu, sağı veya üstü)

---

## Kaynaklar

**Ürünler**

- [Motorobit – ESP32-S3 2.8" 480×480 yuvarlak ekran kartı](https://www.motorobit.com/esp32-s3-28-480480-yuvarlak-ekranli-gelistirme-karti)
- [Waveshare – ESP32-S3-LCD-2.8C ürün sayfası](https://waveshare.com/product/iot-communication/short-range-wireless/esp32-s3-touch-lcd-2.8c.htm)
- [Waveshare – ESP32-S3-Touch-AMOLED-2.06 (alternatif)](https://www.waveshare.com/esp32-s3-touch-amoled-2.06.htm?sku=31957)
- [Newegg – RE 6 pin OBD2 adaptör kablo (referans ilan, Euro 4 siyah soket için)](https://www.newegg.com/p/0FB-0ACC-000K7)

**Royal Enfield OBD**

- [Team-BHP – Bullet 350 BS6'ya OBD okuyucu bağlama](https://www.team-bhp.com/news/setting-obd-reader-indian-motorcycles)
- [Team-BHP – Hint motosikletlerinde OBD tartışması](https://www.team-bhp.com/forum/motorbikes/237904-obd-board-diagnostics-indian-motorcycles.html)
- [Reddit r/royalenfield – Meteor 350 OBD soketleri (siyah/kırmızı)](https://pol1.lr.ggtyler.dev/r/royalenfield/comments/1j0uor1/do_you_know_what_this_cable_is)
- [MotorBeam – Hunter 350'nin Meteor ile ortak parçaları](https://motorbeam.com/royal-enfield-hunter-350-to-borrow-meteors-instrument-cluster/amp)

**Teknik dokümanlar**

- [ELM327 datasheet (komutlar ve protokoller)](https://www.scantool.net/scantool/downloads/103/elm327dsh.pdf)
- [Wikipedia – OBD-II PID listesi ve formüller](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [LVGL dokümantasyonu](https://docs.lvgl.io/)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [MapLibre Native Android](https://maplibre.org/maplibre-native/android/api/)
- [Android – NotificationListenerService](https://developer.android.com/reference/android/service/notification/NotificationListenerService)
- [Android – Foreground services](https://developer.android.com/develop/background-work/services/fgs)
