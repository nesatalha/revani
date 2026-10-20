#include "ui_map.h"

#include <lvgl.h>
#include <string.h>

#include "protocol.h"
#include "src/libs/tjpgd/tjpgd.h"
#include "ui_internal.h"

#define MAX_CHUNKS ((UI_MAP_MAX_JPEG + PROTO_MAP_CHUNK_DATA - 1) / PROTO_MAP_CHUNK_DATA)
#define WORKBUF_SIZE 4096

static struct {
  uint8_t *jpeg;  // PSRAM'de (büyük malloc)
  int32_t frame;  // birleştirilen kare, yoksa -1
  uint16_t count;
  uint16_t received;
  size_t total;
  uint8_t got[MAX_CHUNKS];
  lv_draw_buf_t *bufs[2];  // çift tampon: biri ekranda, diğerine çözülür
  int front;
  ui_map_stats_t stats;
} m = {.frame = -1};

typedef struct {
  const uint8_t *src;
  size_t len, pos;
  lv_draw_buf_t *dst;
} decode_ctx_t;

static size_t in_func(JDEC *jd, uint8_t *buf, size_t n) {
  decode_ctx_t *c = jd->device;
  if (n > c->len - c->pos) n = c->len - c->pos;
  if (buf) memcpy(buf, c->src + c->pos, n);
  c->pos += n;
  return n;
}

// TJpgDec bloklarını (LVGL'in yapılandırmasında B, G, R sırası) ekranın renk biçimine yazar.
static int out_func(JDEC *jd, void *bitmap, JRECT *r) {
  decode_ctx_t *c = jd->device;
  lv_draw_buf_t *d = c->dst;
  const uint8_t *p = bitmap;
  for (int y = r->top; y <= r->bottom; y++) {
    uint8_t *row = d->data + y * d->header.stride;
    for (int x = r->left; x <= r->right; x++, p += 3) {
      lv_color_t col = lv_color_make(p[2], p[1], p[0]);
      if (d->header.cf == LV_COLOR_FORMAT_RGB565) {
        ((uint16_t *)row)[x] = lv_color_to_u16(col);
      } else {
        ((uint32_t *)row)[x] = lv_color_to_u32(col);
      }
    }
  }
  return 1;
}

static lv_draw_buf_t *ensure_buf(int i, uint16_t w, uint16_t h) {
  lv_draw_buf_t *b = m.bufs[i];
  if (b && b->header.w == w && b->header.h == h) return b;
  if (b) lv_draw_buf_destroy(b);
  lv_color_format_t cf = lv_display_get_color_format(NULL);
  if (cf != LV_COLOR_FORMAT_RGB565) cf = LV_COLOR_FORMAT_XRGB8888;
  m.bufs[i] = lv_draw_buf_create(w, h, cf, 0);
  return m.bufs[i];
}

static void decode_and_show(void) {
  uint32_t t0 = lv_tick_get();
  static uint8_t work[WORKBUF_SIZE];
  decode_ctx_t c = {.src = m.jpeg, .len = m.total};
  JDEC jd;
  if (jd_prepare(&jd, in_func, work, sizeof work, &c) != JDR_OK || jd.width > UI_MAP_MAX_SIZE ||
      jd.height > UI_MAP_MAX_SIZE) {
    m.stats.frames_dropped++;
    return;
  }
  int back = 1 - m.front;
  c.dst = ensure_buf(back, jd.width, jd.height);
  if (!c.dst || jd_decomp(&jd, out_func, 0) != JDR_OK) {
    m.stats.frames_dropped++;
    return;
  }
  m.front = back;
  m.stats.frames_ok++;
  m.stats.last_decode_ms = lv_tick_elaps(t0);
  ui_show_map(c.dst);
}

void ui_map_chunk(uint16_t frame, uint16_t chunk, uint16_t count, const uint8_t *data, size_t len) {
  if (count == 0 || count > MAX_CHUNKS || chunk >= count) return;
  if (len > PROTO_MAP_CHUNK_DATA || (chunk + 1 < count && len != PROTO_MAP_CHUNK_DATA)) return;
  if (!m.jpeg) {
    m.jpeg = lv_malloc(UI_MAP_MAX_JPEG);
    if (!m.jpeg) return;
  }
  if (frame != m.frame) {
    if (m.frame >= 0 && m.received < m.count) m.stats.frames_dropped++;  // yarım kalan kare
    m.frame = frame;
    m.count = count;
    m.received = 0;
    m.total = 0;
    memset(m.got, 0, sizeof m.got);
  }
  if (count != m.count || m.got[chunk]) return;

  memcpy(m.jpeg + (size_t)chunk * PROTO_MAP_CHUNK_DATA, data, len);
  m.got[chunk] = 1;
  m.received++;
  if (chunk + 1 == count) m.total = (size_t)chunk * PROTO_MAP_CHUNK_DATA + len;

  if (m.received == m.count) {
    decode_and_show();
    m.frame = -1;  // aynı kare numarasının tekrarını yeni kare say
  }
}

const ui_map_stats_t *ui_map_stats(void) { return &m.stats; }
