/* a C tinyvg parser with minimal RAM requirement and geometry culling
 * ability, (c) 2022 Øyvind Kolås, pippin@gimp.org
 */


#if CTX_TINYVG

#define CTX_TVG_STDIO     1

#if CTX_TVG_STDIO
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


typedef struct CtxTinyVGStyle
{
   uint8_t  type;
   uint32_t idx0;
   uint32_t idx1;
   float    x0;
   float    y0;
   float    x1;
   float    y1;
} CtxTinyVGStyle;


#define CTX_TVG_CACHE_SIZE  256

typedef struct CtxTinyVG {
   Ctx     *ctx;
   uint32_t length;
   uint32_t pos;

#if CTX_TVG_STDIO
   int      fd;
   uint32_t fd_pos;
#endif
   uint8_t  _cache[CTX_TVG_CACHE_SIZE];
   uint8_t  *cache;
   uint32_t cache_length;
   uint32_t cache_offset;

   int      pal_pos; // position of palette in TVG
   int      flags;
   uint32_t width;
   uint32_t height;
   uint8_t  scale;
   float    scale_factor;
   uint8_t  val_type:2;
   uint8_t  color_bytes:5;
   uint16_t color_count;
   uint8_t  error;
   uint8_t *pal;

   float    clipx0;
   float    clipy0;
   float    clipx1;
   float    clipy1;

   CtxTinyVGStyle fill;
   CtxTinyVGStyle stroke;
} CtxTinyVG;

#define CTX_TVG_MAGIC0    0x72
#define CTX_TVG_MAGIC1    0x56
#define CTX_TVG_VERSION      1

enum {
  CTX_TVG_VALTYPE_U16=0,
  CTX_TVG_VALTYPE_U8,
  CTX_TVG_VALTYPE_U32
};

enum {
  CTX_TVG_FLAT=0,
  CTX_TVG_LGRAD,
  CTX_TVG_RGRAD
};

enum {
  CTX_TVG_END_DRAWING=0,
  CTX_TVG_FILL_POLYGON,
  CTX_TVG_FILL_RECTANGLES,
  CTX_TVG_FILL_PATH,
  CTX_TVG_DRAW_LINES,
  CTX_TVG_DRAW_LINE_LOOP,
  CTX_TVG_DRAW_LINE_STRIP,
  CTX_TVG_DRAW_LINE_PATH,
  CTX_TVG_OUTLINE_FILL_POLYGON,
  CTX_TVG_OUTLINE_FILL_RECTANGLES,
  CTX_TVG_OUTLINE_FILL_PATH
};

enum {
  CTX_TVG_LINE_TO = 0,
  CTX_TVG_HOR_TO,
  CTX_TVG_VER_TO,
  CTX_TVG_CURVE_TO,
  CTX_TVG_ARC_CIRCLE,
  CTX_TVG_ARC_ELLIPSE,
  CTX_TVG_CLOSE_PATH,
  CTX_TVG_QUAD_TO
};

/* bounds protected accesors */
static inline void ctx_tvg_seek (CtxTinyVG *tvg, uint32_t pos)
{
  if (pos < tvg->length)
  {
    tvg->pos = pos;
  }
}

static void ctx_tvg_init_fd (CtxTinyVG *tvg, Ctx *ctx, int fd, int flags)
{
  memset (tvg, 0, sizeof (CtxTinyVG));
  tvg->fd = fd;
  tvg->ctx = ctx;
  tvg->length = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);
  tvg->cache_offset = 900000000;
  tvg->flags = flags;
  tvg->cache = &tvg->_cache[0];
  tvg->cache_length = CTX_TVG_CACHE_SIZE;
}

static void ctx_tvg_init_data (CtxTinyVG *tvg, Ctx *ctx, void *data, int len, int flags)
{
  memset (tvg, 0, sizeof (CtxTinyVG));
  tvg->ctx = ctx;
  tvg->cache = data;
  tvg->cache_length = len;
  tvg->length = len;
  tvg->cache_offset = 0;
  tvg->flags = flags;
}

#if CTX_TVG_STDIO
static inline int ctx_tvg_prime_cache (CtxTinyVG *tvg, uint32_t pos, int len)
{
  if (!tvg->fd)
  {
    if (pos + len < tvg->length)
      return 1;
    return 0;
  }
  if (tvg->cache_offset < pos && tvg->cache_offset + CTX_TVG_CACHE_SIZE - 1 > pos+len)
  {
    return 1;
  }
  tvg->cache_offset = pos;

  if (tvg->fd_pos != pos)
  {
    lseek (tvg->fd, pos, SEEK_SET);
    tvg->fd_pos = pos;
  }
  read (tvg->fd, tvg->cache, CTX_TVG_CACHE_SIZE);
  tvg->fd_pos += CTX_TVG_CACHE_SIZE;
  return 1;
}
#endif

static inline void ctx_tvg_memcpy (CtxTinyVG *tvg, void *dst, int pos, int len)
{
  if (ctx_tvg_prime_cache (tvg, pos, len))
    memcpy (dst, &tvg->cache[pos-tvg->cache_offset], len);
  else
    memset (dst, 0, len);
}

#define CTX_TVG_DEFINE_ACCESOR(nick, type) \
static inline type ctx_tvg_##nick (CtxTinyVG *tvg)\
{ type ret;\
  ctx_tvg_memcpy (tvg, &ret, tvg->pos, sizeof (type));\
  tvg->pos += sizeof(type);\
  return ret;\
}

CTX_TVG_DEFINE_ACCESOR(u8, uint8_t);
CTX_TVG_DEFINE_ACCESOR(u16, uint16_t);
CTX_TVG_DEFINE_ACCESOR(u32, uint32_t);
CTX_TVG_DEFINE_ACCESOR(float, float);

#undef CTX_TVG_DEFINE_ACCESSOR

static inline uint8_t ctx_tvg_u6_u2 (CtxTinyVG *tvg, uint8_t *u2_ret)
{
  uint8_t ret = ctx_tvg_u8 (tvg);
  *u2_ret = (ret >> 6) & 3;
  return (ret & 63);
}

static inline int32_t ctx_tvg_val (CtxTinyVG *tvg)
{
  switch (tvg->val_type)
  {
    case CTX_TVG_VALTYPE_U16: return ctx_tvg_u16(tvg);
    case CTX_TVG_VALTYPE_U8:  return ctx_tvg_u8(tvg);
    case CTX_TVG_VALTYPE_U32: return ctx_tvg_u32(tvg);
  }
  return 0;
}

static inline float ctx_tvg_unit (CtxTinyVG *tvg)
{
  uint32_t rv = ctx_tvg_val(tvg);
  return rv * tvg->scale_factor;
}

static inline uint32_t ctx_tvg_var (CtxTinyVG *tvg)
{
  uint32_t pos = 0, last, res = 0;
  do {
    last = ctx_tvg_u8(tvg);
    res += ((last & 0x7f) << (7*pos));
    pos++;
  } while (last & 0x80);
  return res;
}

#define CTX_TVG_MIN(a,b)  (((a)<(b))?(a):(b))
#define CTX_TVG_MAX(a,b)  (((a)>(b))?(a):(b))

static void
ctx_tvg_segment (CtxTinyVG *tvg, int n_commands)
{
  Ctx *ctx = tvg->ctx;

  if (tvg->flags & CTX_TVG_FLAG_BBOX_CHECK)
  {
      float minx = 1000000.0;
      float miny = 1000000.0;
      float maxx = -1000000.0;
      float maxy = -1000000.0;
      int start_pos = tvg->pos;
  
      float x = ctx_tvg_unit(tvg);
      float y = ctx_tvg_unit(tvg);
  
  #define ADD_COORD(x,y)\
    minx = CTX_TVG_MIN (minx, x);\
    miny = CTX_TVG_MIN (miny, y);\
    maxx = CTX_TVG_MAX (maxx, x);\
    maxy = CTX_TVG_MAX (maxy, y);\
  
    ADD_COORD(x,y);
  
    for (int i = 0; i < n_commands; i++)
    {
       int kind = ctx_tvg_u8(tvg);
       int has_line_width = (kind & ~0x7) !=0;
       kind = kind & 0x7;
  
       if (has_line_width)
       {
          float new_line_width = ctx_tvg_unit (tvg);
          //printf ("with new line width! %f\n", new_line_width);
          ctx_line_width (ctx, new_line_width);
       }
       switch (kind)
       {
         case CTX_TVG_LINE_TO:
           x = ctx_tvg_unit(tvg);
           y = ctx_tvg_unit(tvg);
           ADD_COORD(x,y);
           break;
         case CTX_TVG_HOR_TO:
           x = ctx_tvg_unit(tvg);
           ADD_COORD(x,y);
           break;
         case CTX_TVG_VER_TO:
           y = ctx_tvg_unit(tvg);
           ADD_COORD(x,y);
           break;
         case CTX_TVG_CURVE_TO:
           {
             float cx0 = ctx_tvg_unit(tvg);
             float cy0 = ctx_tvg_unit(tvg);
             float cx1 = ctx_tvg_unit(tvg);
             float cy1 = ctx_tvg_unit(tvg);
                   x   = ctx_tvg_unit(tvg);
                   y   = ctx_tvg_unit(tvg);
             if (cx0 + cy0 + cx1 + cy1){}
             ADD_COORD(x,y);
           }
           break;
         case CTX_TVG_ARC_CIRCLE:
           { // XXX NYI XXX
             uint8_t large_and_sweep = ctx_tvg_u8 (tvg);
             float   radius = ctx_tvg_unit (tvg);
                     x = ctx_tvg_unit (tvg);
                     y = ctx_tvg_unit (tvg);
             if (radius && large_and_sweep){};
             ADD_COORD(x,y);
           }
           break;
         case CTX_TVG_ARC_ELLIPSE:
           { // XXX NYI XXX
             uint8_t large_and_sweep = ctx_tvg_u8 (tvg);
             float rx       = ctx_tvg_unit (tvg);
             float ry       = ctx_tvg_unit (tvg);
             float rotation = ctx_tvg_unit (tvg);
                   x        = ctx_tvg_unit (tvg);
                   y        = ctx_tvg_unit (tvg);
             if (rotation && rx && ry && large_and_sweep) {};
             ADD_COORD(x,y);
           }
           break;
         case CTX_TVG_CLOSE_PATH:
  //       ctx_close_path (ctx);
           break;
         case CTX_TVG_QUAD_TO:
           {
             float cx0 = ctx_tvg_unit(tvg);
             float cy0 = ctx_tvg_unit(tvg);
             if (cx0 + cy0){}
             x = ctx_tvg_unit(tvg);
             y = ctx_tvg_unit(tvg);
             ADD_COORD(x,y);
           }
           break;
         default:
           tvg->error = 1;
       }
    }
  
      if (minx > tvg->clipx1) return;
      if (miny > tvg->clipy1) return;
      if (maxx < tvg->clipx0) return;
      if (maxy < tvg->clipy0) return;
  
      ctx_tvg_seek (tvg,start_pos);
  }

  {
  float x = ctx_tvg_unit(tvg);
  float y = ctx_tvg_unit(tvg);
  ctx_move_to (ctx, x, y);

  for (int i = 0; i < n_commands; i++)
  {
     int kind = ctx_tvg_u8(tvg);
     int has_line_width = (kind & ~0x7) !=0;
     kind = kind & 0x7;

     if (has_line_width)
     {
       float new_line_width = ctx_tvg_unit (tvg);
       //printf ("with new line width! %f\n", new_line_width);
       ctx_line_width (ctx, new_line_width);
     }
     switch (kind)
     {
       case CTX_TVG_LINE_TO:
         x = ctx_tvg_unit(tvg);
         y = ctx_tvg_unit(tvg);
         ctx_line_to (ctx, x, y);
         break;
       case CTX_TVG_HOR_TO:
         x = ctx_tvg_unit(tvg);
         ctx_line_to (ctx, x, y);
         break;
       case CTX_TVG_VER_TO:
         y = ctx_tvg_unit(tvg);
         ctx_line_to (ctx, x, y);
         break;
       case CTX_TVG_CURVE_TO:
         {
           float cx0 = ctx_tvg_unit(tvg);
           float cy0 = ctx_tvg_unit(tvg);
           float cx1 = ctx_tvg_unit(tvg);
           float cy1 = ctx_tvg_unit(tvg);
                 x   = ctx_tvg_unit(tvg);
                 y   = ctx_tvg_unit(tvg);
           ctx_curve_to (ctx, cx0, cy0, cx1, cy1, x, y);
         }
         break;
       case CTX_TVG_ARC_CIRCLE:
         { 
           uint8_t large_and_sweep = ctx_tvg_u8 (tvg);
           uint8_t large = ((large_and_sweep & 1) == 1);
           uint8_t sweep = ((large_and_sweep & 2) == 2);
           float   radius = ctx_tvg_unit (tvg);
                   x = ctx_tvg_unit (tvg);
                   y = ctx_tvg_unit (tvg);
           ctx_svg_arc_to (ctx, radius, radius, 0.0, large, sweep, x, y);
         }
         break;
       case CTX_TVG_ARC_ELLIPSE:
         {
           uint8_t large_and_sweep = ctx_tvg_u8 (tvg);
           uint8_t large = ((large_and_sweep & 1) == 1);
           uint8_t sweep = ((large_and_sweep & 2) == 2);
           float rx       = ctx_tvg_unit (tvg);
           float ry       = ctx_tvg_unit (tvg);
           float rotation = ctx_tvg_unit (tvg);
                 x        = ctx_tvg_unit (tvg);
                 y        = ctx_tvg_unit (tvg);
           ctx_svg_arc_to (ctx, rx, ry, rotation, large, sweep, x, y);
         }
         break;
       case CTX_TVG_CLOSE_PATH:
         ctx_close_path (ctx);
         break;
       case CTX_TVG_QUAD_TO:
         {
           float cx0 = ctx_tvg_unit(tvg);
           float cy0 = ctx_tvg_unit(tvg);
           x = ctx_tvg_unit(tvg);
           y = ctx_tvg_unit(tvg);
           ctx_quad_to (ctx, cx0, cy0, x, y);
         }
         break;
       default:
         tvg->error = 1;
     }
  }
  }
}

static void ctx_tvg_style (CtxTinyVG *tvg, int type,
                           CtxTinyVGStyle *style)
{
  style->type = type;
  switch (type)
  {
    case CTX_TVG_FLAT:
      style->idx0 = ctx_tvg_var(tvg);
      if (style->idx0 >= tvg->color_count) style->idx0 = 0;
      break;
    case CTX_TVG_LGRAD:
    case CTX_TVG_RGRAD:
      style->x0   = ctx_tvg_unit (tvg);
      style->y0   = ctx_tvg_unit (tvg);
      style->x1   = ctx_tvg_unit (tvg);
      style->y1   = ctx_tvg_unit (tvg);
      style->idx0 = ctx_tvg_var (tvg);
      style->idx1 = ctx_tvg_var (tvg);
      /*printf ("style:%f %f-%f %f   %i %i\n", 
      style->x0   ,
      style->y0   ,
      style->x1   ,
      style->y1   ,
      style->idx0 ,
      style->idx1 );
      */
      if (style->idx0 >= tvg->color_count) style->idx0 = 0;
      if (style->idx1 >= tvg->color_count) style->idx1 = 0;
      break;
  }
}

static inline void
ctx_tvg_get_color (CtxTinyVG *tvg, uint32_t idx,
                   float *red, float *green, float *blue, float *alpha)
{
#if CTX_TVG_STDIO
   if (tvg->fd && ((tvg->flags & CTX_TVG_FLAG_LOAD_PAL)==0))
   {
     int old_pos = tvg->pos;
     switch (tvg->color_bytes)
     {
       default:
       case 4: 
         ctx_tvg_seek (tvg, tvg->pal_pos + 4 * idx);
         *red   = ctx_tvg_u8 (tvg)/255.0f;
         *green = ctx_tvg_u8 (tvg)/255.0f;
         *blue  = ctx_tvg_u8 (tvg)/255.0f;
         *alpha = ctx_tvg_u8 (tvg)/255.0f;
         break;
       case 2: 
         {
           ctx_tvg_seek (tvg, tvg->pal_pos + 2 * idx);
           uint16_t val = ctx_tvg_u16 (tvg);
           *red     = ((val >> 0) & 31) / 31.0f;
           *green   = ((val >> 5) & 63) / 63.0f;
           *blue    = ((val >> 11)& 31) / 31.0f;
           *alpha   = 1.0f;
         }
         break;
       case 16: 
         ctx_tvg_seek (tvg, tvg->pal_pos + 16 * idx);
         *red   = ctx_tvg_float (tvg);
         *green = ctx_tvg_float (tvg);
         *blue  = ctx_tvg_float (tvg);
         *alpha = ctx_tvg_float (tvg);
         break;
      }
      ctx_tvg_seek (tvg, old_pos);
      return;
   }
#endif
   switch (tvg->color_bytes)
   {
     default:
     case 4: 
         *red   = tvg->pal[4*idx+0]/255.0f;
         *green = tvg->pal[4*idx+1]/255.0f;
         *blue  = tvg->pal[4*idx+2]/255.0f;
         *alpha = tvg->pal[4*idx+3]/255.0f;
         break;
     case 2: 
         {
           uint16_t val = ((uint16_t*)tvg->pal)[idx];
           *red     = ((val >> 0) & 31) / 31.0f;
           *green   = ((val >> 5) & 63) / 63.0f;
           *blue    = ((val >> 11)& 31) / 31.0f;
           *alpha   = 1.0f;
         }
         break;
     case 16: 
         *red   = ((float*)(tvg->pal))[4*idx+0];
         *green = ((float*)(tvg->pal))[4*idx+1];
         *blue  = ((float*)(tvg->pal))[4*idx+2];
         *alpha = ((float*)(tvg->pal))[4*idx+3];
         break;
   }
}

static void ctx_tvg_set_style (CtxTinyVG *tvg, CtxTinyVGStyle *style)
{
  Ctx *ctx = tvg->ctx;
  float x0 = style->x0, y0 = style->y0, x1 = style->x1, y1 = style->y1;
  uint32_t idx0 = style->idx0, idx1 = style->idx1;
  float red, green, blue, alpha;
  switch (style->type)
  {
    case CTX_TVG_FLAT:
      ctx_tvg_get_color (tvg, idx0, &red, &green, &blue, &alpha);
      ctx_rgba (ctx, red, green, blue, alpha);
      break;
    case CTX_TVG_LGRAD:
      ctx_linear_gradient (ctx, x0, y0, x1, y1);
      ctx_tvg_get_color (tvg, idx0, &red, &green, &blue, &alpha);
      ctx_gradient_add_stop (ctx, 0.0, red, green, blue, alpha);
      ctx_tvg_get_color (tvg, idx1, &red, &green, &blue, &alpha);
      ctx_gradient_add_stop (ctx, 1.0, red, green, blue, alpha);
      break;
    case CTX_TVG_RGRAD:
      {
        float radius = ctx_sqrtf ((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
        ctx_radial_gradient (ctx, x0, y0, 0.0, x1, y1, radius);
      }
      ctx_tvg_get_color (tvg, idx0, &red, &green, &blue, &alpha);
      ctx_gradient_add_stop (ctx, 0.0, red, green, blue, alpha);
      ctx_tvg_get_color (tvg, idx1, &red, &green, &blue, &alpha);
      ctx_gradient_add_stop (ctx, 1.0, red, green, blue, alpha);
      break;
  }
}

static void ctx_tvg_path (CtxTinyVG *tvg, int item_count)
{
   int segment_length[item_count];
   for (int i = 0; i < item_count; i ++)
     segment_length[i] = ctx_tvg_var(tvg) + 1;
   for (int i = 0; i < item_count; i ++)
     ctx_tvg_segment (tvg, segment_length[i]);
}

static void ctx_tvg_poly (CtxTinyVG *tvg, int item_count)
{
   Ctx *ctx = tvg->ctx;
   for (int i = 0; i < item_count; i ++)
   {
     float x = ctx_tvg_unit (tvg);
     float y = ctx_tvg_unit (tvg);
     if (i)
       ctx_line_to (ctx, x, y);
     else
       ctx_move_to (ctx, x, y);
   }
}

static void ctx_tvg_rectangles (CtxTinyVG *tvg, int item_count,
                                int fill, int stroke)
{
   Ctx *ctx = tvg->ctx;
   for (int i = 0; i < item_count; i ++)
   {
     float x = ctx_tvg_unit (tvg);
     float y = ctx_tvg_unit (tvg);
     float w = ctx_tvg_unit (tvg);
     float h = ctx_tvg_unit (tvg);
   //  printf ("%f %f %f %f\n", x, y, w, h);
     if (fill)
     {
       ctx_begin_path (ctx);
       ctx_rectangle (ctx, x, y, w, h);
       ctx_tvg_set_style (tvg, &tvg->fill);
       ctx_fill (ctx);
     }
     if (stroke)
     {
       ctx_begin_path (ctx);
       ctx_rectangle (ctx, x, y, w, h);
       ctx_tvg_set_style (tvg, &tvg->stroke);
       ctx_stroke (ctx);
     }
   }
}

static void ctx_tvg_lines (CtxTinyVG *tvg, int item_count)
{
   Ctx *ctx = tvg->ctx;
   for (int i = 0; i < item_count; i ++)
   {
     float x0 = ctx_tvg_unit (tvg);
     float y0 = ctx_tvg_unit (tvg);
     float x1 = ctx_tvg_unit (tvg);
     float y1 = ctx_tvg_unit (tvg);
     ctx_move_to (ctx, x0, y0);
     ctx_line_to (ctx, x1, y1);
   }
}

static int ctx_tvg_command (CtxTinyVG *tvg)
{
  Ctx *ctx = tvg->ctx;
  uint8_t primary_style_type;
  int command = ctx_tvg_u6_u2(tvg, &primary_style_type);
  int item_count;
  float line_width = 0.0;

  int save_offset = 0;

  switch (command)
  {
    case CTX_TVG_FILL_POLYGON:
    case CTX_TVG_FILL_RECTANGLES:
    case CTX_TVG_FILL_PATH:
      item_count = ctx_tvg_var (tvg) + 1;
      ctx_tvg_style (tvg, primary_style_type, &tvg->fill);
      switch (command)
      {
        case CTX_TVG_FILL_POLYGON:
           ctx_tvg_poly (tvg, item_count);
           ctx_tvg_set_style (tvg, &tvg->fill);
           ctx_fill (ctx);
           break;
        case CTX_TVG_FILL_RECTANGLES:
           ctx_tvg_rectangles (tvg, item_count, 1, 0);
           break;
        case CTX_TVG_FILL_PATH:
           ctx_tvg_path (tvg, item_count); 
           ctx_tvg_set_style (tvg, &tvg->fill);
           ctx_fill (ctx);
           break;
      }
      break;

    case CTX_TVG_DRAW_LINES:
    case CTX_TVG_DRAW_LINE_LOOP:
    case CTX_TVG_DRAW_LINE_STRIP:
    case CTX_TVG_DRAW_LINE_PATH:
      item_count = ctx_tvg_var (tvg) + 1;
      ctx_tvg_style (tvg, primary_style_type, &tvg->stroke);
      line_width = ctx_tvg_unit (tvg);

      // XXX incorporate ctx matrix!
      if (line_width < 0.5) line_width = 0.5;
      if (command == CTX_TVG_DRAW_LINE_PATH)
        ctx_tvg_path (tvg, item_count);
      else if (command == CTX_TVG_DRAW_LINES)
        ctx_tvg_lines (tvg, item_count);
      else
        ctx_tvg_poly (tvg, item_count);

      if (command == CTX_TVG_DRAW_LINE_LOOP)
        ctx_close_path (ctx);
      ctx_tvg_set_style (tvg, &tvg->stroke);

      ctx_line_width (ctx, line_width);
      ctx_stroke (ctx);
      break;

    case CTX_TVG_OUTLINE_FILL_RECTANGLES:
      item_count = ctx_tvg_u6_u2 (tvg, &tvg->stroke.type) + 1;
      ctx_tvg_style (tvg, primary_style_type, &tvg->fill);
      ctx_tvg_style (tvg, tvg->stroke.type, &tvg->stroke);
      line_width = ctx_tvg_unit (tvg);

    //  printf ("%i lw:%f %i\n", item_count, line_width, tvg->stroke.type);

      // XXX incorporate ctx matrix!
      if (line_width < 0.5) line_width = 0.5;
      ctx_line_width (ctx, line_width);
      ctx_tvg_rectangles (tvg, item_count, 1, 1);
      break;

    case CTX_TVG_OUTLINE_FILL_POLYGON:
    case CTX_TVG_OUTLINE_FILL_PATH:
      item_count = ctx_tvg_u6_u2 (tvg, &tvg->stroke.type) + 1;
      ctx_tvg_style (tvg, tvg->fill.type, &tvg->fill);
      ctx_tvg_style (tvg, tvg->stroke.type, &tvg->stroke);
      line_width = ctx_tvg_unit (tvg);

      // XXX incorporate ctx matrix!
      if (line_width < 0.5) line_width = 0.5;
      save_offset = tvg->pos;
      if (command == CTX_TVG_OUTLINE_FILL_POLYGON)
        ctx_tvg_poly (tvg, item_count);
      else
        ctx_tvg_path (tvg, item_count);
      ctx_tvg_set_style (tvg, &tvg->fill);
      ctx_fill (ctx);

      ctx_tvg_seek (tvg, save_offset);
      if (command == CTX_TVG_OUTLINE_FILL_POLYGON)
      {
        ctx_tvg_poly (tvg, item_count);
        ctx_close_path (ctx);
      }
      else
        ctx_tvg_path (tvg, item_count);

      if (line_width > 4) line_width =4;
      ctx_line_width (ctx, line_width);
      ctx_tvg_set_style (tvg, &tvg->stroke);
      ctx_stroke (ctx);
      break;
    case CTX_TVG_END_DRAWING:
      break;
  }
  return command;
}

int ctx_tvg_read_header (CtxTinyVG *tvg)
{
   ctx_tvg_seek (tvg, 0);
   if (ctx_tvg_u8 (tvg) != CTX_TVG_MAGIC0) return -1;
   if (ctx_tvg_u8 (tvg) != CTX_TVG_MAGIC1) return -1;
   if (ctx_tvg_u8 (tvg) != CTX_TVG_VERSION) return -1;
   {
     uint8_t val         = ctx_tvg_u8(tvg);
     tvg->scale          = (val>> 0) & 0xf;
     tvg->color_bytes    = (val>> 4) & 0x3;
     tvg->val_type       = (val>> 6) & 0x3;
     tvg->scale_factor   = 1.0f/(1<<(tvg->scale));

     switch (tvg->color_bytes)
     {
       case 0: tvg->color_bytes = 4; break;
       case 1: tvg->color_bytes = 2; break;
       case 2: tvg->color_bytes = 16; break;
     }
   }

   tvg->width = ctx_tvg_val(tvg);
   tvg->height = ctx_tvg_val(tvg);
   return 0;
}


static int
ctx_tvg_draw (CtxTinyVG *tvg)
{
   Ctx *ctx = tvg->ctx;

   tvg->color_count = ctx_tvg_var (tvg);

   tvg->pal_pos = tvg->pos;
   if (tvg->flags & CTX_TVG_FLAG_LOAD_PAL)
   {
     int count = tvg->color_bytes * tvg->color_count;
     tvg->pal = malloc (count);
     for (int i = 0; i < count; i++)
       tvg->pal[i] = ctx_tvg_u8 (tvg);
   }
   else if (!tvg->fd)
   {
     tvg->pal = &tvg->cache[tvg->pos];
     ctx_tvg_seek (tvg, tvg->pos + tvg->color_bytes * tvg->color_count);
   }

   ctx_save (ctx);
   ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
   ctx_line_cap  (ctx, CTX_CAP_ROUND);

   if (tvg->flags & CTX_TVG_FLAG_BBOX_CHECK)
   {
      float minx = 1000000.0;
      float miny = 1000000.0;
      float maxx = -1000000.0;
      float maxy = -1000000.0;
      float x, y;
      x = 0; y =0;
      ctx_device_to_user (ctx, &x, &y);
      ADD_COORD(x,y);
      x = ctx_width (ctx); y =0;
      ctx_device_to_user (ctx, &x, &y);
      ADD_COORD(x,y);
      x = ctx_width (ctx); y = ctx_height (ctx);
      ctx_device_to_user (ctx, &x, &y);
      ADD_COORD(x,y);
      x = 0; y = ctx_height (ctx);
      ctx_device_to_user (ctx, &x, &y);
      ADD_COORD(x,y);

      //fprintf (stderr, "%f %f %f %f\n", minx, miny, maxx, maxy);
      tvg->clipx0 = minx;
      tvg->clipy0 = miny;
      tvg->clipx1 = maxx;
      tvg->clipy1 = maxy;
   }

   while (ctx_tvg_command (tvg));

   ctx_restore (ctx);
   if (tvg->flags & CTX_TVG_FLAG_LOAD_PAL)
     free (tvg->pal);
   return tvg->error;
}

#if 0
static int
ctx_tvg_draw2 (CtxTinyVG *tvg,
               float x, float y,
               float target_width,
               float target_height)
{
   Ctx*ctx = tvg->ctx;
   float scale_x = 1.0;
   float scale_y = 1.0;
   { /* handle aspect ratio, add translate ? */
   if (target_width<=0 && target_height <= 0)
   {
     target_width  = tvg->width;
     target_height = tvg->height;
   }
   else if (target_width<=0 && target_height > 0)
   {
     target_width = target_height / tvg->height * tvg->width;
   }
   else if (target_width<=0 && target_height > 0)
   {
     target_height = target_width / tvg->width * tvg->height;
   }
   scale_x = target_width / tvg->width;
   scale_y = target_height / tvg->height;
   if (scale_x > scale_y)
     scale_x = scale_y;
   else
     scale_y = scale_x;
   }

   ctx_save (ctx);
   ctx_translate (ctx, x, y);
   ctx_scale (ctx, scale_x, scale_y);

   ctx_tvg_draw (tvg);
   ctx_restore (ctx);
   return tvg->error;
}
#endif

int
ctx_tinyvg_get_size (uint8_t *data, int length, int *width, int *height)
{
   CtxTinyVG tvg;
   ctx_tvg_init_data (&tvg, NULL, data, length, 0);
   if (ctx_tvg_read_header (&tvg))
     return -1;
   if (width)*width = tvg.width;
   if (height)*height = tvg.height;
   return 0;
}

int
ctx_tinyvg_fd_get_size (int fd, int *width, int *height)
{
#if CTX_TVG_STDIO
   CtxTinyVG tvg;
   ctx_tvg_init_fd (&tvg, NULL, fd, 0);
   if (ctx_tvg_read_header (&tvg))
     return -1;
   if (width)*width = tvg.width;
   if (height)*height = tvg.height;
   return 0;
#else
   return -1;
#endif
}

int ctx_tinyvg_draw (Ctx     *ctx,
                     uint8_t *data, int length,
                     int      flags)
{
   CtxTinyVG tvg;
   ctx_tvg_init_data (&tvg, ctx, data, length, flags);
   if (ctx_tvg_read_header (&tvg))
     return -1;
   return ctx_tvg_draw (&tvg);
}

int ctx_tinyvg_fd_draw (Ctx *ctx, int fd, int flags)
{
#if CTX_TVG_STDIO
   CtxTinyVG tvg;
   ctx_tvg_init_fd (&tvg, ctx, fd, flags);
   if (ctx_tvg_read_header (&tvg))
     return -1;
   return ctx_tvg_draw (&tvg);
#else
   return -1;
#endif
}

#endif
