#include <stdint.h>

#include "ctx-font-ascii.h"
#define CTX_LIMIT_FORMATS   0
#define CTX_EVENTS          1
#define CTX_ENABLE_CM       0
#define CTX_PARSER          0
#define CTX_DITHER          0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_FORMATTER       1
#define CTX_SIMD            1
#define CTX_BITPACK_PACKER  0
#define CTX_GRADIENT_CACHE  1
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_IMPLEMENTATION

#include "ctx.h"

#define WIDTH    1024
#define HEIGHT   1024

#define MEASURE_SECS   2.0
//#define MEASURE_SECS   0.25

static void run_tests (Ctx *ctx)
  {

  char *utf8 = "tinytest\necho foobaz\n";
  ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

  long start;
  int count;
  float elapsed;
 
  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 1);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "color alpha 1.0 fills: %.1f\n", count/elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.75);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "color alpha 0.8 fills: %.1f\n", count/elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.5);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "color alpha 0.5 fills: %.1f\n", count/elapsed);


  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.25);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "color alpha 0.25 fills: %.1f\n", count/elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.0);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "color alpha 0.0 fills: %.1f\n", count/elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_linear_gradient (ctx, 30.0, 3.0, 500.0, 500.0);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "lgrad fills: %.1f\n", count/elapsed);


  start = ctx_ticks ();
  count = 0;
  do {
    ctx_rectangle (ctx, 10.5, 10.5, 800, 800);
    ctx_radial_gradient (ctx, 100.0, 100.0, 40.4, 100.0, 100.0, 100.0);
    ctx_fill (ctx);
    count ++;
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  } while (elapsed < MEASURE_SECS);
  fprintf (stdout, "rgrad fills: %.1f\n", count/elapsed);

  }

int main (int argc, char **argv)
{
  {
   fprintf (stdout, "RGB8\n");
   int stride = WIDTH * 3;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_RGB8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "RGBA8\n");
   int stride = WIDTH * 4;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_RGBA8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "RGBAF\n");
   int stride = WIDTH * 4 * sizeof (float);
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_RGBAF);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "GRAY1\n");
   int stride = WIDTH / 8;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_GRAY1);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "GRAY8\n");
   int stride = WIDTH * 1;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_GRAY8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "GRAYA8\n");
   int stride = WIDTH * 2;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_GRAYA8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "GRAYAF\n");
   int stride = WIDTH * 2 * sizeof (float);
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_GRAYAF);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "CMYKAF\n");
   int stride = WIDTH * 5 * sizeof (float);
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_CMYKAF);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }
  {
   fprintf (stdout, "CMYKA8\n");
   int stride = WIDTH * 5;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_CMYKA8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }

  {
   fprintf (stdout, "CMYK8\n");
   int stride = WIDTH * 4;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, CTX_FORMAT_CMYK8);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }


  return 0;
}
