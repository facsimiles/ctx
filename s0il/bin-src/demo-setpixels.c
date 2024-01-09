#include "s0il.h"

void ctx_set_pixels(void *ctx, void *user_data, int x, int y, int w, int h,
                    void *buf);

static inline uint16_t rgb888_to_rgb565bs(uint8_t red, uint8_t green,
                                          uint8_t blue) {
  uint16_t ret = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
  return (ret >> 8) | (ret << 8); // byteswap
}

Ctx *ctx_host(void);

MAIN(demo_setpixels) {
  int width = ctx_width(ctx_host());
  int height = ctx_height(ctx_host());
  uint16_t *pixels = NULL;
  for (int i = 0; !pixels && i < 10; i++) {
    pixels = malloc(width * height * 2);
    if (!pixels) {
      height *= 0.9f;
    }
  }
  if (!pixels)
    return -1;
  for (int frame_no = -120; frame_no < 120; frame_no++) {
    int o = 0;
    for (int y = 0; y < height; y++)
      for (int x = 0; x < width; x++) {
        uint8_t red = x * (y + frame_no) + frame_no;
        uint8_t green = (x + frame_no / 4) ^ (y + frame_no);
        uint8_t blue = x;
        pixels[o] = rgb888_to_rgb565bs(red, green, blue);
        o++;
      }
    ctx_set_pixels(ctx_host(), NULL, 0, 0, width, height, pixels);
  }
  free(pixels);
  return 0;
}
