#include <stdint.h>

#include "ctx-font-ascii.h"
#define CTX_LIMIT_FORMATS   0
#define CTX_EVENTS          1
#define CTX_ENABLE_CM       1
#define CTX_PARSER          0
#define CTX_DITHER          0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_FORMATTER       1
#define CTX_BITPACK_PACKER  0
#define CTX_GRADIENT_CACHE  0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_IMPLEMENTATION

#include "ctx.h"

#define WIDTH    1024
#define HEIGHT   1024

#define MEASURE_SECS   2.0
//#define MEASURE_SECS   0.25

int main (int argc, char **argv)
{
  {
  fprintf (stdout, "RGBA8\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_RGBA8);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }


  {
  fprintf (stdout, "RGBAF\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4*4,
    CTX_FORMAT_RGBAF);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }

  {
  fprintf (stdout, "CMYKAF\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*5*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*5*4,
    CTX_FORMAT_CMYKAF);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }


  {
  fprintf (stdout, "CMYKA8\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*5);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*5,
    CTX_FORMAT_CMYKA8);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }

  {
  fprintf (stdout, "CMYK8\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_CMYK8);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }


  {
  fprintf (stdout, "GRAYF\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_GRAYF);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }

  {
  fprintf (stdout, "GRAYAF\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4*2);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4 * 2,
    CTX_FORMAT_GRAYAF);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }

  {
  fprintf (stdout, "GRAYA8\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*2);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH* 2,
    CTX_FORMAT_GRAYA8);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }


  {
  fprintf (stdout, "GRAY8\n");
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH,
    CTX_FORMAT_GRAY8);

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
    ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 0.8);
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

  ctx_free (ctx);
  free (pixels);
  }
  return 0;
}
