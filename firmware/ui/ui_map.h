// Harita karesi (Faz 2): MAP_CHUNK parçalarını birleştirir, JPEG'i çözer ve
// navigasyon ekranının ortasına verir. Eksik kare atlanır, öncekisi kalır (2.2.5).
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_MAP_MAX_JPEG (64 * 1024)
#define UI_MAP_MAX_SIZE 400  // kenar, piksel

void ui_map_chunk(uint16_t frame, uint16_t chunk, uint16_t chunk_count, const uint8_t *data, size_t len);

typedef struct {
  uint32_t frames_ok;
  uint32_t frames_dropped;  // eksik ya da çözülemeyen
  uint32_t last_decode_ms;
} ui_map_stats_t;

const ui_map_stats_t *ui_map_stats(void);

#ifdef __cplusplus
}
#endif
