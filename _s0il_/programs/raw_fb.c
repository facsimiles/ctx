#include <stdint.h>
#include <string.h>
#include <math.h>
#include "ui.h"

void ctx_set_pixels(void *ctx, void *user_data, int x, int y, int w, int h, void *buf);

static uint16_t pixels[240*240];

static inline uint16_t rgb888_to_rgb565bs(uint8_t red,
                                          uint8_t green,
                                          uint8_t blue)
{
  uint16_t ret = ((red>>3) << 11) | ((green>>2) << 5) | (blue>>3);
  return (ret >> 8) | (ret << 8); // byteswap
}

MAIN(raw_fb)
{
  for (int frame_no = -120; frame_no < 120; frame_no++)
  {
    int o = 0;
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < 240; x++)
      {
        uint8_t red = x*(y+frame_no)+frame_no;
        uint8_t green = (x+frame_no/4)^(y+frame_no);
        uint8_t blue = x;
        pixels[o] = rgb888_to_rgb565bs(red, green, blue);
        o++;
      }
    ctx_set_pixels(NULL, NULL, 0,0,240,240,pixels);
  }
  return 0;
}
