#include "s0il.h"

int width = 240;
int height = 240;
uint16_t *pixels = NULL;

int frame_no = -120;

void ctx_set_pixels(void *ctx, void *user_data, int x, int y, int w, int h,
                    void *buf);

static inline uint16_t rgb888_to_rgb565bs(uint8_t red, uint8_t green,
                                          uint8_t blue) {
  uint16_t ret = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
  return (ret >> 8) | (ret << 8); // byteswap
}

Ctx *ctx_host(void);

int ctx_add_idle(Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *data), void *data);

int setpixels_iteration(Ctx *ctx, void *data) {
  if (frame_no < 120) {
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
    frame_no++;
    return 1;
  } else {
    if (pixels)
      free(pixels);
    pixels = NULL;
  }
  return 0;
}

MAIN(demo_setpixels_tsr) {
  Ctx *ctx = ctx_host();
  width = ctx_width(ctx);
  height = ctx_height(ctx);
  frame_no = -120;

  for (int i = 0; !pixels && i < 10; i++) {
    pixels = malloc(width * height * 2);
    if (!pixels) {
      height *= 0.9f;
    }
  }
  if (!pixels)
    return -1;
  ctx_add_idle(ctx, setpixels_iteration, NULL);

  return 42;
}
