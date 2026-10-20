# Kendi OBD Okuyucumuz: Detaylı Anlatım

> Bu bir fikir dokümanıdır. Ana plan (`moto-dash-plan.md`) değişmedi.
> Basit anlatım için: `can-yolu-basit.md`

---

## 1. Genel bakış

ELM327 yerine, OBD soketine bağlanan **kendi CAN okuyucumuzu** yapıyoruz. Bu okuyucuya bundan sonra **OBD düğümü** diyeceğiz.

```
┌───────────────── Hunter 350 ─────────────────┐
│                                              │
│  ECU ═══════ CAN hattı (500 kbps) ═══════╗   │
│  ABS ════════════════════════════════════╣   │
│  Gösterge paneli ════════════════════════╣   │
│                                          ║   │
│                                     OBD soketi
└──────────────────────────────────────────╫───┘
                                           ║
                              ┌────────────╨────────────┐
                              │ OBD DÜĞÜMÜ (koltuk altı)│
                              │ SN65HVD230 + ESP32-C3   │
                              └────────────┬────────────┘
                                           │ BLE (telemetri, ~50 Hz)
                                           ▼
┌──────────────────┐  BLE (harita,   ┌─────────────────────────┐
│ Android (cepte)  │ ──────────────▶ │ EKRAN (gidonda)         │
│ harita, navigasyon│  nav, müzik)   │ ESP32-S3 2.8" yuvarlak  │
│ müzik, bildirim  │ ◀────────────── │ dış halka: devir        │
└──────────────────┘   buton olayı   │ orta: harita            │
                                     └─────────────────────────┘
```

**ELM327'li plandan farkı:** devir verisi artık telefondan geçmiyor. OBD düğümü doğrudan ekrana gönderiyor. Telefon sadece harita, navigasyon ve medya işini yapıyor.

---

## 2. Donanım

### 2.1 Parçalar

| Parça | Görevi | Not |
|---|---|---|
| **ESP32-C3 SuperMini** | CAN denetleyicisi (TWAI) ve BLE | ESP32-C6 de olur. İkisinde de dahili TWAI var. |
| **SN65HVD230 modülü** | CAN alıcı-verici (3.3V) | Modüldeki 120 Ω direnç **sökülecek** |
| **Mini buck dönüştürücü** (MP1584 vb.) | 12V → 3.3V | Giriş en az 24V'a dayanıklı olsun (motor elektriğinde ani yükselmeler olur) |
| **Sigorta** (1A, hat üstü) | Kısa devre koruması | 12V hattına, soketten hemen sonra |
| **TVS diyot** (SMBJ18A vb.), isteğe bağlı | Gerilim dalgalanmalarından koruma | 12V girişine |
| Dönüştürücü kablo veya konnektör | Motor soketine bağlantı | Faz 1'deki fotoğrafa göre seçilecek |
| Makaron ve küçük kutu | Su ve titreşim koruması | |

### 2.2 Kablolama

```
Motor OBD soketi            Sigorta    Buck            ESP32-C3        SN65HVD230
────────────────            ───────    ────            ────────        ──────────
+12V  ────────────────────── [1A] ──── VIN
                                       VOUT 3.3V ───── 3V3 ─────────── VCC
GND   ──────────────────────────────── GND ─────────── GND ─────────── GND
                                                       GPIO 4 (TX) ─── CTX (D)
                                                       GPIO 5 (RX) ─── CRX (R)
CAN High ──────────────────────────────────────────────────────────── CANH
CAN Low  ──────────────────────────────────────────────────────────── CANL
```

> GPIO numaraları örnek. ESP32-C3'te TWAI pinleri istenen GPIO'ya atanabilir.
> Pin 4 ve 5 seçildi çünkü ESP32-C3'te derin uykudan uyandırma GPIO 0–5 ile yapılabiliyor.

### 2.3 120 Ω direnç neden sökülüyor?

- CAN hattının **iki ucunda** birer 120 Ω direnç olur (toplam ~60 Ω). Bu direnç sinyal yansımalarını engeller.
- Motorun hattı zaten bu şekilde sonlandırılmış.
- Modüldeki direnç de takılı kalırsa hat ~40 Ω'a düşer. Sinyal zayıflar, iletişim hataları başlayabilir.
- **Kontrol:** modülde "R1" veya "120" yazan küçük SMD direnci sök. Ya da multimetreyle CANH–CANL arasını ölç: modül tek başındayken ~120 Ω okuyorsan direnç takılı demektir.

---

## 3. CAN hattı nasıl çalışır (kısaca)

- **İki tel:** CAN High ve CAN Low. Bilgi, bu iki tel arasındaki **voltaj farkıyla** taşınır. Bu yüzden parazite dayanıklıdır.
- **Hız (bitrate):** hattaki herkes aynı hızda konuşmalıdır. OBD standardında genelde **500 kbps**.
- **Mesaj (frame):** her mesajın bir **ID'si** (11 bit, örneğin `0x7DF`) ve en fazla **8 bayt verisi** var.
- **Yayın mantığı:** bir mesaj gönderildiğinde hattaki herkes duyar, ilgilenen dinler.

### 3.1 OBD soru-cevap mesajları

**Soru (OBD düğümü → ECU):**

```
ID:   0x7DF                     (fonksiyonel adres = "tüm ECU'lar, beni duyan cevap versin")
Veri: 02 01 0C 00 00 00 00 00
      │  │  │
      │  │  └─ PID 0C = devir
      │  └──── Mod 01 = canlı veri iste
      └─────── 2 bayt anlamlı veri var
```

**Cevap (ECU → OBD düğümü):**

```
ID:   0x7E8                     (motor ECU'sunun cevap adresi)
Veri: 04 41 0C 1A F8 00 00 00
      │  │  │  │  │
      │  │  │  └──┴─ A = 0x1A (26), B = 0xF8 (248)
      │  │  └─────── PID 0C
      │  └────────── 0x41 = "Mod 01'e cevap"
      └───────────── 4 bayt anlamlı veri

Devir = (A × 256 + B) / 4 = (26 × 256 + 248) / 4 = 1726 rpm
```

### 3.2 Okunacak PID'ler

| PID | Veri | Formül | Sorgu sıklığı |
|---|---|---|---|
| `0C` | Devir | `(A*256 + B) / 4` | ~50 Hz |
| `0D` | Hız (km/h) | `A` | 10 Hz |
| `11` | Gaz kelebeği (%) | `A * 100 / 255` | 10 Hz |
| `05` | Motor sıcaklığı (°C) | `A - 40` | 1 Hz |
| `42` | Akü voltajı (V) | `(A*256 + B) / 1000` | 1 Hz |

> Hunter hava/yağ soğutmalı olduğu için `05` desteklenmeyebilir veya yağ sıcaklığını gösterebilir. Desteklenen PID'leri `01 00`, `01 20`, `01 40` sorgularıyla öğreniriz.

---

## 4. Yazılım: adım adım ne oluyor?

### 4.1 OBD düğümünün yaşam döngüsü

```
       ┌──────────────┐
       │  DERİN UYKU  │ ◀─────────────────────────────┐
       └──────┬───────┘                               │
              │ CAN RX pininde hareket (kontak açıldı) │
              ▼                                       │
       ┌──────────────┐                               │
       │   UYANMA     │ TWAI'yi başlat                │
       └──────┬───────┘                               │
              ▼                                       │
       ┌──────────────┐                               │
       │ DİNLEME MODU │ listen-only, 500 kbps         │
       │ (ilk açılış) │ hatada mesaj var mı?          │
       └──────┬───────┘                               │
              │ geçerli mesajlar görüldü              │
              ▼                                       │
       ┌──────────────┐                               │
       │ NORMAL MOD   │ PID sorgu döngüsü             │
       │ + BLE yayını │ ekrana telemetri gönder       │
       └──────┬───────┘                               │
              │ 3 sn boyunca hatta hiç mesaj yok       │
              │ (kontak kapandı)                      │
              └───────────────────────────────────────┘
```

### 4.2 Adım adım

**Adım 1: Uyanma**
- ESP32-C3 derin uykudayken CAN RX pinini izler.
- Kontak açılınca ECU hatta konuşmaya başlar. SN65HVD230'un RX çıkışı değişir, ESP32 uyanır.
- Uyanma süresi yaklaşık 200–400 ms.

**Adım 2: Güvenli başlangıç (dinleme modu)**
- TWAI sürücüsü önce **listen-only** modunda, 500 kbps ile başlatılır. Bu modda ESP32 hatta **hiçbir bit göndermez**, onay (ACK) bile vermez.
- 1–2 saniye boyunca hatasız mesaj geliyorsa hız doğrudur.
- Hata geliyorsa 250 kbps denenir. İkisinde de hata varsa sorgu modu açılmaz, ekrana "OBD yok" bilgisi gönderilir.
- Bu kontrol sadece ilk kurulumda gerekli. Doğru hız bulunduktan sonra kalıcı belleğe (NVS) kaydedilir.

**Adım 3: Normal mod ve sorgu döngüsü**
- TWAI **normal** moda geçer.
- Döngü örneği (her adım ~20 ms):

```
0C → 0D → 0C → 11 → 0C → 0D → 0C → 05 → 0C → 0D → 0C → 42 → (başa dön)
```

- Her sorgudan sonra en fazla 50 ms cevap beklenir. Cevap gelmezse o PID atlanır.
- Devir böylece ~25–50 Hz, diğerleri daha seyrek güncellenir.

**Adım 4: Ekrana gönderme (BLE)**
- OBD düğümü bir BLE **peripheral** olarak yayın yapar.
- Gidondaki ESP32-S3 ona **central** olarak bağlanır. (ESP32-S3 aynı anda Android'e karşı peripheral, OBD düğümüne karşı central olabilir. NimBLE bunu destekliyor.)
- Her yeni değer küçük bir paket olarak **notify** ile gönderilir:

```
[0x01][rpm: u16][hız: u8][gaz: u8][sıcaklık: i8][voltaj_mV: u16][bayraklar: u8]
= 9 bayt
```

**Adım 5: Ekranda çizim**
- Ekran gelen devri doğrudan iğneye/halkaya yazmaz, 30–50 ms içinde **yumuşak geçişle** hedefe götürür.
- Bağlantı 1 saniyeden uzun koparsa halka gri olur, "OBD bağlantısı yok" gösterilir.

**Adım 6: Kapanış**
- Kontak kapanınca hat susar.
- 3 saniye boyunca hiç CAN mesajı görülmezse:
  1. Ekrana "kapanıyor" paketi gönderilir.
  2. SN65HVD230 **bekleme moduna** alınır (Rs pini yüksek; bu modda alıcı açık kalır, uyandırma çalışır).
  3. ESP32-C3 **derin uykuya** geçer.

### 4.3 Uyku tüketimi

| Parça | Uykuda tüketim (yaklaşık) |
|---|---|
| ESP32-C3 derin uyku | ~5–10 µA (SuperMini kartındaki LED ve regülatör yüzünden daha fazla olabilir) |
| SN65HVD230 bekleme modu | ~0.4 mA |
| Buck dönüştürücü boşta | ~0.1–1 mA |
| **Toplam** | **~1–2 mA** |

Hunter'ın aküsü ~8 Ah. 2 mA tüketimle aküyü %50'ye düşürmek için yaklaşık **80 gün** gerekir. Pratikte sorun değil.

> **Daha basit alternatif:** OBD düğümünü kontakla açılan bir 12V hattından beslemek. Bu durumda uyku mantığına hiç gerek kalmaz, ama motora ek bir kablo bağlantısı gerekir.

---

## 5. Gecikme hesabı

Gaz verdiğin andan ekranın değiştiği ana kadar:

| Adım | Süre |
|---|---|
| ECU'nun devri hesaplaması | ~5–10 ms |
| Sorgu + cevap (CAN) | ~2–5 ms |
| Döngüde sıra bekleme | 0–40 ms |
| BLE iletimi | ~7–15 ms |
| Ekranda çizim ve yumuşatma | ~16–50 ms |
| **Toplam** | **~30–120 ms** |

ELM327 + Android yolunda aynı süre ~150–300 ms. Bu yol göz için **anlık** sayılır.

---

## 6. Güvenlik

| Risk | Önlem |
|---|---|
| Yanlış hızda mesaj göndermek hatta hata oluşturur, panelde uyarı çıkabilir | Önce listen-only, hız doğrulanmadan sorgu yok |
| Hattı mesajla boğmak | Aynı anda tek sorgu, cevap gelmeden yenisi yok, en fazla ~50 sorgu/sn |
| Mod 01 dışındaki komutlar ECU'yu etkileyebilir | Sadece **Mod 01 (okuma)** kullanılacak. Hata silme (Mod 04) gibi komutlar **kodda olmayacak** |
| Kısa devre | Sigorta, ters polarite koruması, makaron |
| Gerilim dalgalanmaları | 24V+ dayanıklı buck, isteğe bağlı TVS diyot |
| Su | Makaron, küçük kutu, kablo girişinde silikon |

---

## 7. ELM327 yolu ile karşılaştırma

| | ELM327 + Android | Kendi OBD düğümümüz |
|---|---|---|
| Maliyet | 700–1.500 TL | ~300–450 TL |
| Devir güncelleme hızı | ~10–20 Hz | ~25–50 Hz |
| Gecikme | ~150–300 ms | ~30–120 ms |
| Telefon olmadan devir | ❌ | ✅ |
| Kurulum zorluğu | Tak-çalıştır | Lehim, kablolama, kod |
| İlk test kolaylığı | Car Scanner uygulamasıyla 5 dk | Kendi test kodumuzu yazmamız gerekir |
| Motor hattına risk | Çok düşük (test edilmiş ürün) | Düşük (önlemler alınırsa) |
| Hat CAN değilse | ELM327 çoğu protokolü destekler | Çalışmaz, farklı çip gerekir (K-line için L9637D) |
| İleride vites gibi özel verileri çözmek | Zor | Mümkün (hattı dinleyerek) |

---

## 8. Bu yolu seçersek iş sırası

1. **Soket kontrolü:** fotoğraf ve multimetre. Hattın **CAN** olduğunu doğrula (CANH ~2.5–3.5 V, CANL ~1.5–2.5 V).
2. **Masada test:** ESP32-C3 ve SN65HVD230'u breadboard'da kur, USB'den besle. Kod derlenip çalışıyor mu?
3. **İlk motor testi (sadece dinleme):** dizüstü bilgisayardan USB ile besle, listen-only modda hattaki mesajları seri porta yazdır. Hiçbir şey gönderme.
4. **İlk sorgu:** tek bir `01 0C` sorgusu gönder, cevabı oku. Panelde uyarı çıkıyor mu, kontrol et.
5. **Sorgu döngüsü:** tüm PID'leri döngüye al, değerleri seri porta yazdır.
6. **12V besleme:** buck ve sigortayı ekle, motordan besle.
7. **Uyku ve uyanma:** kontak aç/kapa testleri, uyku akımını multimetreyle ölç.
8. **BLE:** ekran kartına bağlan, devir halkasını canlı oynat.
9. **Paketleme:** makaron, kutu, koltuk altına sabitleme.
10. **Akü takibi:** ilk hafta her gün akü voltajını ölç.

---

## 9. Kararı etkileyecek sorular

- Hunter'ın OBD soketi **CAN** mı? (Faz 1 ölçümü)
- Lehim yapmak sana uygun mu?
- Devir göstergesinin **telefon olmadan** da çalışması senin için önemli mi?
- İleride vites göstergesi gibi standart dışı verileri çözmek ister misin?

Bu soruların çoğuna "evet" diyorsan bu yol daha iyi. Hızlıca ve dertsiz başlamak istiyorsan ELM327 yolu daha güvenli.
