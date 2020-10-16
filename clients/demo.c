#include <math.h>
#include "ctx.h"

int frame_no = 0;
float width = 10;
float height = 10;

typedef void (*Test)(Ctx *ctx, int frame_no);

#if 0
static const signed char *rocket_op (Ctx *ctx, const signed char *g)
{
  switch (*g++) {

    case ' ': ctx_new_path (ctx); break;
    /* all of these path commands are directly in integer coordinates */
    case 'm':
      ctx_move_to  (ctx, g[0], g[1]); g += 2;
break;
    case 'l': ctx_line_to  (ctx, g[0], g[1]); g += 2; break;
    case 'c': ctx_curve_to (ctx, g[0], g[1], g[2], g[3], g[4], g[5]); g += 6; break;
    case 'z': ctx_close_path (ctx); break;

    case 'g': ctx_rgba8 (ctx, g[0]*10, g[0]*10, g[0]*10, 255); g ++; break;

    case 'f': ctx_fill (ctx); break;
    case 's': break;
    default:
    case '\0':
    case '.':  /* end */
      return NULL;
    }
  return g;
}

static void
render_rocket (Ctx *ctx, float x, float y, float scale, float angle)
{
static signed char rakett[] = {
  ' ',
  'm',38,6,
  'c',38,6,36,13,36,15,
  'c',24,22,23,26,21,32,'c',19,41,23,61,23,61,'c',15,73,14,95,17,110,'l',26,109,'c',26,102,26,87,30,83,'c',30,83,30,88,30,95,'c',31,103,31,108,31,108,'l',36,108,'c',36,108,35,98,36,91,'c',37,83,38,80,38,80,'c',41,79,43,80,47,79,'c',56,85,56,89,58,99,'c',58,103,58,108,58,108,'l',68,108,'c',67,89,69,73,54,58,'c',54,58,56,41,53,31,'c',50,21,40,15,40,15,'l',38,6,'z','g',100,'f','g',100,'s',
  ' ',
  'm',33,20,'c',31,20,29,21,27,22,'c',25,24,23,27,22,29,'c',20,35,21,38,21,38,'c',26,38,29,36,34,33,'c',38,31,42,24,34,21,'c',34,21,33,20,33,20,'z','g', 50,'f','.'
};
  ctx_save (ctx);
  ctx_global_alpha (ctx, 0.8);
  for (const signed char *g = rakett; g; g = rocket_op (ctx, g));
  ctx_restore (ctx);
}
#endif

static void snippet_card10 (Ctx *ctx, int frame_no)
{
  ctx_save (ctx);
  ctx_translate (ctx, width/2, height/2);
  ctx_scale (ctx, 1.0 + frame_no / 1000.0, 1.0 + frame_no / 1000.0);
  ctx_translate (ctx, -width/2, -height/2);
  ctx_parse (ctx, "M 78.6,74 112,39 c 2.6,-3.15 5.4,-8.9 3.6,-18 -1.6,-4.6 -7.36,-14.8 -19.6,-14.9 -9.9,-0.0 -16,8 -16,8.1 0,0 -6.0,-8.7 -17.2,-8.57 C 51.3,5.5 44.6,12.75 42.8,19.1 c -1.74,6.3 0.34,14.7 4.8,19.74 ");
  ctx_linear_gradient (ctx, 44, 0, 121, 0);
  ctx_gradient_add_stop_u8 (ctx, 0.0f, 255,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.1f, 255,0,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.13f, 255,0,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.25, 0,0,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.35, 0,255,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.4, 0,255,255,255);
  ctx_gradient_add_stop_u8 (ctx, 0.5, 0,255,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.6, 255,255,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.7, 255,235,0,255);
  ctx_gradient_add_stop_u8 (ctx, 0.95, 255,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1.0, 255,0,0,255);
  ctx_fill (ctx);
  ctx_parse (ctx, "M 48.5,38 l 16.578324,0 8.5,-13.25 9.5,25 10.3,-17.7 21,0");
  ctx_rgba (ctx, 1,0,0,1);
  ctx_line_width (ctx, 4.0);
  ctx_stroke (ctx);
  ctx_restore (ctx);
}

static void snippet_arc (Ctx *ctx, int frame_no)
{
  frame_no = frame_no % 300;

  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 0.33, 0.33);
  float xc = 128.0;
  float yc = 128.0;
  float radius = 100.0;
  float angle1 = (15.0 + frame_no/2)  * (CTX_PI/180.0);  /* angles are specified */
  float angle2 = 180.0 * (CTX_PI/180.0);  /* in radians           */

  ctx_line_width (ctx, 10.0);
  ctx_arc (ctx, xc, yc, radius, angle1, angle2, 0);
  ctx_stroke (ctx);

  /* draw helping lines */
  ctx_rgba (ctx, 1, 0.2, 0.2, 0.6);
  ctx_line_width (ctx, 6.0);

  ctx_arc (ctx, xc, yc, 10.0, 0, 2*CTX_PI, 0);
  ctx_fill (ctx);

  ctx_move_to (ctx, xc, yc);
  ctx_arc (ctx, xc, yc, radius, angle1, angle1, 0);
  ctx_move_to (ctx, xc, yc);
  ctx_arc (ctx, xc, yc, radius, angle2, angle2, 0);
  ctx_stroke (ctx);
}

static void snippet_arc_negative (Ctx *ctx, int frame_no)
{
  frame_no = frame_no % 300;

  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 0.33, 0.33);
  float xc = 128.0;
  float yc = 128.0;
  float radius = 100.0;
  float angle1 = (15.0 +frame_no/2)  * (CTX_PI/180.0);  /* angles are specified */
  float angle2 = 180.0 * (CTX_PI/180.0);  /* in radians           */

  ctx_line_width (ctx, 10.0);
  ctx_arc (ctx, xc, yc, radius, angle1, angle2, 1);
  ctx_stroke (ctx);

  /* draw helping lines */
  ctx_rgba (ctx, 1, 0.2, 0.2, 0.6);
  ctx_line_width (ctx, 6.0);

  ctx_arc (ctx, xc, yc, 10.0, 0, 2*CTX_PI, 0);
  ctx_fill (ctx);

  ctx_move_to (ctx, xc, yc);
  ctx_arc (ctx, xc, yc, radius, angle1, angle1, 0);
  ctx_move_to (ctx, xc, yc);
  ctx_arc (ctx, xc, yc, radius, angle2, angle2, 0);
  ctx_stroke (ctx);
}

static void snippet_curve_to (Ctx *ctx, int frame_no)
{
  frame_no %= 400;
  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 0.33, 0.33);
  float x=25.6,  y=128.0;
  float x1=102.4 + frame_no - 102, y1=230.4,
        x2=153.6, y2=25.6 + frame_no,
        x3=230.4, y3=128.0;

  ctx_move_to (ctx, x, y);
  ctx_curve_to (ctx, x1, y1, x2, y2, x3, y3);

  ctx_line_width (ctx, 10.0);
  ctx_stroke (ctx);

  ctx_rgba (ctx, 1, 0.2, 0.2, 0.6);
  ctx_line_width (ctx, 6.0);
  ctx_move_to (ctx,x,y);   ctx_line_to (ctx,x1,y1);
  ctx_move_to (ctx,x2,y2); ctx_line_to (ctx,x3,y3);
  ctx_stroke (ctx);
}


static void snippet_dash (Ctx *ctx, int frame_no)
{
#if 0
  float dashes[] = {50.0,  /* ink */
                    10.0,  /* skip */
                    10.0,  /* ink */
                    10.0   /* skip*/
                   };
  int    ndash  = sizeof (dashes)/sizeof(dashes[0]);
  float offset = -50.0;
  ctx_dash (ctx, dashes, ndash, offset);
#endif

  ctx_scale (ctx, 0.33, 0.33);
  ctx_line_width (ctx, 10.0);

  ctx_move_to (ctx, 128.0, 25.6);
  ctx_line_to (ctx, 230.4, 230.4);
  ctx_rel_line_to (ctx, -102.4, 0.0);
  ctx_curve_to (ctx, 51.2, 230.4, 51.2, 128.0, 128.0, 128.0);

  ctx_stroke (ctx);
}


static void snippet_fill_rule (Ctx *ctx, int frame_no)
{
  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 2.4, 2.4);
  ctx_line_width (ctx, 6);

  ctx_arc (ctx, 64, 64, 40, 0, 1.9*CTX_PI, 0);
  ctx_close_path (ctx);
  ctx_arc (ctx, 192, 64, 40, 0, -1.9*CTX_PI, 1);
  ctx_close_path (ctx);
  ctx_rectangle (ctx, 12, 12, 232, 70);

  ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
  ctx_rgba (ctx, 0, 0.7, 0, 1);
  ctx_preserve (ctx);
  ctx_fill (ctx);
  ctx_rgba (ctx, 1, 1, 1, 1); ctx_stroke (ctx);

  ctx_translate (ctx, 0, 128);
  ctx_arc (ctx, 64, 64, 40, 0, 1.9*CTX_PI, 0);
  ctx_close_path (ctx);
  ctx_arc (ctx, 192, 64, 40, 0, -1.9*CTX_PI, 1);
  ctx_close_path (ctx);
  ctx_rectangle (ctx, 12, 12, 232, 70);

  ctx_fill_rule (ctx, CTX_FILL_RULE_WINDING);
  ctx_rgba (ctx, 0, 0, 0.9, 1);
  ctx_preserve (ctx);
  ctx_fill (ctx);
  ctx_rgba (ctx, 1, 1, 1, 1); ctx_stroke (ctx);
}

static void snippet_gradient (Ctx *ctx, int frame_no)
{
  frame_no %= 256;

ctx_linear_gradient (ctx, 0.0, 0.0,  0.0, height);
ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
ctx_rectangle (ctx, 0, 0, width, height);
ctx_fill (ctx);

ctx_radial_gradient (ctx, width * 0.4 + frame_no, height * 0.4, height * 0.1,
                          width * 0.4 + frame_no, height * 0.4, height * 0.4);
ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
ctx_arc (ctx, width/2 + frame_no, height/2, height * 0.3, 0, 1.9 * CTX_PI, 0);
ctx_fill (ctx);
}

static void snippet_image (Ctx *ctx, int frame_no)
{
#if 0
int              w, h;
ctx_surface_t *image;

image = ctx_image_surface_create_from_png ("data/romedalen.png");
w = ctx_image_surface_get_width (image);
h = ctx_image_surface_get_height (image);

ctx_translate (ctx, 128.0, 128.0);
ctx_rotate (ctx, 45* CTX_PI/180);
ctx_scale  (ctx, 256.0/w, 256.0/h);
ctx_translate (ctx, -0.5*w, -0.5*h);

ctx_source_surface (ctx, image, 0, 0);
ctx_paint (ctx);
ctx_surface_destroy (image);
#endif
}

static void snippet_image_pattern (Ctx *Ctx, int frame_no)
{
#if 0
int              w, h;
ctx_surface_t *image;
ctx_pattern_t *pattern;
ctx_matrix_t   matrix;

image = ctx_image_surface_create_from_png ("data/romedalen.png");
w = ctx_image_surface_get_width (image);
h = ctx_image_surface_get_height (image);

pattern = ctx_pattern_create_for_surface (image);
ctx_pattern_extend (pattern, CAIRO_EXTEND_REPEAT);

ctx_translate (ctx, 128.0, 128.0);
ctx_rotate (ctx, CTX_PI / 4);
ctx_scale (ctx, 1 / sqrt (2), 1 / sqrt (2));
ctx_translate (ctx, -128.0, -128.0);

ctx_matrix_init_scale (&matrix, w/256.0 * 5.0, h/256.0 * 5.0);
ctx_pattern_matrix (pattern, &matrix);

ctx_source (ctx, pattern);

ctx_rectangle (ctx, 0, 0, 256.0, 256.0);
ctx_fill (ctx);

ctx_pattern_destroy (pattern);
ctx_surface_destroy (image);
#endif
}


static void snippet_multi_segment_caps (Ctx *ctx, int frame_no)
{
  ctx_scale (ctx, 0.33, 0.33);

  ctx_move_to (ctx, 50.0, 75.0);
  ctx_line_to (ctx, 200.0, 75.0);

  ctx_move_to (ctx, 50.0, 125.0);
  ctx_line_to (ctx, 200.0, 125.0);

  ctx_move_to (ctx, 50.0, 175.0);
  ctx_line_to (ctx, 200.0, 175.0);

  ctx_line_width (ctx, 30.0);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  ctx_stroke (ctx);
}

static void slider (Ctx *ctx, float x0, float y0, float width, float pos)
{
  ctx_gray (ctx, 1.0);
  ctx_line_width (ctx, height * 0.025);
  ctx_move_to (ctx, x0, y0);
  ctx_line_to (ctx, x0 + width, y0);
  ctx_stroke (ctx);
  ctx_arc (ctx, x0 + width * pos, y0, height * 0.05, 0.0, CTX_PI*1.95, 0);
  ctx_fill (ctx);
}

static void snippet_rounded_rectangle (Ctx *ctx, int frame_no)
{
  int frame_no_b = frame_no % 330;
  frame_no = frame_no % 400;
  /* a custom shape that could be wrapped in a function */

  ctx_round_rectangle (ctx, height * 0.1, height * 0.1,
                            width - height * 0.2,
                            height - height * 0.2,
                            height * 0.1);
  ctx_rgba (ctx, 0.5, 0.5, 1, 1);
  ctx_fill (ctx);

  slider (ctx, height * 0.2, height * 0.4, width - height * 0.4, (frame_no  % 400) / 400.0);
  slider (ctx, height * 0.2, height * 0.5, width - height * 0.4, (frame_no  % 330) / 330.0);
  slider (ctx, height * 0.2, height * 0.6, width - height * 0.4, (frame_no  % 100) / 100.0);
}

static void snippet_text (Ctx *ctx, int frame_no)
{
  frame_no = frame_no % 200;

  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 0.1 + frame_no/100.0, 0.1 + frame_no / 100.0);
  ctx_font_size (ctx, height*0.2);

  ctx_rgba (ctx, 1.0, 0.5, 1, 1);
  ctx_move_to (ctx, height * 0.1 + frame_no * height * 0.001, height * 0.4);
  ctx_text (ctx, "Hello");

  ctx_move_to (ctx, height * 0.1 + frame_no * height * 0.001, height * 0.8);
  ctx_rgba (ctx, 0.5, 0.5, 1, 1);
  ctx_text (ctx, "void");

  ctx_move_to (ctx, height * 0.1 + frame_no * height * 0.001, height * 0.3);
  ctx_rgba (ctx, 0, 0, 0, 1);
  ctx_line_width (ctx, 2.56);
  ctx_text_stroke (ctx, "void");
}


static void scope (Ctx *ctx, int frame_no)
{
  ctx_rgba8 (ctx, 255,0,0,255);
  ctx_line_width (ctx, CTX_LINE_WIDTH_FAST);
  for (int i = 0; i < 180; i++)
  {
    float x = i;
    float y = height*0.5f*ctx_sinf ((x+frame_no) / 10.0)+height/2;
    if (i == 0)
      ctx_move_to (ctx, x, y);
    else
      ctx_line_to (ctx, x, y);
  }
  ctx_stroke (ctx);
}

static void scope2 (Ctx *ctx, int frame_no)
{
  ctx_rgba8 (ctx, 255,0,0,255);
  ctx_line_width (ctx, CTX_LINE_WIDTH_FAST);
  for (int i = 0; i < 180; i++)
  {
    float x = i;
    float y = height / 2 *
      ctx_sinf ((x+frame_no)/7.0f) * ctx_cosf((x+frame_no)/3.0f) + height/2;
    if (i == 0)
      ctx_move_to (ctx, x, y);
    else
      ctx_line_to (ctx, x, y);
  }
  ctx_stroke (ctx);
}

static void scope_aa (Ctx *ctx, int frame_no)
{
  ctx_rgba8 (ctx, 255,0,0,255);
  ctx_line_width (ctx, 1);
  for (int i = 0; i < 180; i++)
  {
    float x = i;
    float y = height*0.5f*ctx_sinf ((x+frame_no) / 10.0)+height/2;
    if (i == 0)
      ctx_move_to (ctx, x, y);
    else
      ctx_line_to (ctx, x, y);
  }
  ctx_stroke (ctx);
}


static void scope2_aa (Ctx *ctx, int frame_no)
{
  ctx_rgba8 (ctx, 255,0,0,255);
  ctx_line_width (ctx, 1);
  for (int i = 0; i < 180; i++)
  {
    float x = i;
    float y = height / 2 *
      ctx_sinf ((x+frame_no)/7.0f) * ctx_cosf((x+frame_no)/3.0f) + height/2;
    if (i == 0)
      ctx_move_to (ctx, x, y);
    else
      ctx_line_to (ctx, x, y);
  }
  ctx_stroke (ctx);
}

static void _analog_clock (Ctx     *ctx,
                           uint64_t ms,
                           float    x,
                           float    y,
                           float    radius,
                           int smoothstep)
{
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;

  float r;
  ctx_save (ctx);

  ctx_rgba8 (ctx, 255, 255, 255, 196);

#if 0
  ctx_set_rgba_u8 (ctx, 127, 127, 127, 255);
  ctx_move_to (ctx, x, y);
  ctx_arc (ctx, x, y, radius * 0.9, 0.0, CTX_PI * 2, 0);
  ctx_set_line_width (ctx, radius * 0.2);
  ctx_set_line_cap (ctx, CTX_CAP_NONE);
  ctx_stroke (ctx);
#else
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  for (int markh = 0; markh < 12; markh++)
  {
    r = markh * CTX_PI * 2 / 12.0 - CTX_PI / 2;

    ctx_move_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.8f);
    ctx_stroke (ctx);
  }
  ctx_line_width (ctx, radius * 0.01f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

#if 0
  for (int markm = 0; markm < 60; markm++)
  {
    r = markm * CTX_PI * 2 / 60.0 - CTX_PI / 2;

    ctx_move_to (ctx, x + cosf(r) * radius * 0.75f, y + sinf (r) * radius * 0.75f);
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.8f);
    ctx_stroke (ctx);
  }
#endif


#endif

  ctx_line_width (ctx, radius * 0.075f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  r = m * CTX_PI * 2 / 60.0 - CTX_PI / 2;
  ;
#if 1
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);
  ctx_stroke (ctx);
#endif

  r = h * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.4f, y + sinf (r) * radius * 0.4f);
  ctx_stroke (ctx);


  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_NONE);

  if (smoothstep)
    r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;
  else
    r = (s ) * CTX_PI * 2 / 60 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.78f, y + sinf (r) * radius * 0.78f);
  ctx_stroke (ctx);

  ctx_restore (ctx);
}

static void __analog_clock (Ctx     *ctx,
		           uint32_t ms,
			   float    x,
			   float    y,
			   float    radius,
			   int smoothstep)
{
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;

  ms = ((uint32_t)(ms))%1000;
  s %= 60;
  m %= 60;
  h %= 12;

  float r;
  ctx_save (ctx);

  ctx_rgba8 (ctx, 127, 127, 127, 255);
//ctx_move_to (ctx, x, y);
  ctx_arc (ctx, x, y, radius * 0.9, 0.0, CTX_PI * 1.9, 0);
  ctx_line_width (ctx, radius * 0.2);
  ctx_line_cap (ctx, CTX_CAP_NONE);
  ctx_stroke (ctx);

  ctx_line_width (ctx, 7);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  ctx_rgba8 (ctx, 188,188,188,255);

  r = m * CTX_PI * 2/ 60.0 - CTX_PI/2;
	  ;
#if 1
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.7f, y + ctx_sinf (r) * radius * 0.61f);
  ctx_stroke (ctx);
#endif

  r = h * CTX_PI * 2/ 12.0 - CTX_PI/2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.4f, y + ctx_sinf (r) * radius * 0.4f);
  ctx_stroke (ctx);


  ctx_line_width (ctx, 2);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  ctx_rgba8 (ctx, 255,0,0,127);

  if (smoothstep)
    r = (s + ms/1000.0f) * CTX_PI * 2/ 60 - CTX_PI/2;
  else
    r = (s ) * CTX_PI * 2/ 60 - CTX_PI/2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.75f, y + ctx_sinf (r) * radius * 0.75f);
  ctx_stroke (ctx);

  ctx_restore (ctx);
}

static void analog_clock (Ctx *ctx, int frame_no)
{
  uint64_t ms64 = ctx_ticks() / 1000;
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 1);
}

static void analog_clock2 (Ctx *ctx, int frame_no)
{
  uint64_t ms64 = ctx_ticks()/ 1000;
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 0);
}

static void gradient_text (Ctx *ctx, int frame_no)
{
  frame_no = frame_no % 400;
  float r = frame_no * 0.01;
  ctx_save (ctx);
  ctx_linear_gradient (ctx, 12, 12, 200, 200);
  ctx_translate (ctx, width/2, height/2);
  ctx_rotate (ctx, r * 1.66);
  ctx_translate (ctx, -width/2, -height/2);
  ctx_move_to (ctx, 0, 40);
  ctx_font_size (ctx, 24);
  ctx_text (ctx, "card10+ctx");
  ctx_restore (ctx);
}

uint32_t ctx_glyph_no (Ctx *ctx, int no);

static int glyph_no = 0;
static int unichar = 0;
static void font_unicode (Ctx *ctx, int frame_no)
{
  char buf[64];
  frame_no = frame_no % 2000;
  ctx_rgba8 (ctx, 0,0,0,255);

  int i = 0;
#define SYMBOL_WIDTH 28
#define SYMBOL_SIZE  28

  ctx_font (ctx, "symbol");
  ctx_font_size (ctx, SYMBOL_SIZE);
  ctx_rgba8 (ctx, 255,0,0,255);

  i = 0;
  for (int j = 0; j < 10 && SYMBOL_WIDTH * j < width; j ++)
  {
    int unichar = ctx_glyph_no (ctx, glyph_no + j);
    ctx_move_to (ctx, j * SYMBOL_WIDTH - (frame_no-1)%SYMBOL_WIDTH, 70);
    ctx_glyph (ctx, unichar, 0);
  }

  if (frame_no % SYMBOL_WIDTH == 0)
    glyph_no++;
  if (ctx_glyph_no (ctx, glyph_no) == 0) glyph_no = 0;
}


static void font_scaling (Ctx *ctx, int frame_no)
{
  char buf[64]="  px 0123456789\n";
  frame_no = frame_no % 7000;
  int font_size = height * 0.02 + frame_no * height * 0.01;
  ctx_gray (ctx, 1.0);

  buf[0] = '0' + (font_size/10);
  buf[1] = '0' + (font_size%10);

  ctx_font (ctx, "regular");
  ctx_font_size (ctx, font_size);
  ctx_move_to (ctx, height*0.02, font_size);
  ctx_text (ctx, buf);
  ctx_text (ctx, "text can be tricky\nbut we do seem\nto be fast now!");

  //ctx_font (ctx, "regular");
  //ctx_font_size (ctx, 18);
  //ctx_move_to (ctx, 5, 15);
  //sprintf (buf, "%i", 10 + frame_no/40);
  //ctx_text (ctx, buf);
}

void dot (Ctx *ctx, float x, float y, float radius)
{
  ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
  ctx_fill (ctx);
}

void dots_1000(Ctx *ctx, int frame_no)
{
    ctx_save(ctx);

    ctx_rgba(ctx, 1, 1, 1, 0.5);
    for (int i = 0; i < 1000; i ++)
    {
      float x = ctx_width (ctx)/ 2;
      float y = ctx_height (ctx) / 2;
      float siz = ctx_height (ctx) * 0.0125;

      float dist = i * (ctx_height (ctx)/ 2) / 1000.0;

      float twist = i * (12000 + frame_no) * 0.000033;
      x += cos (twist) * dist;
      y += sin (twist) * dist;

      dot (ctx, x, y, siz);
      // dot (ctx, ((int)(x*8))/8.0, ((int)(y*8))/8.0, siz);
        dot (ctx, ((int)(x*4))/4.0, ((int)(y*4))/4.0, siz);

    }

    ctx_restore (ctx);
}


void dots_100 (Ctx *ctx, int frame_no)
{
  {
    ctx_save(ctx);

    ctx_rgba(ctx, 1, 1, 1, 1);
    for (int i = 0; i < 100; i ++)
    {
      float x = ctx_width (ctx) / 2;
      float y = ctx_height (ctx) / 2;
//      float siz = ctx_height (ctx) * (0.005 + 0.02 *((frame_no-500)/400.0));

      float siz = 0.5 + i/100.0 * 6.0;
      float dist = i * (ctx_height (ctx) / 2) / 100;
      float twist = i * frame_no * 0.00031415*2;
      x += cos (twist) * dist;
      y += sin (twist) * dist;

      //dot (ctx, x, y, siz);
//      dot (ctx, ((int)(x*8))/8.0, ((int)(y*8))/8.0, siz);
//      dot (ctx, ((int)(x*6))/6.0, ((int)(y*6))/6.0, siz);

        dot (ctx, ((int)(x*4))/4.0, ((int)(y*4))/4.0, siz);
    }

    ctx_restore (ctx);
  }
}

static Test tests[]={
  analog_clock,
  dots_100,
  dots_1000,
  analog_clock2,
  gradient_text,
  snippet_gradient,
  font_unicode,
  font_scaling,
  snippet_text,
  snippet_card10,
  snippet_arc,
  snippet_arc_negative,
  snippet_curve_to,
  snippet_rounded_rectangle,
  //snippet_dash,
  snippet_fill_rule,
//snippet_image,
//snippet_image_pattern,
  snippet_multi_segment_caps,
  scope2,
  scope_aa,
  scope2_aa,
  scope,
};

static int test_no;
static long start;
static int quit = 0;

static void do_quit (CtxEvent *event, void *data1, void *data2)
{
        quit = 1;
}

static void prev_test (CtxEvent *event, void *data1, void *data2)
{
      test_no --;
      if (test_no < 0)
        test_no = sizeof (tests) / sizeof (tests[0]) - 1;
      frame_no = 0;
      start = ctx_ticks ();
}

static void next_test (CtxEvent *event, void *data1, void *data2)
{
      test_no ++;
      if (test_no >= sizeof (tests) / sizeof (tests[0]))
        test_no = 0;
      frame_no = 0;
      start = ctx_ticks ();
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);
  //Ctx *ctx = ctx_new_ui (1200, 1200);

  int frame_no = 0;

  //ctx_set_antialias (ctx, CTX_ANTIALIAS_FAST);
  start = ctx_ticks ();

  long fps_sec = start / (1000 * 1000 );
  int fps_frames = 0;

  char fpsbuf[16]="";

  while (!quit)
  {
    width = ctx_width (ctx);
    height = ctx_height (ctx);
    //epicfb_start_frame ();

    if (ctx_ticks () / (1000*1000) != fps_sec)
    {
      fps_sec = ctx_ticks () / (1000*1000);
      sprintf (fpsbuf, "\r%ifps  ", fps_frames);
      fps_frames=0;
    }
    fps_frames++;

    {
      ctx_reset (ctx);

#if 1
      /* clear */
      ctx_rectangle (ctx, 0, 0, width, height);
      ctx_rgba8 (ctx, 0,0,0,255);
      ctx_fill (ctx);
#else
      for (int i = 0; i < width * height; i++) pixels[i] = 0xffff;
#endif

      ctx_begin_path (ctx);
      ctx_save (ctx);
      tests[test_no](ctx, frame_no);
      ctx_restore (ctx);

      ctx_font_size (ctx, ctx_height(ctx)*0.1);
      ctx_move_to (ctx, 0.0, ctx_height(ctx)*0.1);
      ctx_rgba8 (ctx,255,0,0,255);
      ctx_text (ctx, fpsbuf);

      ctx_flush (ctx);
      ctx_add_key_binding (ctx, "left", NULL, "prev",  prev_test, NULL);
      ctx_add_key_binding (ctx, "right", NULL, "next",  next_test, NULL);
      ctx_add_key_binding (ctx, "q", NULL, "next",  do_quit, NULL);
      ctx_add_key_binding (ctx, "control-q", NULL, "next",  do_quit, NULL);
    }

    frame_no ++;

    while(ctx_get_event (ctx))
    {
    }
  }
  ctx_free (ctx);
  return 0;
}
