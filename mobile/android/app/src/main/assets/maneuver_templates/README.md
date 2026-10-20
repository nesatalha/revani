# Manevra şablonları

Google Maps navigasyon bildiriminin büyük simgeleri (108×108, beyaz ok, saydam zemin).
`ManeuverTemplates` dosya adından okur: `<manevra_kodu>_<çıkış_no>_<serbest>.png`.

Toplama: uygulama tanınmayan simgeleri telefonda `files/maneuver_icons/` altına
(yanında bildirim metniyle) kaydeder. Çekmek için:

```bash
adb shell run-as com.nesatalha.revani ls files/maneuver_icons
adb exec-out run-as com.nesatalha.revani cat files/maneuver_icons/<ad>.png > <ad>.png
```

| Dosya | Manevra | Kaynak |
|---|---|---|
| `0_0_duz_baslangic.png` | 0 düz (rota başı, kesikli) | "Head northeast", S22, 2026-10-20 |
| `1_0_hafif_sag.png` | 1 hafif sağ | emülatör (Medium Phone, API 36), rota oynatma |
| `2_0_sag.png` | 2 sağ | S22, 2026-10-20 |
| `3_0_keskin_sag.png` | 3 keskin sağ | emülatör, rota oynatma |
| `5_0_hafif_sol.png` | 5 hafif sol | S22, 2026-10-20 |
| `6_0_sol.png` | 6 sol | S22, 2026-10-20 |
| `7_0_keskin_sol_ayna.png` | 7 keskin sol | `3_0_keskin_sag` aynası (hafif sağ aynası ile gerçek hafif sol arası fark 1, yani Maps simgeleri simetrik) |

Eksik: düz (normal), U dönüşü, dönel kavşak (çıkışlarıyla), varış.

Emülatörde toplama: Extended Controls → Location → Routes ile rota oynat, Maps arka planda
olsun (ön plandayken navigasyon bildirimi yayınlamıyor) ve Maps'in bildirim izni açık olsun.
