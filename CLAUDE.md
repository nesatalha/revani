# Revani

Royal Enfield Hunter 350 için yuvarlak gidon ekranı. Proje dili Türkçe.

## Plan takibi

- Tek doğru kaynak: `docs/PLAN.md`.
- Bir adım tamamlandığında aynı değişiklikte:
  1. İlgili maddeyi `[ ]` → `[x]` yap.
  2. "İlerleme" tablosunda o fazın sayacını ve durum simgesini güncelle (⬜ Başlamadı · 🟨 Devam ediyor · ✅ Tamamlandı · ⏸️ Ertelendi).
  3. Plandan sapma, yeni karar veya yeni adım varsa "Değişiklik günlüğü"ne tarihli bir satır ekle ve gerekirse adımı plana ekle.
  4. Satın alınan parçaları alışveriş listesinde `[x]` yap.
- `docs/legacy/` altındaki dosyalar arşivdir, değiştirilmez.

## Mimari

- `firmware/`: ESP32-S3 (PlatformIO, LVGL 9, NimBLE). Arayüz `firmware/ui/` altında, simülatörle ortak.
- `mobile/`: Flutter projesi. İş mantığı ve arka plan işleri (BLE, bildirim okuma, GPS, harita) Kotlin servisinde; Flutter sadece arayüz ve kanal sarmalayıcıları.
- BLE protokolü `docs/PLAN.md` bölüm 4'te tanımlı; değişirse orayı da güncelle.
