// ui/ modülleri arasında paylaşılan iç fonksiyonlar (dışarıya açık değil).
#pragma once

#include <lvgl.h>

// Çözülmüş harita karesini ekrana koyar (ui.c). buf ekranda kaldığı sürece geçerli olmalı.
void ui_show_map(lv_draw_buf_t *buf);
