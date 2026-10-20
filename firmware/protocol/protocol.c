#include "protocol.h"

#include <string.h>

static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static uint32_t rd_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static proto_result_t parse_nav(const uint8_t *d, size_t len, const proto_handlers_t *h) {
  // u8 manevra, u8 çıkış, u16 mesafe, u16 varış, u8 uzunluk, utf8 cadde
  if (len < 8) return PROTO_ERR_SHORT;
  size_t street_len = d[7];
  if (len < 8 + street_len) return PROTO_ERR_SHORT;
  if (!h->on_nav) return PROTO_OK;

  proto_nav_t nav;
  nav.maneuver = d[1];
  nav.roundabout_exit = d[2];
  nav.distance_m = rd_u16(&d[3]);
  nav.eta_min = rd_u16(&d[5]);
  memcpy(nav.street, &d[8], street_len);
  nav.street[street_len] = '\0';
  h->on_nav(&nav, h->ctx);
  return PROTO_OK;
}

proto_result_t proto_parse(const uint8_t *d, size_t len, const proto_handlers_t *h) {
  if (!d || len == 0) return PROTO_ERR_EMPTY;
  switch (d[0]) {
    case PROTO_NAV:
      return parse_nav(d, len, h);

    case PROTO_NAV_END:
      if (h->on_nav_end) h->on_nav_end(h->ctx);
      return PROTO_OK;

    case PROTO_GPS:  // u8 hız, u16 yön
      if (len < 4) return PROTO_ERR_SHORT;
      if (h->on_gps) h->on_gps(d[1], rd_u16(&d[2]), h->ctx);
      return PROTO_OK;

    case PROTO_TIME:  // u32 unix, i16 utc farkı (dk)
      if (len < 7) return PROTO_ERR_SHORT;
      if (h->on_time) h->on_time(rd_u32(&d[1]), (int16_t)rd_u16(&d[5]), h->ctx);
      return PROTO_OK;

    case PROTO_MAP_CHUNK:  // u16 kare_no, u16 parça_no, u16 parça_sayısı, veri
      if (len < 7) return PROTO_ERR_SHORT;
      if (h->on_map_chunk) h->on_map_chunk(rd_u16(&d[1]), rd_u16(&d[3]), rd_u16(&d[5]), &d[7], len - 7, h->ctx);
      return PROTO_OK;

    case PROTO_TELEMETRY:
    case PROTO_NOTIFICATION:
    case PROTO_MEDIA:
    case PROTO_CALL:
      return PROTO_ERR_UNSUPPORTED;

    default:
      return PROTO_ERR_UNKNOWN;
  }
}

size_t proto_encode_status(uint8_t *out, uint8_t battery_pct, uint8_t screen) {
  out[0] = PROTO_STATUS;
  out[1] = battery_pct;
  out[2] = screen;
  return 3;
}

const char *proto_result_str(proto_result_t r) {
  switch (r) {
    case PROTO_OK: return "ok";
    case PROTO_ERR_EMPTY: return "boş mesaj";
    case PROTO_ERR_SHORT: return "eksik veri";
    case PROTO_ERR_UNKNOWN: return "bilinmeyen tip";
    case PROTO_ERR_UNSUPPORTED: return "desteklenmeyen tip";
  }
  return "?";
}
