#include "ctx-split.h"

CtxBuffer *ctx_buffer_new_bare (void)
{
  CtxBuffer *buffer = (CtxBuffer *) ctx_calloc (sizeof (CtxBuffer), 1);
  return buffer;
}

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          void (*freefunc) (void *pixels, void *user_data),
                          void *user_data)
{
  if (buffer->free_func)
    { buffer->free_func (buffer->data, buffer->user_data); }
  if (stride <= 0)
    stride = ctx_pixel_format_get_stride (pixel_format, width);
  buffer->data      = data;
  buffer->width     = width;
  buffer->height    = height;
  buffer->stride    = stride;
  buffer->format    = ctx_pixel_format_info (pixel_format);
  buffer->free_func = freefunc;
  buffer->user_data = user_data;
}

CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                                    void (*freefunc) (void *pixels, void *user_data),
                                    void *user_data)
{
  CtxBuffer *buffer = ctx_buffer_new_bare ();
  ctx_buffer_set_data (buffer, data, width, height, stride, pixel_format,
                       freefunc, user_data);
  return buffer;
}

void ctx_buffer_pixels_free (void *pixels, void *userdata)
{
  ctx_free (pixels);
}

CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format)
{
  //CtxPixelFormatInfo *info = ctx_pixel_format_info (pixel_format);
  CtxBuffer *buffer = ctx_buffer_new_bare ();
  int stride = ctx_pixel_format_get_stride (pixel_format, width);
  int data_len = stride * height;
  if (pixel_format == CTX_FORMAT_YUV420)
    data_len = width * height + ((width/2) * (height/2)) * 2;

  uint8_t *pixels = (uint8_t*)ctx_calloc (data_len, 1);

  ctx_buffer_set_data (buffer, pixels, width, height, stride, pixel_format,
                       ctx_buffer_pixels_free, NULL);
  return buffer;
}

static void ctx_buffer_deinit (CtxBuffer *buffer)
{
  if (buffer->free_func)
    buffer->free_func (buffer->data, buffer->user_data);
  if (buffer->eid)
  {
    ctx_free (buffer->eid);
  }
  buffer->eid = NULL;
  buffer->data = NULL;
  buffer->free_func = NULL;
  buffer->user_data  = NULL;
  if (buffer->color_managed)
  {
    if (buffer->color_managed != buffer)
    {
      ctx_buffer_destroy (buffer->color_managed);
    }
    buffer->color_managed = NULL;
  }
}

void ctx_buffer_destroy (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  ctx_free (buffer);
}

#if 0
static int
ctx_texture_check_eid (Ctx *ctx, const char *eid, int *tw, int *th)
{
  for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
  {
    if (ctx->texture[i].data &&
        ctx->texture[i].eid  &&
        !ctx_strcmp (ctx->texture[i].eid, eid))
    {
      if (tw) *tw = ctx->texture[i].width;
      if (th) *th = ctx->texture[i].height;
      ctx->texture[i].frame = ctx->texture_cache->frame;
      return i;
    }
  }
  return -1;
}
#endif

const char* ctx_texture_init (Ctx           *ctx,
                              const char    *eid,
                              int            width,
                              int            height,
                              int            stride,
                              CtxPixelFormat format,
                              void          *space,
                              uint8_t       *pixels,
                              void (*freefunc) (void *pixels, void *user_data),
                              void          *user_data)
{
  int id = 0;
  if (eid)
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data &&
          ctx->texture[i].eid &&
          !ctx_strcmp (ctx->texture[i].eid, eid))
      {
        ctx->texture[i].frame = ctx->texture_cache->frame;
        if (freefunc && user_data != (void*)23)
          freefunc (pixels, user_data);
        return ctx->texture[i].eid;
      }
      if (ctx->texture[i].data == NULL 
          ||   (ctx->texture_cache->frame - ctx->texture[i].frame >= 2))
        id = i;
    }
  } else
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data == NULL 
          || (ctx->texture_cache->frame - ctx->texture[i].frame > 2))
        id = i;
    }
  }
  //int bpp = ctx_pixel_format_bits_per_pixel (format);
  ctx_buffer_deinit (&ctx->texture[id]);

  if (stride<=0)
  {
    stride = ctx_pixel_format_get_stride ((CtxPixelFormat)format, width);
  }

  int data_len = stride * height;
  if (format == CTX_FORMAT_YUV420)
          data_len = width * height +
                  2 * ((width/2)*(height/2));

  if (freefunc == ctx_buffer_pixels_free && user_data == (void*)23)
  {
     uint8_t *tmp = (uint8_t*)ctx_malloc (data_len);
     memcpy (tmp, pixels, data_len);
     pixels = tmp;
  }

  ctx_buffer_set_data (&ctx->texture[id],
                       pixels, width, height,
                       stride, format,
                       freefunc, user_data);
#if CTX_ENABLE_CM
  ctx->texture[id].space = space;
#endif
  ctx->texture[id].frame = ctx->texture_cache->frame;
  if (eid)
  {
    /* we got an eid, this is the fast path */
    ctx->texture[id].eid = ctx_strdup (eid);
  }
  else
  {
    uint8_t hash[20];
    char ascii[41];

    CtxSHA1 *sha1 = ctx_sha1_new ();
    ctx_sha1_process (sha1, pixels, stride * height);
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    ctx->texture[id].eid = ctx_strdup (ascii);
  }
  return ctx->texture[id].eid;
}

void
_ctx_texture_prepare_color_management (CtxState      *state,
                                       CtxBuffer     *buffer)
{
// _ctx_texture_lock ();
   switch (buffer->format->pixel_format)
   {
#if CTX_BABL
     case CTX_FORMAT_RGBA8:
       if (buffer->space == state->gstate.device_space)
       {
         buffer->color_managed = buffer;
       }
       else
       {
          CtxBuffer *color_managed = ctx_buffer_new (buffer->width, buffer->height,
                                                  CTX_FORMAT_RGBA8);
          babl_process (
             babl_fish (babl_format_with_space ("R'G'B'A u8", buffer->space),
                        babl_format_with_space ("R'G'B'A u8", state->gstate.device_space)),
             buffer->data, color_managed->data,
             buffer->width * buffer->height
             );
          buffer->color_managed = color_managed;
       }
       break;
     case CTX_FORMAT_RGB8:
       if (buffer->space == state->gstate.device_space)
       {
         buffer->color_managed = buffer;
       }
       else
       {
         CtxBuffer *color_managed = ctx_buffer_new (buffer->width, buffer->height,
                                               CTX_FORMAT_RGB8);
         babl_process (
            babl_fish (babl_format_with_space ("R'G'B' u8", buffer->space),
                       babl_format_with_space ("R'G'B' u8", state->gstate.device_space)),
            buffer->data, color_managed->data,
            buffer->width * buffer->height
          );
         buffer->color_managed = color_managed;
       }
       break;
#endif
     default:
       buffer->color_managed = buffer;
   }
//  _ctx_texture_unlock ();
}


