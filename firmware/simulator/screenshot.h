#pragma once

#include <stdbool.h>
#include <stdint.h>

// XRGB8888 tamponu 24 bit BMP olarak yazar.
bool screenshot_write_bmp(const char *path, const uint32_t *pixels, int w, int h);
