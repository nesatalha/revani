# Kendi OBD Okuyucumuz: Basit Anlatım

> Bu bir fikir dokümanıdır. Ana plan (`moto-dash-plan.md`) değişmedi.
> Bu yol beğenilirse ana plana eklenecek.

---

## Tek cümleyle

Koltuğun altına **küçük bir kutu** koyuyoruz. Bu kutu motorun beynine "devrin kaç?" diye sürekli soruyor ve cevabı **Bluetooth ile gidondaki ekrana** gönderiyor.

---

## Kimler var?

Sistemde 4 oyuncu var:

| Oyuncu | Nerede | Ne yapar |
|---|---|---|
| 🧠 **Motorun beyni (ECU)** | Motorun içinde, zaten var | Devri, hızı, sıcaklığı bilir. Soran olursa söyler. |
| 📦 **Küçük kutu** (ESP32-C3 + CAN modülü) | Koltuk altında, OBD soketine bağlı | Beyne soru sorar, cevabı ekrana iletir. |
| ⭕ **Yuvarlak ekran** (ESP32-S3 2.8") | Gidonda | Gelen bilgileri çizer. |
| 📱 **Android telefon** | Cebinde | Sadece harita ve navigasyon gönderir. |

---

## Nasıl çalışır? Adım adım

### 1. Kontağı açarsın
Motorun beyni uyanır ve motorun kablolarında konuşmaya başlar. Bu kablolara **CAN hattı** denir. Motorun parçaları birbiriyle bu hat üzerinden haberleşir.

### 2. Küçük kutu uyanır
Kutu, hattaki konuşmayı fark edip uyanır. Bunun için bir düğmeye basman gerekmez.

### 3. Kutu soru sorar
Kutu, hatta motorun beynine standart bir soru gönderir: **"Devrin kaç?"**
Bu soru her araç ve motor için aynıdır, uluslararası bir standarttır (OBD).

### 4. Beyin cevap verir
Beyin birkaç milisaniye içinde cevap verir: **"1726 devir."**

### 5. Kutu bunu saniyede onlarca kez tekrarlar
Kutu soru-cevap işini **saniyede 50 kez** yapar. Arada bir de hızı ve sıcaklığı sorar.

### 6. Kutu cevabı ekrana gönderir
Her cevabı **Bluetooth** ile gidondaki yuvarlak ekrana gönderir. Arada kablo yok.

### 7. Ekran çizer
Yuvarlak ekran gelen devri **dış halkaya** çizer. Gaz verdikçe halka dolar, kırmızı bölgeye yaklaşınca renk değişir.

### 8. Telefon haritayı gönderir
Bu sırada cebindeki telefon da haritayı ve "200 m sonra sağa dön" bilgisini ekrana gönderir. Ekran haritayı **ortaya** koyar.

### 9. Kontağı kapatırsın
Hattaki konuşma susar. Kutu bunu fark eder ve **derin uykuya** geçer. Uykudayken neredeyse hiç elektrik harcamaz, akün boşalmaz.

---

## Resimle

```
   KONTAK AÇIK
       │
       ▼
 🧠 Motorun beyni  ◀── "Devrin kaç?" ──  📦 Küçük kutu
                   ──  "1726"      ──▶       │
                                             │ Bluetooth
                                             ▼
 📱 Telefon ── harita, yön ─────────────▶  ⭕ Gidondaki ekran
                                             dış halka: devir
                                             orta: harita
```

---

## Neden ELM327 yerine bunu düşünüyoruz?

| | ELM327 (hazır cihaz) | Küçük kutu (kendimiz yapıyoruz) |
|---|---|---|
| Fiyat | 700 – 1.500 TL | **300 – 450 TL** |
| Devir göstergesi | Saniyede ~10–20 güncelleme, biraz takılabilir | **Saniyede ~50 güncelleme, akıcı** |
| Telefon gerekir mi? | Evet, veriyi telefon okur | **Hayır**, devir doğrudan ekrana gider |
| Kurulum | Tak, çalışsın | Biraz lehim ve kod gerekir |
| Risk | Çok düşük | Dikkatli olunmazsa motorun hattını rahatsız edebilir |

**Kısacası:** Daha ucuz, daha hızlı ve telefondan bağımsız. Karşılığında biraz daha emek ve dikkat istiyor.

---

## Küçük kutunun içinde ne var?

Sadece 3 ana parça:

1. **ESP32-C3:** kutunun beyni. Soruyu sorar, cevabı Bluetooth ile gönderir. (Bir çakmak boyutunda.)
2. **SN65HVD230 CAN modülü:** tercüman. Motorun hattındaki sinyali ESP32'nin anlayacağı dile çevirir. (Bir tırnak boyutunda.)
3. **12V → 3.3V dönüştürücü:** motorun 12 voltunu kutunun kullanabileceği 3.3 volta düşürür.

Bir de güvenlik için **küçük bir sigorta** ve kutuyu sudan korumak için **makaron veya küçük bir kutu**.

---

## Dikkat etmemiz gereken 3 şey

### 1. Önce sadece dinleyeceğiz
Kutu ilk gün motora **hiçbir şey söylemeyecek**, sadece dinleyecek. Hattın hangi hızda konuştuğunu öğrenince soru sormaya başlayacak. Böylece motorun beynini yanlışlıkla rahatsız etmeyiz.

### 2. Modüldeki küçük bir direnci sökeceğiz
CAN modüllerinin çoğunda hazır bir direnç takılı geliyor. Motorun hattında zaten aynı direnç var. İkincisi hattı bozar, bu yüzden modüldekini sökeceğiz. Bir lehim işi, 1 dakika sürer.

### 3. Kutu uyumayı bilmeli
Kontak kapanınca kutu mutlaka uyumalı. Uyumazsa birkaç günde aküyü boşaltır. Bunu kodla çözeceğiz ve ilk hafta akünün voltajını kontrol edeceğiz.

---

## Sıradaki adım

Her şey **motordaki soketin** ne olduğuna bağlı. Önce soketi bulup fotoğraflıyoruz, sonra multimetreyle hattın **CAN** olduğunu doğruluyoruz. CAN ise bu yol çalışır.

Daha teknik ayrıntılar için: `can-yolu-detayli.md`
