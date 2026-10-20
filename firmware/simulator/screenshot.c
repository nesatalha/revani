#include "screenshot.h"

#include <stdio.h>

static void put_u16(FILE *f, uint16_t v) { fputc(v & 0xFF, f), fputc(v >> 8, f); }
static void put_u32(FILE *f, uint32_t v) { put_u16(f, v & 0xFFFF), put_u16(f, v >> 16); }

bool screenshot_write_bmp(const char *path, const uint32_t *pixels, int w, int h) {
  FILE *f = fopen(path, "wb");
  if (!f) return false;
  int row = (w * 3 + 3) & ~3;
  uint32_t data = (uint32_t)(row * h);

  fputc('B', f), fputc('M', f);
  put_u32(f, 54 + data);
  put_u32(f, 0);
  put_u32(f, 54);
  put_u32(f, 40);
  put_u32(f, (uint32_t)w);
  put_u32(f, (uint32_t)-h);  // yukarıdan aşağı
  put_u16(f, 1);
  put_u16(f, 24);
  put_u32(f, 0);
  put_u32(f, data);
  put_u32(f, 2835), put_u32(f, 2835);
  put_u32(f, 0), put_u32(f, 0);

  for (int y = 0; y < h; y++) {
    int x;
    for (x = 0; x < w; x++) {
      uint32_t p = pixels[y * w + x];
      fputc(p & 0xFF, f), fputc((p >> 8) & 0xFF, f), fputc((p >> 16) & 0xFF, f);
    }
    for (x = w * 3; x < row; x++) fputc(0, f);
  }
  fclose(f);
  return true;
}
