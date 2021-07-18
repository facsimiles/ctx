#include <stdint.h>
#include <cairo.h>

#define CTX_LIMIT_FORMATS   0
#ifdef CTX_AVX2
#undef CTX_AVX2
#endif
#define CTX_AVX2                  1
#define CTX_EVENTS                1
#define CTX_ENABLE_CM             0
#define CTX_PARSER                0
#define CTX_DITHER                0
#define CTX_NATIVE_GRAYA8         0  // the native gray8 is better for code-size, worse for
                                     // performance
#define CTX_FORMATTER             1
#define CTX_RASTERIZER            1
#define CTX_RASTERIZER_AA        15
#define CTX_RASTERIZER_FORCE_AA   0
#define CTX_BITPACK_PACKER        0
#define CTX_AUDIO                 0
#define CTX_RENDERSTREAM_STATIC   0
#define CTX_FONTS_FROM_FILE       1 /* leaves out code */
#define CTX_IMPLEMENTATION
#define NO_LIBCURL
#include <immintrin.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../../deps/stb_image.h"

#include "ctx.h"
#define WIDTH    512
#define HEIGHT   512

#define ITERATIONS     1000

typedef struct Fmt { 
   char *name;
   CtxPixelFormat format;
   int stridemul;
   int stridediv;
} Fmt;

Fmt formats[]={
   {"RGBA8",     CTX_FORMAT_RGBA8,  4, 1},
   {"RGB565",    CTX_FORMAT_RGB565, 2,   1},
   {"BGRA8",     CTX_FORMAT_BGRA8,  4, 1},
   {"GRAYA8",    CTX_FORMAT_GRAYA8, 2, 1},
   {"RGBAF",     CTX_FORMAT_RGBAF,  4*4, 1},
   {"RGB332",    CTX_FORMAT_RGB332, 1,   1},
   {"RGB565_BS", CTX_FORMAT_RGB565_BYTESWAPPED,   2,   1},
   {"GRAYAF",    CTX_FORMAT_GRAYAF, 2*4, 1},
   {"CMYKAF",    CTX_FORMAT_CMYKAF, 5*4, 1},
   {"CMYKA8",    CTX_FORMAT_CMYKA8, 5, 1},
   {"GRAY1",     CTX_FORMAT_GRAY1,  1, 8},
   {"GRAY2",     CTX_FORMAT_GRAY2,  1, 4},
   {"GRAY4",     CTX_FORMAT_GRAY4,  1, 2},
   {"GRAY8",     CTX_FORMAT_GRAY8,  1, 1},
   {"GRAYF",     CTX_FORMAT_GRAYF,  4*1, 1},
   {"RGB8",      CTX_FORMAT_RGB8,   3,   1},
   {"CMYK8",     CTX_FORMAT_CMYK8, 4, 1},
};

static void report_result (int count, float elapsed)
{
  fprintf (stdout, "<td>%1.0f</td>", elapsed * 1000.0 * 1000.0 / ITERATIONS);
 // (1000.0)/(count/elapsed));
}

static void cairo_shape (cairo_t *cr)
{
    cairo_arc (cr, WIDTH/2, HEIGHT/2, HEIGHT/2, 0.0, 6.2);
 // cairo_rectangle (cr, 0,0, WIDTH, HEIGHT);
}

static void ctx_shape (Ctx *ctx)
{
    ctx_arc (ctx, WIDTH/2, HEIGHT/2, HEIGHT/2, 0.0, 6.2, 0);
 // ctx_rectangle (ctx, 0,0, WIDTH, HEIGHT);
}

static void run_test_set_cairo (cairo_t *cr)
  {
  long start;
  int count;
  float elapsed;

  //ctx_blend_mode (ctx, CTX_BLEND_MULTIPLY);
  //ctx_blend_mode (ctx, CTX_BLEND_NORMAL);
  //ctx_compositing_mode (ctx, CTX_COMPOSITE_SOURCE_ATOP);
 // ctx_compositing_mode (ctx, CTX_COMPOSITE_DESTINATION_OVER);
 
  start = ctx_ticks ();
  count = 0;
  do {
    cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
    cairo_shape (cr);
    cairo_fill (cr);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

#if 0
  start = ctx_ticks ();
  count = 0;
  do {
    cairo_shape (cr);
    cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.0);
    cairo_fill (cr);
    count ++;
  } while (count < 1000);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  fprintf (stdout, "<td>%.0f</td>", count/elapsed);
#endif

  start = ctx_ticks ();
  count = 0;
  do {
    cairo_shape (cr);
    cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.75);
    cairo_fill (cr);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

  start = ctx_ticks ();
  count = 0;
  {
    cairo_pattern_t *pat = cairo_pattern_create_linear (30.0, 3.0, 500.0, 500.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.0, 1.0, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 0.4, 0.2, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1.0, 1.0, 1.0, 1.0, 0.5);
    cairo_set_source (cr, pat);
  do {
    cairo_shape (cr);
    cairo_fill (cr);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  }
  report_result (count, elapsed);

  count = 0;
  {
    cairo_pattern_t *pat = cairo_pattern_create_radial (100.0, 100.0, 40.4, 100.0, 100.0, 100.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.0, 1.0, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 0.4, 0.2, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1.0, 1.0, 1.0, 1.0, 0.5);
    cairo_set_source (cr, pat);
  do {
    cairo_shape (cr);
    cairo_fill (cr);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  }
  report_result (count, elapsed);


  start = ctx_ticks ();

  cairo_surface_t *surf =
          cairo_image_surface_create_from_png ("img.png");
  cairo_set_source_rgba (cr ,1,1,1,1);
  cairo_set_source_surface (cr, surf, 0.0, 0.0);

  count = 0;
  do {
    cairo_shape (cr);
    // set texture
    cairo_fill (cr);
    count ++;
  } while (count < 1000);
  elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);


#if 0
  start = ctx_ticks ();
  count = 0;
  do {
    cairo_arc (ctx, WIDTH/2, HEIGHT/2, HEIGHT/2, 0.0, 6.2, 0);
    cairo_linear_gradient (ctx, 30.0, 3.0, 500.0, 500.0);
    cairo_gradient_add_stop (ctx, 0.0, 1.0, 1.0, 1.0, 0.5);
    cairo_gradient_add_stop (ctx, 0.4, 0.2, 1.0, 1.0, 0.5);
    cairo_gradient_add_stop (ctx, 1.0, 1.0, 1.0, 1.0, 0.5);
    cairo_fill (ctx);
    count ++;
  } while (count < 1000);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);


  start = ctx_ticks ();
  count = 0;
  do {
    cairo_arc (ctx, WIDTH/2, HEIGHT/2, HEIGHT/2, 0.0, 6.2, 0);
    cairo_radial_gradient (ctx, 100.0, 100.0, 40.4, 100.0, 100.0, 100.0);
    cairo_gradient_add_stop (ctx, 0.0, 1.0, 1.0, 1.0, 0.5);
    cairo_gradient_add_stop (ctx, 0.4, 0.2, 1.0, 1.0, 0.5);
    cairo_gradient_add_stop (ctx, 1.0, 1.0, 1.0, 1.0, 0.5);
    cairo_fill (ctx);
    count ++;
  } while (count < 1000);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);
#endif
  }


static void run_test_set (Ctx *ctx)
  {
  long start;
  int count;
  float elapsed;

  //ctx_blend_mode (ctx, CTX_BLEND_MULTIPLY);
  //ctx_blend_mode (ctx, CTX_BLEND_NORMAL);
  //ctx_compositing_mode (ctx, CTX_COMPOSITE_SOURCE_ATOP);
  //ctx_compositing_mode (ctx, CTX_COMPOSITE_DESTINATION_OVER);
 
  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_rgba (ctx, 0.5, 0.5, 0.5, 1.0);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);
#if 0
  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_rgba (ctx, 0.5, 0.5, 0.5, 0.0);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  fprintf (stdout, "<td>%.0f</td>", count/elapsed);
#endif

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_rgba (ctx, 0.5, 0.5, 0.5, 0.75);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_linear_gradient (ctx, 30.0, 3.0, 500.0, 500.0);
    ctx_gradient_add_stop (ctx, 0.0, 1.0, 1.0, 1.0, 0.5);
    ctx_gradient_add_stop (ctx, 0.4, 0.2, 1.0, 1.0, 0.5);
    ctx_gradient_add_stop (ctx, 1.0, 1.0, 1.0, 1.0, 0.5);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_radial_gradient (ctx, 100.0, 100.0, 40.4, 100.0, 100.0, 100.0);
    ctx_gradient_add_stop (ctx, 0.0, 1.0, 1.0, 1.0, 0.5);
    ctx_gradient_add_stop (ctx, 0.4, 0.2, 1.0, 1.0, 0.5);
    ctx_gradient_add_stop (ctx, 1.0, 1.0, 1.0, 1.0, 0.5);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

char eid[1024]="";
ctx_rgb (ctx,1,0,0);
//ctx_texture_load (ctx, "file:///home/pippin/src/ctx/tests/test-size/img.png", NULL, NULL, eid);
ctx_texture_load (ctx, "/home/pippin/src/ctx/tests/test-size/img.png", NULL, NULL, eid);
  start = ctx_ticks ();
  count = 0;
  do {
    ctx_shape (ctx);
    ctx_texture (ctx, eid, 0.0, 0.0);
    ctx_fill (ctx);
    count ++;
  } while (count < ITERATIONS);
    elapsed = (ctx_ticks() - start) / 1000.0 / 1000.0;
  report_result (count, elapsed);

  }

static void run_tests (Ctx *ctx)
{
  long start;
  int count;
  float elapsed;

  run_test_set (ctx);
#if 1
  ctx_compositing_mode (ctx, CTX_COMPOSITE_SOURCE_ATOP);
  run_test_set (ctx);
#endif
#if 0
  ctx_blend_mode (ctx, CTX_BLEND_OVERLAY);
  run_test_set (ctx);
#endif

#if 0
  ctx_blend_mode (ctx, CTX_BLEND_OVERLAY);
  ctx_compositing_mode (ctx, CTX_COMPOSITE_SOURCE_ATOP);
  run_test_set (ctx);
#endif
  fprintf (stdout, "</tr>\n");

}

int main (int argc, char **argv)
{
  if (1)
  {
   fprintf (stdout, "<tr><td>cairo</td>");
   int stride = WIDTH * 4;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   //Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, formats[f].format);
   cairo_surface_t *surface =cairo_image_surface_create_for_data (pixels,
                   CAIRO_FORMAT_ARGB32,
                   WIDTH, HEIGHT, stride);
   cairo_t *cr = cairo_create (surface);
   Ctx *ctx = ctx_new_for_cairo (cr);

   run_test_set_cairo (cr);
#if 1
   cairo_set_operator (cr, CAIRO_OPERATOR_ATOP);
   run_test_set_cairo (cr);
#endif
   cairo_destroy (cr);
   cairo_surface_destroy (surface);
   free (pixels);
  fprintf (stdout, "</tr>\n");
  }

  for (int f = 0; f < sizeof(formats)/sizeof(formats[0]); f++)
  {
   fprintf (stdout, "<tr><td>%s</td>", formats[f].name);
   int stride = WIDTH * formats[f].stridemul / formats[f].stridediv;
   uint8_t *pixels = (uint8_t*)calloc (WIDTH,stride);
   Ctx *ctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, stride, formats[f].format);
   run_tests (ctx);
   ctx_free (ctx);
   free (pixels);
  }
  return 0;
}
