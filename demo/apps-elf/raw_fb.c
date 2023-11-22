#include <stdint.h>
#include <math.h>

void display_set_pixels(int x, int y, int w, int h, void *buf);

static uint16_t pixels[240*240];

static inline uint16_t rgb888_to_rgb565bs(uint8_t red,
                                          uint8_t green,
                                          uint8_t blue)
{
  uint16_t ret = ((red>>3) << 11) | ((green>>2) << 5) | (blue>>3);
  return (ret >> 8) | (ret << 8); // byteswap
}

int main (int argc, char **argv)
{
  for (int frame_no = 0; frame_no < 100; frame_no++)
  {
    int o = 0;
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < 240; x++)
      {
        uint8_t val = hypotf (x-120, y-120);
        pixels[o] = rgb888_to_rgb565bs(val, val, val);
        o++;
      }
#ifndef NATIVE
    display_set_pixels(0,0,240,240,pixels);
#endif
  }
  return 0;
}
