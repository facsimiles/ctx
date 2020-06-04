#include <stdlib.h>
#include <sys/time.h>
#include <mrg.h>

//#define UPNG_IMPLEMENTATION
//#include "upng.h"
//#define CTX_FULL_CB 1
//#define STB_TRUETYPE_IMPLEMENTATION
//#include "stb_truetype.h"
#include "ctx-font-regular.h"

//#define CTX_MAX_JOURNAL_SIZE 1024 * 15
//#define CTX_MIN_JOURNAL_SIZE CTX_MAX_JOURNAL_SIZE 
#define CTX_BACKEND_FONT 0
#define CTX_BITPACK_PACKER 0
#define CTX_SHAPE_CACHE 0
#define CTX_SHAPE_CACHE_DIM 32*32
#define CTX_SHAPE_CACHE_ENTRIES 1024
#define CTX_IMPLEMENTATION
#include "ctx.h"

#define WIDTH    400
#define HEIGHT   300

//uint8_t pixels[WIDTH*HEIGHT*4];
uint8_t *pixels = NULL;

int frame = -200;

static void tiger (Ctx *ctx);

static void render_test (Ctx *ctx)
{
    frame ++;
    if (frame > 1000)
      frame=-1000;
    ctx_clear (ctx);
    ctx_translate (ctx, frame/3.0, 0.0);
    //ctx_scale (ctx, 0.3f, 0.3f);
  ctx_set_font (ctx, "italic");
    //ctx_rotate (ctx, -frame * 0.05);
    //ctx_scale (ctx, 1 - frame * 0.001, 1 - frame * 0.001);
    //ctx_scale (ctx, 1 - frame * 0.001, 1 - frame * 0.001);
    //ctx_translate (ctx, frame / 8.0, 0);//frame/8.0);
    //ctx_scale (ctx, 1.2, 1.2);
#if 0

  if (1)
  for (int i = 0; i < 1; i++)
  {
    ctx_save (ctx);
    ctx_translate (ctx, 30.0 + i * 131, 30.0);
    //ctx_rotate (ctx, 0.15);
    //ctx_scale (ctx, 0.8, 0.8);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 0.0, 0.0);
    ctx_line_to (ctx, 4.0, -4.0);
    ctx_line_to (ctx, 45.0, 0.0);
    ctx_line_to (ctx, 55.0, 25.0);
    ctx_line_to (ctx, 65.0, -25.0);
    ctx_line_to (ctx, 75.0, -4.0);
    ctx_line_to (ctx, 100.0, -4.0);
    ctx_line_to (ctx, 100.0, 40.0);
    ctx_line_to (ctx, 120.0, 47.0);
    ctx_line_to (ctx,  80.0, 50.0);
    ctx_line_to (ctx, 100.0, 74.0);
    ctx_line_to (ctx, 110.0, 110.0);
    ctx_line_to (ctx, 70.0, 100.0);
    ctx_line_to (ctx, 50.0, 140.0);
    ctx_line_to (ctx, 30.0, 80.0);
    ctx_line_to (ctx, 20.0, 107.0);
    ctx_line_to (ctx, 0.0, 104.0);
    ctx_line_to (ctx, 0.0, 95.0);
    ctx_line_to (ctx, 0.0, 90.0);
    ctx_line_to (ctx, 0.0, 80.0);
    ctx_line_to (ctx, 15.0, 70.0);
    ctx_line_to (ctx, -15.0, 60.0);
    ctx_line_to (ctx, 0.0, 30.0);
    ctx_line_to (ctx, 0.0, 24.0);  // neighboring
    ctx_line_to (ctx, 20.0, 23.0); // neighboring
    ctx_line_to (ctx, 0.0, 20.0);
    ctx_line_to (ctx, 0.0, 10.0);

//  ctx_curve_to (ctx, 10.0, 30.0, 1,1,3,3);
    ctx_set_rgba8 (ctx, 255, 255, 0, 255);
    ctx_close_path (ctx);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }

 if(1) {
    ctx_save (ctx);
    ctx_translate (ctx, 200.0, 30.0);
    //ctx_rotate (ctx, 0.15);
    //ctx_scale (ctx, 0.8, 0.8);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 0.0, 50.0);
    ctx_line_to (ctx, 50.0, 0.0);
    ctx_line_to (ctx, 100.0, 30.0);
    ctx_line_to (ctx, 50.0, 100.0);
    ctx_set_rgba8 (ctx, 255, 255, 0, 255);
    ctx_close_path (ctx);
    ctx_fill (ctx);
    ctx_restore (ctx);
 }

  if(1){
    ctx_save (ctx);
    ctx_translate (ctx, 350.0, 30.0);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 100.0,   50.0);
    ctx_line_to (ctx, 50.0,   0.0);
    ctx_line_to (ctx, 0.0, 30.0);
    ctx_line_to (ctx, 50.0, 100.0);
    ctx_set_rgba8 (ctx, 255, 255, 0, 255);
    ctx_close_path (ctx);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }


  if(1){
    ctx_save (ctx);
    ctx_translate (ctx, 350.0, 150.0);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 50.0,   50.0);
    ctx_rel_line_to (ctx, 25, 25.0);
    ctx_rel_line_to (ctx, 0,  -70.0);
    ctx_rel_line_to (ctx, -50,  0.0);
    ctx_rel_line_to (ctx, 0,   70.0);
    ctx_set_rgba8 (ctx, 255, 255, 0, 255);
    ctx_close_path (ctx);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }


  if(1){
    ctx_save (ctx);
    ctx_translate (ctx, 350.0, 210.0);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 50.0,   50.0);
    ctx_rel_line_to (ctx, 25,  -25.0);
    ctx_rel_line_to (ctx, 0,   70.0);
    ctx_rel_line_to (ctx, -50,  0.0);
    ctx_rel_line_to (ctx, 0,   -70.0);
    ctx_set_rgba8 (ctx, 255, 25, 0, 255);
    ctx_close_path (ctx);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }
#endif

  if(1){
    ctx_save (ctx);
    ctx_translate (ctx, 500.0, 210.0);
    ctx_new_path (ctx);
    ctx_move_to (ctx, 50.0,   50.0);
    ctx_rel_line_to (ctx, 40,0);
    ctx_rel_line_to (ctx, 0,-40);
    ctx_rel_line_to (ctx, -40,0);
    ctx_rel_line_to (ctx, 0,-40);
    ctx_rel_line_to (ctx, -40,0);
    ctx_rel_line_to (ctx, 0,40);
    ctx_rel_line_to (ctx, -40,0);
    ctx_rel_line_to (ctx, 0,40);
    ctx_rel_line_to (ctx, 40,0);
    ctx_rel_line_to (ctx, 0,40);
    ctx_rel_line_to (ctx, 40,0);
    ctx_set_rgba8 (ctx, 255, 25, 0, 255);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }

#if 1
  ctx_set_rgba8 (ctx, 0, 0, 0, 255);
    ctx_move_to (ctx, 54, 50);
    ctx_set_font_size (ctx, 32);
    ctx_text (ctx, "æøåabcdefghijklmnopqrstuvwxyz\n");
    ctx_move_to (ctx, 54, 82);
    ctx_text (ctx, "AvBCDEFGHIJKLMNOPQRSTUVWXYZ\n");
    ctx_move_to (ctx, 54, 82+32);
    ctx_text (ctx, "GRAY8 GRAYA8 RGB8 RGBA8 BGRA8 GRAYF RGBAF\n");
    ctx_move_to (ctx, 54, 82+32+32);
    ctx_text (ctx, "this is some text, with more\nduplicated chars\n");
    ctx_move_to (ctx, 10, 300);
    ctx_set_font_size (ctx, 200);
    ctx_text (ctx, "beziers");
    ctx_move_to (ctx, 10, 440);
    ctx_text (ctx, "æøå\nßñ€");
    ctx_move_to (ctx, 44, 120);

  ctx_set_font_size (ctx, 100);

    ctx_set_rgba8 (ctx, 0, 0, 0, 20);
    for (int o=0;o<10;o++)
    {
      ctx_move_to (ctx, 45 + o, 532 + o);
      ctx_text (ctx, "alpha\nhandling");
    }


#endif

  //ctx_set_rgba8 (ctx, 255,0,0,111);
  //ctx_rectangle (ctx, 22, 22, 222, 122);
  //ctx_fill (ctx);

#if 1

  ctx_save (ctx);
  ctx_translate (ctx, WIDTH*0.66, HEIGHT/2);
  ctx_scale (ctx, 0.3, 0.3);
  tiger (ctx);
  ctx_restore (ctx);

#endif


#if 1
  ctx_set_rgba8 (ctx, 255,0,0,111);
  ctx_linear_gradient (ctx, 10.0, 11.0, 320.0, 240.0);
  ctx_gradient_add_stop_u8 (ctx, 0.0f, 0,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1.0f, 255,255,255,255);

  ctx_set_line_width(ctx, 15);
  ctx_set_line_cap (ctx, CTX_CAP_ROUND);
#if 1
  ctx_move_to (ctx, 40, 40);
  ctx_line_to (ctx, 500, 420);
  ctx_line_to (ctx, 50, 400);
  ctx_line_to (ctx, 50, 200);
  ctx_line_to (ctx, 30, 420);
  ctx_line_to (ctx, 400, 10);
  ctx_move_to (ctx, 600, 600);
  ctx_line_to (ctx, 300, 300);
  ctx_rel_line_to (ctx, 400, 0);
  ctx_curve_to (ctx, 330, 40,
                     320, 70,
                     120, 176);
#else
  ctx_rectangle (ctx, 100, 100, 200, 300);
#endif
  ctx_stroke (ctx);

#endif
  ctx_radial_gradient (ctx, 128.0, 128.0, 100.0,
                            128.0, 128.0, 140.0);
  ctx_gradient_add_stop_u8 (ctx, 0.0f, 255,0,255,0);
  ctx_gradient_add_stop_u8 (ctx, 0.1f, 255,0,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.2, 0,0,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.35, 0,255,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.4, 0,255,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.5, 0,255,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.6, 255,255,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.7, 255,235,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.95, 255,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1.0, 0,0,0,0);

  ctx_arc (ctx, 128.0, 128.0, 140, 0, 2 * CTX_PI, 0);
  ctx_fill (ctx);

  ctx_rectangle (ctx, 0, 0, 100, 100);
  ctx_image_path (ctx, "vw.png", 20, 20);
  ctx_fill (ctx);

  ctx_set_font (ctx, "mono");
  ctx_set_font_size (ctx, 48);
  ctx_move_to (ctx, 2, 32);
  ctx_set_rgba (ctx, 0,0,1,1);
  ctx_text (ctx, "mono");
  ctx_move_to (ctx, 2, 64);
  ctx_set_font (ctx, "bitmap");
  ctx_text (ctx, "bitmap");

  if(1)for (int y = 100; y < 150; y++)
    for (int x = 100; x < 150; x++)
      {
#if 0
        ctx_set_rgba (ctx, 1.0, 0.5, 0.0 + (x-100.0)/25.0, 1.0);
        ctx_rectangle (ctx, x, y, 1, 1);
        ctx_fill (ctx);
#else
	ctx_set_pixel_u8 (ctx, x, y, 255, 0.5*255, 0.0 + (x-100.0)/25.0*255, 255);
#endif
      }

}

typedef void (*RenderTest)(Ctx *ctx);

RenderTest tests[]={
  render_test,
};

int test_no = 0;

static struct timeval start_time;

#define usecs(time)    ((time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)

static void
init_ticks (void)
{
  static int done = 0;

  if (done)
    return;
  done = 1;
  gettimeofday (&start_time, NULL);
}
static inline long
ticks (void)
{
  struct timeval measure_time;
  init_ticks ();
  gettimeofday (&measure_time, NULL);
  return usecs (measure_time) - usecs (start_time);
}

#include "tiger.inc"

static void tiger (Ctx *ctx)
{
   for (int i = 0; i < sizeof(tiger_commands)/sizeof(tiger_commands[0]); i++) {
     const struct command *cmd = &tiger_commands[i];
     switch (cmd->type) {
        case 'm':
        ctx_move_to (ctx, cmd->x0, cmd->y0);
        break;
        case 'l':
        ctx_line_to (ctx, cmd->x0, cmd->y0);
        break;
        case 'c':
        ctx_curve_to (ctx,
                        cmd->x0, cmd->y0,
                        cmd->x1, cmd->y1,
                        cmd->x2, cmd->y2);
                        break;
        case 'f':
          ctx_set_rgba8 (ctx, cmd->x0*255, cmd->y0*255, cmd->x1*255, cmd->y1*255);
          ctx_fill (ctx);
          break;
     }
   }
}

static void clear ()
{
  for (int i = 0; i < WIDTH*HEIGHT; i++)
  {
    pixels[i * 4 + 0] = 255;
    pixels[i * 4 + 1] = 255;
    pixels[i * 4 + 2] = 255;
    pixels[i * 4 + 3] = 255;
  }
}



static void ui (Mrg *mrg, void *data)
{
  cairo_t *cr = mrg_cr (mrg);
  Ctx *ctx = data;
  clear();
  long int start;

  render_test (ctx);

  //printf (" ctx_new+ ctx_blit \n");
  start = ticks ();
  ctx_blit (ctx, pixels, 0, 0, WIDTH, HEIGHT, WIDTH*4, CTX_FORMAT_BGRA8);
  int ctx_time = ticks()-start;

  cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels,
     CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
  cairo_set_source_surface (cr, surface, WIDTH, 0);
  cairo_paint (cr);

  cairo_surface_destroy (surface);

#if 0
  clear();
  start = ticks ();
  {
    Ctx *ctx = ctx_new ();
    ctx->transformation = CTX_TRANSFORMATION_SCREEN_SPACE;
       //CTX_TRANSFORMATION_RELATIVE     = 2,
    render_test (ctx);
    ctx_blit (ctx, pixels, 0, 0, WIDTH, HEIGHT, WIDTH*4, CTX_FORMAT_BGRA8);
    ctx_free (ctx);
  }

  int ctx_nonrel_time = ticks()-start;
  surface = cairo_image_surface_create_for_data (pixels,
     CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
  cairo_set_source_surface (cr, surface, 0, HEIGHT);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
#endif

  clear();
  start = ticks ();
  if(1){
    Ctx *dctx = ctx_new_for_framebuffer (pixels, WIDTH, HEIGHT, WIDTH*4, CTX_FORMAT_BGRA8);
    ctx_render_ctx (ctx, dctx);
    ctx_free (dctx);
  }

  int ctx_framebuffer_time = ticks()-start;
  surface = cairo_image_surface_create_for_data (pixels,
     CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
  cairo_set_source_surface (cr, surface, WIDTH, HEIGHT);
  cairo_paint (cr);

  //printf ("render_ctx | ");
  clear();
  {
    Ctx *dctx = ctx_new ();
    //ctx->transformation = CTX_TRANSFORMATION_SCREEN_SPACE;
       //CTX_TRANSFORMATION_RELATIVE     = 2,
  //cairo_set_source_rgba(cr, 1,1,1,1);
  //cairo_paint (cr);
  //ctx_render_cairo (ctx, cr);
  start = ticks ();
    //ctx_render_ctx (ctx, dctx);
    ctx_blit (ctx, pixels, 0, 0, WIDTH, HEIGHT, WIDTH*4, CTX_FORMAT_BGRA8);
    ctx_free (dctx);
  }
  int ctx_render_ctx_time = ticks()-start;
  surface = cairo_image_surface_create_for_data (pixels,
     CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
  cairo_set_source_surface (cr, surface, 0, HEIGHT*2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);

  //printf ("chunks 60, ctx_blit from render..\n");
  clear();
  {
    Ctx *dctx = ctx_new ();
    render_test (dctx);
  start = ticks ();
    //ctx_render_ctx (ctx, dctx);
    for (int y = 0; y < HEIGHT-2; y+=HEIGHT/5)
    {
// 1: 7.3
// 2: 9.2
// 3: 11.
// 4: 12..5
// 5: 
// 6: 16

      ctx_blit (dctx, pixels + y * WIDTH*4, 0, y, WIDTH, HEIGHT/5, WIDTH*4, CTX_FORMAT_BGRA8);
    }
    ctx_free (dctx);
  }
  int chunk_time = (ticks()-start);
  surface = cairo_image_surface_create_for_data (pixels,
     CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
  cairo_set_source_surface (cr, surface, WIDTH, HEIGHT *2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);

  cairo_save (cr);
  cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
  cairo_clip (cr);
  start = ticks ();
  ctx_render_cairo (ctx, cr);
  int cairo_time = (ticks()-start);
  cairo_restore (cr);

  // we only take into account renderings of less than 30ms ..
  // something weird is going on with mrg and idle time

    fprintf (stderr, "  cairo: %2.2fms ctx: %2.2fms -- ctx-frambuffer: %2.2f render-ctx: %2.2f  chunk: %2.2f\r",
         cairo_time / 1000.0,
         ctx_time / 1000.0,
         // ctx_nonrel_time / 1000.0,
         ctx_framebuffer_time / 1000.0,
         ctx_render_ctx_time / 1000.0,
         chunk_time / 1000.0);

  mrg_queue_draw (mrg, NULL);

#if 0
  cairo_move_to (cr, 10, 20);
  cairo_line_to (cr, 100, 20);
  cairo_line_to (cr, 100, 120);
  cairo_line_to (cr, 10, 120);
  cairo_set_source_rgb (cr, 1.0,0,0);
  cairo_fill (cr);
#endif
}



void ctx_set_full_cb (Ctx *ctx, CtxFullCb cb, void *data);
static void full_cb (CtxRenderstream *rs, void *data)
{
  Ctx *ctx = data;
  fprintf (stderr, "!\n");
  rs->count = 0;
}

int main (int argc, char **argv)
{
  putenv ("MRG_BACKEND=mmm");
  pixels = malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new ();

//  ctx_set_full_cb (ctx, full_cb, ctx);

  Mrg *mrg = mrg_new (WIDTH*2, HEIGHT*3, NULL);
  mrg_set_ui (mrg, ui, ctx);

  //ctx_load_ttf (ctx, "regular", "Vera.ttf");

  mrg_main (mrg);


  ctx_free (ctx);

  return 0;
}

// cairo            ctx blit
// non-relative     ctx drive
//
//
