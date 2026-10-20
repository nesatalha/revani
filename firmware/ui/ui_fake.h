// Sahte veri üreteci: telefon yokken arayüzü canlı göstermek için (simülatör
// ve kartta demo). BLE'nin çağıracağı ui_* fonksiyonlarını aynı şekilde çağırır.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// speedup: 1 gerçek zaman, 4 ise dört kat hızlı (GPS yine her saniye gelir).
void ui_fake_start(int speedup);
void ui_fake_stop(void);

#ifdef __cplusplus
}
#endif
