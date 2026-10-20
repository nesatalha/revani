// protocol.c birim testleri. Paket örnekleri Kotlin tarafındaki
// ProtocolTest.kt ile aynı: iki uç aynı baytlarda anlaşıyor mu?
#include <stdio.h>
#include <string.h>

#include "protocol.h"

static int failures;
#define CHECK(c)                                                    \
  do {                                                              \
    if (!(c)) {                                                     \
      fprintf(stderr, "%s:%d: başarısız: %s\n", __FILE__, __LINE__, #c); \
      failures++;                                                   \
    }                                                               \
  } while (0)

static struct {
  int nav, nav_end, gps, time;
  proto_nav_t last_nav;
  uint8_t speed;
  uint16_t heading;
  uint32_t unix_time;
  int16_t offset;
} got;

static void on_nav(const proto_nav_t *n, void *ctx) { got.nav++, got.last_nav = *n; }
static void on_nav_end(void *ctx) { got.nav_end++; }
static void on_gps(uint8_t s, uint16_t h, void *ctx) { got.gps++, got.speed = s, got.heading = h; }
static void on_time(uint32_t t, int16_t o, void *ctx) { got.time++, got.unix_time = t, got.offset = o; }

static struct {
  int count;
  uint16_t frame, chunk, chunk_count;
  size_t len;
  uint8_t first;
} map;
static void on_map(uint16_t f, uint16_t c, uint16_t n, const uint8_t *d, size_t len, void *ctx) {
  map.count++, map.frame = f, map.chunk = c, map.chunk_count = n, map.len = len, map.first = len ? d[0] : 0;
}

static const proto_handlers_t handlers = {on_nav, on_nav_end, on_gps, on_time, on_map, NULL};

int main(void) {
  // NAV: sağ, 180 m, 12 dk, "Bağdat Cd."
  const uint8_t nav[] = {0x02, 0x02, 0x00, 0xb4, 0x00, 0x0c, 0x00, 0x0b, 0x42, 0x61,
                         0xc4, 0x9f, 0x64, 0x61, 0x74, 0x20, 0x43, 0x64, 0x2e};
  CHECK(proto_parse(nav, sizeof nav, &handlers) == PROTO_OK);
  CHECK(got.nav == 1);
  CHECK(got.last_nav.maneuver == 2);
  CHECK(got.last_nav.roundabout_exit == 0);
  CHECK(got.last_nav.distance_m == 180);
  CHECK(got.last_nav.eta_min == 12);
  CHECK(strcmp(got.last_nav.street, "Bağdat Cd.") == 0);

  // NAV: dönel kavşak 3. çıkış, boş cadde, en büyük mesafe
  const uint8_t nav2[] = {0x02, 0x08, 0x03, 0xff, 0xff, 0x00, 0x00, 0x00};
  CHECK(proto_parse(nav2, sizeof nav2, &handlers) == PROTO_OK);
  CHECK(got.last_nav.maneuver == 8 && got.last_nav.roundabout_exit == 3);
  CHECK(got.last_nav.distance_m == 65535 && got.last_nav.street[0] == '\0');

  // Cadde uzunluğu veriden uzun: kısa mesaj
  const uint8_t nav_bad[] = {0x02, 0x02, 0x00, 0xb4, 0x00, 0x0c, 0x00, 0x05, 0x41};
  CHECK(proto_parse(nav_bad, sizeof nav_bad, &handlers) == PROTO_ERR_SHORT);
  CHECK(proto_parse(nav, 7, &handlers) == PROTO_ERR_SHORT);
  CHECK(got.nav == 2);

  const uint8_t gps[] = {0x09, 0x2f, 0x0e, 0x01};
  CHECK(proto_parse(gps, sizeof gps, &handlers) == PROTO_OK);
  CHECK(got.speed == 47 && got.heading == 270);
  CHECK(proto_parse(gps, 3, &handlers) == PROTO_ERR_SHORT);

  const uint8_t tm[] = {0x07, 0xc0, 0x57, 0xd7, 0x6a, 0xb4, 0x00};
  CHECK(proto_parse(tm, sizeof tm, &handlers) == PROTO_OK);
  CHECK(got.unix_time == 1792497600u && got.offset == 180);
  const uint8_t tm_neg[] = {0x07, 0, 0, 0, 0, 0xc4, 0xff};
  CHECK(proto_parse(tm_neg, sizeof tm_neg, &handlers) == PROTO_OK);
  CHECK(got.offset == -60);

  const uint8_t end[] = {0x08};
  CHECK(proto_parse(end, 1, &handlers) == PROTO_OK && got.nav_end == 1);

  const uint8_t unknown[] = {0x42};
  CHECK(proto_parse(unknown, 1, &handlers) == PROTO_ERR_UNKNOWN);
  // MAP_CHUNK: kare 258, parça 1/3, 2 bayt veri
  const uint8_t chunk[] = {0x03, 0x02, 0x01, 0x01, 0x00, 0x03, 0x00, 0xff, 0xd8};
  CHECK(proto_parse(chunk, sizeof chunk, &handlers) == PROTO_OK);
  CHECK(map.count == 1 && map.frame == 258 && map.chunk == 1 && map.chunk_count == 3);
  CHECK(map.len == 2 && map.first == 0xff);
  CHECK(proto_parse(chunk, 6, &handlers) == PROTO_ERR_SHORT);

  const uint8_t later[] = {0x01, 0, 0};
  CHECK(proto_parse(later, 3, &handlers) == PROTO_ERR_UNSUPPORTED);
  CHECK(proto_parse(NULL, 0, &handlers) == PROTO_ERR_EMPTY);

  // İşleyicisi NULL olan mesaj sessizce kabul edilir
  const proto_handlers_t none = {0};
  CHECK(proto_parse(nav, sizeof nav, &none) == PROTO_OK);

  uint8_t st[3];
  CHECK(proto_encode_status(st, 85, 1) == 3);
  CHECK(st[0] == 0x82 && st[1] == 85 && st[2] == 1);

  if (failures) {
    fprintf(stderr, "%d test başarısız\n", failures);
    return 1;
  }
  printf("protocol_test: tüm testler geçti\n");
  return 0;
}
