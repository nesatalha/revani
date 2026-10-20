// Revani BLE mesajları (docs/PLAN.md bölüm 4). Kart ve simülatör ortak kullanır;
// Android tarafı mobile/.../ble/Protocol.kt.
//
// Biçim: [tip: 1 bayt][veri], çok baytlı sayılar little-endian.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PROTO_TELEMETRY = 0x01,
  PROTO_NAV = 0x02,
  PROTO_MAP_CHUNK = 0x03,
  PROTO_NOTIFICATION = 0x04,
  PROTO_MEDIA = 0x05,
  PROTO_CALL = 0x06,
  PROTO_TIME = 0x07,
  PROTO_NAV_END = 0x08,
  PROTO_GPS = 0x09,
  PROTO_BUTTON = 0x81,
  PROTO_STATUS = 0x82,
};

#define PROTO_BATTERY_UNKNOWN 0xFF
// MAP_CHUNK: sonuncu hariç her parçada tam bu kadar JPEG baytı var (BLE MTU 517'ye sığar).
#define PROTO_MAP_CHUNK_DATA 480
#define PROTO_MAX_STREET 255

typedef struct {
  uint8_t maneuver;
  uint8_t roundabout_exit;
  uint16_t distance_m;
  uint16_t eta_min;
  char street[PROTO_MAX_STREET + 1];  // NUL ile biten UTF-8
} proto_nav_t;

typedef struct {
  void (*on_nav)(const proto_nav_t *nav, void *ctx);
  void (*on_nav_end)(void *ctx);
  void (*on_gps)(uint8_t speed_kmh, uint16_t heading_deg, void *ctx);
  void (*on_time)(uint32_t unix_time, int16_t utc_offset_min, void *ctx);
  // Harita karesinin bir parçası (Faz 2). data, çağrı süresince geçerli.
  void (*on_map_chunk)(uint16_t frame, uint16_t chunk, uint16_t chunk_count, const uint8_t *data,
                       size_t len, void *ctx);
  void *ctx;
} proto_handlers_t;

typedef enum {
  PROTO_OK = 0,
  PROTO_ERR_EMPTY,
  PROTO_ERR_SHORT,        // tipine göre eksik veri
  PROTO_ERR_UNKNOWN,      // tanınmayan tip
  PROTO_ERR_UNSUPPORTED,  // tanınan ama bu sürümde işlenmeyen tip (sonraki fazlar)
} proto_result_t;

// Android'den gelen bir mesajı çözer ve ilgili işleyiciyi çağırır (NULL olanlar atlanır).
proto_result_t proto_parse(const uint8_t *data, size_t len, const proto_handlers_t *h);

// STATUS mesajını yazar, uzunluğu döner (out en az 3 bayt).
size_t proto_encode_status(uint8_t *out, uint8_t battery_pct, uint8_t screen);

const char *proto_result_str(proto_result_t r);

#ifdef __cplusplus
}
#endif
