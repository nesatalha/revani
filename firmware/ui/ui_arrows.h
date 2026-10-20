// Dönüş oku ikonları. Tuvale çizilir, sadece manevra değişince yeniden çizilir.
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_ARROW_SIZE 160

lv_obj_t *ui_arrow_create(lv_obj_t *parent);
void ui_arrow_set(lv_obj_t *arrow, uint8_t maneuver, uint8_t roundabout_exit);

#ifdef __cplusplus
}
#endif
