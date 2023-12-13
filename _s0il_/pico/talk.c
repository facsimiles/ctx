#include <stdint.h>

#include <stdio.h>
#include <math.h>

#include "ctx.h"

float render_time = 0.0;
float render_fps  = 0.0;

float      scroll       = 0.0f;
int        auto_advance = 1;
int        scene_frames = 0;
float      scene_elapsed_time = 0.0f;
static uint64_t prev_ticks = 0;
  static int   dot_count = 200;
  static float twist = 2.9645;
  static float dot_scale = 42.0;
  static int shape = 0;
static int scene_no = 0;
static int bouncy = 0;

typedef void (*SceneFun)(Ctx *ctx, const char *title, int frame_no, float time_delta);
typedef struct Scene{
  const char *title;
  SceneFun     fun;
  int  duration; // in frames
}Scene;

static void clear (Ctx *ctx)
{
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
  ctx_rgba8 (ctx, 0,0,0,255);
  ctx_fill (ctx);
}

static void scene_logo (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  static int dir = 1;
  static float dim = 0.0;

  clear (ctx);

  if (dir > 0)
  {
     dim += 10;
     if (dim > ctx_width (ctx) * 0.5) dir *= -1;
  }
  else
  {
     dim -= 10;
     if (dim < ctx_width (ctx) * 0.1) dir *= -1;
  }
  

  ctx_logo (ctx, ctx_width(ctx)/2, ctx_height(ctx)*0.4, 
            ctx_height (ctx) * 0.6 +
            ctx_height (ctx) * 0.3 * sin (frame_no/10.0f)
                  );
}

static void scene_clock (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  uint32_t ms = ctx_ticks ()/1000;
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  float radius = ctx_width(ctx)/2;
  int smoothstep = 1;
  clear(ctx);
  float x = ctx_width (ctx)/ 2;
  float y = ctx_height (ctx)/ 2;
  if (radius > ctx_height (ctx)/2) radius = ctx_height (ctx)/2;

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;
  
  float r; 
  ctx_save (ctx);
  
  ctx_rgba_stroke (ctx, 1,1,1,0.8);
  ctx_rgba (ctx, 1,1,1,0.8);

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

static void scene_curve_to (Ctx *ctx, const char *title, int frame_no, float time_delta)
{ 
  ctx_save (ctx);
  frame_no %= 400;
  clear (ctx);
  ctx_rgb (ctx, 1,1,1);
  float scale_factor = ctx_height(ctx)/240.0;
  ctx_translate (ctx, (ctx_width(ctx)-ctx_height(ctx))/2, 0);
  ctx_scale (ctx, scale_factor, scale_factor);
  float x=25.6,  y=128.0;
  float x1=102.4 + ((frame_no % 60) - 52), y1=230.4,
        x2=153.6, y2=25.6 + (frame_no % 60),
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
  ctx_restore (ctx); 
} 
  

typedef struct DragObject
{
  float x;
  float y; 
  float width;
  float height;
  float red;
  float green;
  float blue;
  float alpha;
} DragObject;


static void object_drag (CtxEvent *event, void *data1, void *data2)
{
  DragObject *obj = data1;
  obj->x += event->delta_x;
  obj->y += event->delta_y;
  event->stop_propagate=1;
  ctx_queue_draw (event->ctx);
}


static void scene_drag (Ctx *ctx, const char *title, int frame_no, float time_delta)
{ 
  static DragObject objects[8];
  static int first_run = 1;
  if (first_run)
  for (int i = 0; i <  8; i++)
  {
    objects[i].x = (i+1) * 8;
    objects[i].y = (i+1) * 8;
    objects[i].width = 20;
    objects[i].height = 20;
    objects[i].red = 0.5;
    objects[i].red = 0.1;
    objects[i].red = 0.9;
    objects[i].alpha = 0.6;
    first_run = 0;
  }
  
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_gray (ctx, 0);
  ctx_fill (ctx);

  ctx_rotate (ctx, 0.2 + frame_no * 0.001);
  ctx_scale (ctx, width/100, height/100);
  ctx_scale (ctx, 0.9, 0.9);

  for (int i = 0; i <  8; i++)
  {
    switch (i)
    {
      case 0:
      case 1:
      case 2:
      case 3:
         ctx_rectangle (ctx, objects[i].x, objects[i].y, objects[i].width, objects[i].height);
         break;
      case 4:
        ctx_move_to (ctx, objects[i].x, objects[i].y);
        ctx_rel_line_to (ctx, 0, objects[i].height);
        ctx_rel_line_to (ctx, objects[i].width, 0);
        break;
      case 5:
        ctx_move_to (ctx, objects[i].x, objects[i].y);
        ctx_line_to (ctx, objects[i].x +  objects[i].width, objects[i].y);
        ctx_line_to (ctx, objects[i].x +  objects[i].width, objects[i].y+objects[i].height);
        break;
      case 6:
      case 7:
        ctx_arc (ctx, objects[i].x+objects[i].width/2, objects[i].y+objects[i].width/2,
                      objects[i].width/2,
                      0.0, CTX_PI *  2.0, 0);
        break;
    }
    ctx_rgba (ctx, objects[i].red, objects[i].green, objects[i].blue, objects[i].alpha);
    ctx_listen (ctx, CTX_DRAG, object_drag, &objects[i], NULL);
    ctx_fill (ctx);
  }
#if 0
  ctx_rectangle (ctx, rect1_x, rect1_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,0,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect1_x, &rect1_y);
  ctx_fill (ctx);

  ctx_rectangle (ctx, rect2_x, rect2_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,1,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect2_x, &rect2_y);
  ctx_fill (ctx);
#endif
  ctx_restore (ctx);
}



static void scene_spirals (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  dot_count = frame_no + 16;
  shape = 0;
  clear (ctx);
  float alpha = 0.0;

  ctx_rgba (ctx, 1, 1, 1, 0.5 + alpha/2);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = (i) * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cos (twisted) * dist;
    y += sin (twisted) * dist;
    if (shape == 0)
    {
      ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
    }
    else
    {
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      ctx_rotate (ctx, twisted);
      ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
      ctx_restore (ctx);
    }
    ctx_fill (ctx);
  }
    twist += 0.00001;

  twist += time_delta * 0.001;
}


static void scene_spirals2 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  dot_count = frame_no + 16;

  clear (ctx);
  float alpha = 0.0;
  shape = 1;
  ctx_rgba (ctx, 1, 1, 1, 0.5 + alpha/2);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = (i) * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cos (twisted) * dist;
    y += sin (twisted) * dist;
    if (shape == 0)
    {
      ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
    }
    else
    {
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      ctx_rotate (ctx, twisted);
      ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
      ctx_restore (ctx);
    }
    ctx_fill (ctx);
  }

  twist += time_delta * 0.001;
}

static void scene_spirals3 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  dot_count = frame_no + 16;

  clear (ctx);
  shape = 1;
  ctx_rgb (ctx, 1, 1, 1);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = (i) * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cos (twisted) * dist;
    y += sin (twisted) * dist;
    if (shape == 0)
    {
      ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
    }
    else
    {
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      ctx_rotate (ctx, twisted);
      ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
      ctx_restore (ctx);
    }
    ctx_fill (ctx);
  }

  twist += time_delta * 0.001;
}



static void scene_circles (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  float alpha = 0.0;
  float beta = 0.0;
  if (scene_elapsed_time > 2.0)
  {
     alpha = (scene_elapsed_time-2.0)/2.0;
     alpha = ctx_minf (alpha, 1.0);
     alpha = ctx_maxf (alpha, 0.0);
  }
  beta = scene_elapsed_time/6.0;
  beta = ctx_minf (beta, 1.0);
  int count = 8;//beta * 8 + 2;

  clear (ctx);

  ctx_rgb (ctx, 1,1,1);

  alpha = alpha * 2;
  if (alpha > 1.0) alpha = (2 - alpha);
  else alpha = alpha / 2;

  for (int i = 0; i < count; i ++)
  {
    ctx_translate (ctx, sin (scene_elapsed_time) * height * 0.4, 0);
    ctx_arc (ctx, width * 0.5, height * 0.5, height / 20 * i,  0.0, M_PI*2, 0);
    ctx_translate (ctx, -2*sin (scene_elapsed_time) * height * 0.4, 0);
    ctx_arc (ctx, width * 0.5, height * 0.5, height / 20 * i,  0.0, M_PI*2, 0);
    ctx_translate (ctx, sin (scene_elapsed_time) * height * 0.4, 0);
  }
  ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
  ctx_fill (ctx);
}

char *slide_default[]=
{
  "XXXctx\n","compact","<100kb possible\n",
  "vectors","on","the","wire\n",
  "SVG","path","data","on","steroids\n",
  "high def graphics\n",
  "from microcontrollers to terminals 4k\n",
  NULL
};

#if 1
char *slide_intro[]=
{
  "UNUSEDI wanted good vectors in the terminal\n",
  "I needed vectors working in float and in CMYK\n",
  NULL
};
#endif

char *slide_intro0[]=
{
  "https://ctx.graphics/\n",
  "native","vectors","from","microcontrollers","to","4k displays\n",
  "used","by:", "card10", "ctx terminal,","GEGL","&","(GIMP)\n",
  "float", "1/2/4bit", "grayscale","RGB565","RGB","CMYK","PDF\n",
  "single header", "70-300kb","compiled\n",
  "change","detection","+","sub-region-render\n",
  "event","handling\n",
  "multi","threading\n",
  "LGPLv3+\n",
  NULL,
};

char *slide_terminal[]=
{
  "ctx","terminal\n",
  "DEC term/","xterm","workalike\n",
  "  tabs/MDI\n",
  "  mouse","keyboard\n",
  "  sixels", "kitty","iterm","rastergraphics\n",
  "  atty","PCM\n",
  "  ctx","graphics\n",
  "  GPLv3+,\n",
  "needs","refuzzing\n",
  NULL
};

char *slide_intro1[]=
{
  "native", "vectors", "are","rasterized","as","late","as","possible",
  "ideally","on","the","GPU",
  NULL
};

char *slide_intro2[]= 
{
  "aUNUSED\n",
  NULL
};
char *slide_intro3[]=
{
  "bUNUSED\n",
  NULL
};
char *slide_intro4[]=
{
  "to cache or not to cache",
  NULL
};


char *slide_protocol[]=
{
  "protocol/API\n",
  "  L 10 10\n",
  "  lineTo 10 10\n",
  "  ctx_line_to (ctx, 10, 10);\n",
  "\n",
  "  'L', {10.0f, 10.0f}\n",
  NULL};

char *slide_protocol1[]=
{
  "protocol/API\n",
  "  C 10 10 30 30 40 40\n",
  "  curveTo 10 10 30 30 40 40\n",
  "  ctx_curve_to (ctx, 10, 10, 30, 30, 40, 40);\n",
  "\n",
  "  'L', {10.0f, 10.0f}\n",
  "  0,   {30.0f, 30.0f}\n",
  "  0,   {40.0f, 40.0f}\n",
  NULL};

char *slide_protocol2[]={
  "  polylines, curves\n",
  "  fill","and","stroke\n",
  "  transforms\n",
  "    scale,translate,rotate,perspective\n",
  "  clipping\n",
  "  fonts/text\n",
  "  transparency\n",
  "  gradients\n",
  "  textures\n",
  NULL};


char *slide_backends[]=
{
  "backends\n",
  "for portable interactive use, use\n",
  "Ctx *ctx = ctx_new(-1,-1, NULL);\n",
  "\n",
  "NULL","here","acts","as","an","autoselect","over:\n",
  "  displaylist,","tiled,","SDL,","cb,","KMS,","fbdev,","hashcache,","rasterizer\n",
  NULL
};

char *slide_backend_internals[]=
{

  "struct _CtxBackend\n",
  "{\n",
  "  Ctx                      *ctx;\n",
  "  void  (*process)         (Ctx *ctx, CtxCommand *entry);\n",
  "\n",
  "  /* the following are only for event handling backends, and can be NULL*/ \n",
  "  void  (*start_frame)     (Ctx *ctx);\n",
  "  void  (*end_frame)       (Ctx *ctx);\n",
  "  void  (*set_windowtitle) (Ctx *ctx, const char *text);\n",
  "  char *(*get_event)       (Ctx *ctx, int timout_ms);\n",
  "  void  (*consume_events)  (Ctx *ctx);\n",
  "  void  (*get_event_fds)   (Ctx *ctx, int *fd, int *count);\n",
  "  char *(*get_clipboard)   (Ctx *ctx);\n",
  "  void  (*set_clipboard)   (Ctx *ctx, const char *text);\n",
  "  void  (*destroy)         (void *backend); /* the destroy pointers are abused as the differentiatior\n",
  "                                               between different backends,   */\n",
  "  CtxFlags                  flags;\n",
  "  CtxBackendType            type;\n",
  "  void                     *user_data; // not used by ctx core\n",
  "};\n",
  NULL
};

char *slide_backend_internals2[]={
  "struct\n",
  "  _CtxEntry\n",
  "{\n",
  "  uint8_t code;\n",
  "  union\n",
  "  {\n",
  "    float    f[2];\n",
  "    uint8_t  u8[8];\n",
  "    int8_t   s8[8];\n",
  "    uint16_t u16[4];\n",
  "    int16_t  s16[4];\n",
  "    uint32_t u32[2];\n",
  "    int32_t  s32[2];\n",
  "    uint64_t u64[1]; // unused\n",
  "  } data; // 9bytes long, we're favoring compactness and correctness\n",
  "}\n",
  "\n",
  "struct\n",
  "  _CtxCommand\n",
  "{\n",
  "  union\n",
  "  {\n",
  "    uint8_t  code;\n",
  "    CtxEntry entry;\n",
  "    struct\n",
  "    {\n",
  "      uint8_t code;\n",
  "      float x;\n",
  "      float y;\n",
  "    } line_to;\n",
  "    struct\n",
  "    {\n",
  "      uint8_t code;\n",
  "      float model;\n",
  "      float r;\n",
  "      uint8_t pad1;\n",
  "      float g;\n",
  "      float b;\n",
  "      uint8_t pad2;\n",
  "      float a;\n",
  "    } rgba;\n",
  "    struct\n",
  "    {\n",
  "      uint8_t code;\n",
  "      float model;\n",
  "      float c;\n",
  "      uint8_t pad1;\n",
  "      float m;\n",
  "      float y;\n",
  "      uint8_t pad2;\n",
  "      float k;\n",
  "      float a;\n",
  "    } cmyka;\n",
  "    ..\n",
  "  };\n",
  "  CtxEntry next_entry; // also pads size of CtxCommand slightly.\n",
  "};\n",
  NULL
};

char *slide_backend_formatter[]=
{
  "backend:formatter\n",
  "the reverse of the parser\n",
  "maybe rename to serialize\n",
  NULL
};

char *slide_backend_ctx[]=
{
  "backend:ctx\n",
  "interactive backend\n",
  "ctx in terminal\n",
  "ctxevents for terminal\n",
  NULL
};




char *slide_parser[]=
{
  "ctx_parse(Ctx *ctx, const char *string);\n"
  "\n",
  "  parses","the","text","version","of","the","protocol\n",
  "  interprets","some","unit","suffixes\n",
  "  and","calls","another","backend","for","each","command\n",
  NULL
};

char *slide_rasterizer[]=
{
  "rasterizer\n",
  "active","edge","table","rasterizer\n",
  "heuristic","per","scanline","choosing","one","of\n",
  "  fast","accurate","horizontal\n",
  "  accurate","for","gradient","spans\n",
  "  vertical","supersampling\n",
  NULL
};

char *slide_encodings[]=
{
  "compositing","code","target","pixel","encodings\n",
  "\n",
  "grayscale:",
  "1bit/2bit/4bit/8bit/float\n",
  "RGB",
  "8bit","sRGB,","float,","RGBA,","BGR565","RGB332\n",
  "CMYK",
  "8bit, float",
  NULL
};
char *slide_backend_displaylist[]=
{
  "displaylist\n",
  "stores 9byte chunks of commands in a (resizable) array",
  NULL
};

char *slide_backend_hashcache[]=
{
  "backend:hash cache\n",
  "subdivides screen in 6x5 grid\n",
  "computes/maintains hash for each cell\n",
  NULL
};


char *slide_backend_tiled[]=
{
  "backend:tiled\n",
  "subdivides screen in 6x5 grid\n",
  "updates\n",
  "\n",
  NULL
};

char *slide_backend_headless[]=
{
  "backend:headless\n",
  "for now dev/testing-only\n",
  "callbacks","and","be","used","by","\"external\"","SDL/fb/wayland","drivers\n",
  NULL
};


char *slide_license[]=
{
  //"with","funding","equal","took","100kEUR/year","for","the","time","I","have","invested","in","ctx","the","license","can","be","changed","from","LGPL","to","BSD,",
  "lgplv3+",
  NULL
};



char *slide_backend_cb[]=
{
  "backend:cb\n",
  "multi strategy backend\n",
  "can use hash cache\n",
  "can use lowfi res/bitdepth\n",
  "userfun for subregion update\n",
  NULL
};

char *slide_backend_fb[]=
{
  "backend:kms","backend:fb\n",
  "\n",
  "even","pid=1","is","possible!",
  NULL
};


char *slide_backend_sdl[]=
{
  "backend:SDL2\n",
  "Interactive reference\n",
  "Portable,","alas OpenGL\n",
  "  todo:","wayland\n",
  NULL
};

char *slide_backend_pdf[]=
{
  "backend:pdf\n",
  NULL
};
char *slide_backend_term[]=
{
  "backend:term\n",
  NULL
};

char *slide_fonts[]=
{
  "ctx has two fonts backends\n",
  "  stb_ttf\n",
  "  ctx,","which","is","binary","ctx","protocol\n",
  "  packed","commands\n",
  "  built-in","basic","lating kerning/ligatures","complex","shaping","on","top\n",
  NULL
};


char *slide_mcu[]=
{
  "card10\n",
  "  ARM Cortex-M4f","96mhz,","512kb","RAM\n",
  "esp32\n",
  "  lx7 240mhz","320kb","RAM\n",
  "rp2040\n",
  "  ARM Cortex-M0","133mhz (OC 270mhz)\n",
  NULL
};

char *slide_events2[]=
{
  " // make path .. ctx_move_to,","_line_to,","_close_path ();\n",
  "ctx_listen","(ctx,","CTX_DRAG,","object_drag,","&objects[i],","NULL);",
  "ctx_fill","(ctx);",
  NULL
};

char *slide_events3[]=
{
"static void action_fullscreen (CtxEvent *event, void *data1, void *data2)\n",
"{\n",
" Ctx *ctx = event->ctx;\n",
" if (ctx_get_fullscreen (ctx))\n",
"    ctx_set_fullscreen (ctx, 0);\n",
"  else\n",
"    ctx_set_fullscreen (ctx, 1);\n",
"}\n",
"\n",
"ctx_add_key_binding","(ctx, \"up\", NULL, \"foo\",","action_scroll_up, NULL);\n",
"ctx_add_key_binding","(ctx, \"down\", NULL, \"foo\",","action_scroll_down, NULL);\n",
"ctx_add_key_binding","(ctx, \"home\", NULL, \"foo\",","action_first, NULL);\n",
"ctx_add_key_binding","(ctx, \"left\", NULL, \"foo\",","action_prev, NULL);\n",
"ctx_add_key_binding","(ctx, \"right\", NULL, \"foo\",","action_next, NULL);\n",
"ctx_add_key_binding","(ctx, \"q\", NULL, \"foo\",","action_quit, NULL);\n",
"ctx_add_key_binding","(ctx, \"F11\", NULL, \"foo\",","action_fullscreen, NULL);\n",
"ctx_add_key_binding","(ctx, \"f\", NULL, \"foo\",","action_fullscreen, NULL);\n",
  NULL
};

char *slide_simd[]=
{
  "auto-vectorize!\n",
};


char *slide_pippin[]=
{
  "Øyvind Kolås\n",
  "https://pippin.gimp.org/\n",
  "@pippin@mastodon.xyz\n",
  "\n",
  "GIMP","GEGL","babl","cairo","hnb","hal91","rcairo","Clutter",
  NULL
};

static void scene_text (Ctx *ctx, const char *title, int frame_no, float time_delta, float font_scale)
{
  char **words = slide_default;
  if (!strcmp (title, "rasterizer"))   words = slide_rasterizer;
  else if (!strcmp (title, "parser"))  words = slide_parser;
  else if (!strcmp (title, "encodings"))   words = slide_encodings;
  else if (!strcmp (title, "simd"))               words = slide_simd;


  else if (!strcmp (title, "events2"))    words = slide_events2;
  else if (!strcmp (title, "events3"))    words = slide_events3;
  else if (!strcmp (title, "protocol"))    words = slide_protocol;
  else if (!strcmp (title, "backends"))    words = slide_backends;
  else if (!strcmp (title, "backend-cb"))  words = slide_backend_cb;
  else if (!strcmp (title, "backend-pdf")) words = slide_backend_pdf;
  else if (!strcmp (title, "backend-internals"))  words = slide_backend_internals;
  else if (!strcmp (title, "backend-internals2")) words = slide_backend_internals2;
  else if (!strcmp (title, "backend-formatter"))  words = slide_backend_formatter;
  else if (!strcmp (title, "backend-ctx"))        words = slide_backend_ctx;
  else if (!strcmp (title, "backend-term"))       words = slide_backend_term;
  else if (!strcmp (title, "backend-hashcache"))  words = slide_backend_hashcache;
  else if (!strcmp (title, "backend-displaylist")) words = slide_backend_displaylist;
  else if (!strcmp (title, "backend-tiled"))       words = slide_backend_tiled;
  else if (!strcmp (title, "backend-headless"))  words = slide_backend_headless;
  else if (!strcmp (title, "backend-fb"))        words = slide_backend_fb;
  else if (!strcmp (title, "backend-sdl"))       words = slide_backend_sdl;
  else if (!strcmp (title, "terminal"))          words = slide_terminal;
  else if (!strcmp (title, "mcu"))               words = slide_mcu;
  else if (!strcmp (title, "fonts"))             words = slide_fonts;
  else if (!strcmp (title, "intro"))     words = slide_intro;
  else if (!strcmp (title, "intro0"))    words = slide_intro0;
  else if (!strcmp (title, "intro1"))    words = slide_intro1;
  else if (!strcmp (title, "intro2"))    words = slide_intro2;
  else if (!strcmp (title, "intro3"))    words = slide_intro3;
  else if (!strcmp (title, "intro4"))    words = slide_intro4;
  else if (!strcmp (title, "license"))    words = slide_license;
  else if (!strcmp (title, "protocol1"))    words = slide_protocol1;
  else if (!strcmp (title, "protocol2"))    words = slide_protocol2;
  else if (!strcmp (title, "pippin"))    words = slide_pippin;

  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  float alpha = 0.0;
  float beta = 0.0;
  static int last_lines = 0;

  beta = scene_elapsed_time/6.0;
  beta = ctx_minf (beta, 1.0);

  clear (ctx);

  ctx_rgb (ctx, 1,1,1);

  alpha = alpha * 2;
  if (alpha > 1.0) alpha = (2 - alpha);
  else alpha = alpha / 2;

  float font_size = height * font_scale;

  if (font_size <= 4.0)
          font_size = 3.0;


  float line_height = 1.2;

  if (font_scale <  0.03)
          frame_no += 300;

  int count = frame_no;
  ctx_font_size (ctx, font_size);

  float space_width = ctx_text_width (ctx, " ");

  if (font_size <= 3){
          font_size = 3.0;
          line_height = 1.0;
          space_width = 2;
  }

  float y0 = font_size * line_height;
  float x0 = space_width;
  float y = y0;
  float x = x0;
  y = (int) y;

  ctx_save (ctx);

  static float scrolled = 0.0;
  scrolled = scrolled * 0.8 + scroll * 0.2;

  ctx_translate (ctx, 0.0, scrolled * ctx_height (ctx));

  srandom (0);

  ctx_move_to (ctx, 0, (int)(font_size) * 0.7);


  for (int i = 0; i < count && words[i]; i++)
  {
    const char *word = words[i];

    if (!strcmp(word, "\n"))
    {
       x = x0; 
       y += font_size * line_height;
    }
    else
    {

    if (x != x0)
      x += space_width;

    float word_width = ctx_text_width (ctx, word);
    if (x + word_width > width)
    {
      x = x0; 
      y += font_size * line_height;
    }

    ctx_move_to (ctx, x, y);
    if (!strcmp (word, "RGB"))
    {
      ctx_save (ctx);
      ctx_rgb (ctx, 1.0f, 0.3f, 0.3f);
      ctx_text (ctx, "R");
      ctx_rgb (ctx, 0.2f, 1.0f, 0.2f);
      ctx_text (ctx, "G");
      ctx_rgb (ctx, 0.6f, 0.6f, 1.0f);
      ctx_text (ctx, "B");
      ctx_restore (ctx);
    }
    else if (!strcmp (word, "RGBA"))
    {
      ctx_save (ctx);
      ctx_rgb (ctx, 1.0f, 0.0f, 0.0f);
      ctx_text (ctx, "R");
      ctx_rgb (ctx, 0.0f, 1.0f, 0.0f);
      ctx_text (ctx, "G");
      ctx_rgb (ctx, 0.0f, 0.0f, 1.0f);
      ctx_text (ctx, "B");
      ctx_rgb (ctx, 0.5f, 0.5f, 0.5f);
      ctx_text (ctx, "A");
      ctx_restore (ctx);
    }
    else if (!strcmp (word, "CMYK"))
    {
      ctx_save (ctx);
      ctx_rgb (ctx, 0.2f, 1.0f, 1.0f);
      ctx_text (ctx, "C");
      ctx_rgb (ctx, 1.0f, 0.1f, 1.0f);
      ctx_text (ctx, "M");
      ctx_rgb (ctx, 1.0f, 1.0f, 0.0f);
      ctx_text (ctx, "Y");
      ctx_rgb (ctx, 0.35f, 0.35f, 0.35f);
      ctx_text (ctx, "K");
      ctx_restore (ctx);
    }
    else
    {
      ctx_text (ctx, word);
    }
    x += word_width;


    if (word[strlen(word)-1]=='\n')
      {
        x = x0; 
        y += font_size * line_height;
      }
   }
  }
  ctx_restore (ctx);
  last_lines ++;

  //if (y > height- font_size) scroll = scroll * 0.95 + 0.05 * (y -height + font_size)  ; else scroll = 0;
}

static void scene_text_50 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.02);
}

static void scene_text_40 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.025);
}

static void scene_text_30 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.033);
}

static void scene_text_20 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.05);
}

static void scene_text_25 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/25.0f);
}
static void scene_text_15 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/15.0f);
}
static void scene_text_14 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/14.0f);
}
static void scene_text_13 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/13.0f);
}
static void scene_text_12 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/12.0f);
}

static void scene_text_10 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.1);
}

static void scene_text_7 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 1.0/7.0f);
}

static void scene_text_5 (Ctx *ctx, const char *title, int frame_no, float time_delta)
{
   scene_text (ctx, title, frame_no, time_delta, 0.2);
}


void reset_time(void)
{
  scene_no = 0;
  scene_frames = 0;
  scene_elapsed_time = 0;
}


void set_scene(int no)
{
  if (no>=0)scene_no = no;
}

float _ctx_pause = 0.0;

void ctx_parse2 (Ctx *ctx, const char *str, float *scene_time, int *scene_no);


static void reset_scene (Ctx *ctx)
{
  bouncy = 0;
  scene_frames = 0;
  scene_elapsed_time = 0.0f;
  auto_advance = 0;
  scroll = 0.0f;
  ctx_queue_draw (ctx);
}

static void action_scroll_up (CtxEvent *event, void *data1, void *data2)
{
  scroll += 0.333333;
  ctx_queue_draw (event->ctx);
}

static void action_scroll_down (CtxEvent *event, void *data1, void *data2)
{
  scroll -= 0.333333;
  ctx_queue_draw (event->ctx);
}

Scene scenes[]=
{
  {"logo",        scene_logo, 120},
  {"pippin",        scene_text_12, 120},
  {"spiral dots", scene_spirals, 120},
  {"spiral boxes", scene_spirals2, 120},
  {"spiral boxes", scene_spirals3, 120},
  {"circles",    scene_circles, 120},
  //{"curve_to",    scene_curve_to, 120},
  {"intro0",      scene_text_12, 120},
  {"terminal",    scene_text_13, 120},
  {"intro1",      scene_text_7, 120},
  //{"intro2",      scene_text_12, 120},
  //{"intro3",      scene_text_12, 120},
  {"protocol",    scene_text_12, 120},
  {"protocol1",    scene_text_12, 120},
  {"protocol2",    scene_text_12, 120},
  {"events2",        scene_text_15, 120},
  {"events",        scene_drag, 120},
  {"events3",        scene_text_20, 120},
  {"backends",    scene_text_10, 120},

  {"backend-internals",    scene_text_15, 120},
  {"backend-internals2",    scene_text_15, 120},
  //{"backend-formatter",    scene_text_10, 120},
  {"backend-ctx",          scene_text_10, 120},
  {"parser",  scene_text_10, 120},
  {"rasterizer",  scene_text_10, 120},
  {"encodings",   scene_text_10, 120},
  {"simd",    scene_text_10, 120},
  //{"backend-displaylist",  scene_text_10, 120},
  {"backend-hashcache",    scene_text_10, 120},
  {"fonts",  scene_text_10, 120},
  //{"backend-tiled",        scene_text_10, 120},
  //{"backend-headless",     scene_text_10, 120},
  {"backend-sdl",          scene_text_10, 120},
  {"backend-fb",           scene_text_10, 120},
  {"backend-cb",           scene_text_10, 120},
  {"mcu",    scene_text_10, 120},
  {"backend-term",         scene_text_10, 120},
  {"backend-pdf",          scene_text_10, 120},
  {"logo",        scene_clock, 120},
};
int n_scenes = sizeof (scenes)/sizeof(scenes[0]);
static void action_first (CtxEvent *event, void *data1, void *data2)
{
  scene_no = 0;
  reset_scene (event->ctx);
}

static void action_next (CtxEvent *event, void *data1, void *data2)
{
  scene_no++;
  if (scene_no >= n_scenes)
    scene_no = 0;
  reset_scene (event->ctx);
}

static void action_prev (CtxEvent *event, void *data1, void *data2)
{
  scene_no--;
  if (scene_no < 0)
    scene_no = n_scenes - 1;
  reset_scene (event->ctx);
}

static void action_fullscreen (CtxEvent *event, void *data1, void *data2)
{
 Ctx *ctx = event->ctx;
 if (ctx_get_fullscreen (ctx))
    ctx_set_fullscreen (ctx, 0);
  else
    ctx_set_fullscreen (ctx, 1);
}

static void action_bouncy (CtxEvent *event, void *data1, void *data2)
{
  bouncy = !bouncy;
}

static void action_quit (CtxEvent *event, void *data1, void *data2)
{
  ctx_exit (event->ctx);
  ctx_queue_draw (event->ctx);
}


static int ui_scenes (Ctx *ctx, void *data)
{
  uint64_t ticks = ctx_ticks ();
  static float bx = 11.0;
  static float by = 23.0;
  static float bvx = 4;
  static float bvy = 2.4;
 

  render_time = (ticks - prev_ticks) / 1000.0f/ 1000.0f / 10.0f;
  render_fps = 1.0 / render_time;
  prev_ticks = ticks;

  ctx_start_frame (ctx);

  scene_elapsed_time += render_time;
  scene_frames ++;
  {
    if ((unsigned)scene_no >= sizeof (scenes)/sizeof(scenes[0]))
      scene_no = 0;//sizeof (scenes)/sizeof(scenes[0])-1;

  ctx_save (ctx);
  scenes[scene_no].fun (ctx, scenes[scene_no].title, scene_frames, render_time);
  ctx_restore (ctx);
 
  if (bouncy)
  {
    ctx_logo (ctx, bx, by, ctx_height(ctx) * 0.1f);
    bx += bvx;
    by += bvy;
    if (bx < 0 || bx > ctx_width(ctx)) bvx *= -1;
    if (by < 0 || by > ctx_height(ctx)) bvy *= -1;
  }

  ctx_add_key_binding (ctx, "up", NULL, "foo",  action_scroll_up, NULL);
  ctx_add_key_binding (ctx, "down", NULL, "foo",  action_scroll_down, NULL);
  ctx_add_key_binding (ctx, "home", NULL, "foo",  action_first, NULL);
  ctx_add_key_binding (ctx, "left", NULL, "foo",  action_prev, NULL);
  ctx_add_key_binding (ctx, "right", NULL, "foo", action_next, NULL);
  ctx_add_key_binding (ctx, "q", NULL, "foo",     action_quit, NULL);
  ctx_add_key_binding (ctx, "F11", NULL, "foo",   action_fullscreen, NULL);
  ctx_add_key_binding (ctx, "f", NULL, "foo",   action_fullscreen, NULL);
  ctx_add_key_binding (ctx, "b", NULL, "foo",   action_bouncy, NULL);
  ctx_end_frame (ctx);

  if(auto_advance)
  if (scenes[scene_no].duration < scene_frames)
  {
    scene_no++;
    if (scene_no >= n_scenes)
      scene_no = 0;
    scene_frames = 0;
    scene_elapsed_time = 0.0f;
  }

  }

  return 1;
}

int main (int argc, char **argv)
{
  //set_sys_clock_khz(270000, true);
  Ctx *ctx = ctx_new (-1, -1, NULL);
  prev_ticks = ctx_ticks();

  ctx_get_event (ctx);
  while(!ctx_has_exited (ctx))
     ui_scenes (ctx, NULL);
 
  return 0;
}
