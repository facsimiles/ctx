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


static void ctx_buffer_pixels_free (void *pixels, void *userdata)
{
  free (pixels);
}

CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format)
{
  CtxPixelFormatInfo *info = ctx_pixel_format_info (pixel_format);
  CtxBuffer *buffer = ctx_buffer_new_bare ();
  int stride = width * info->ebpp;
  uint8_t *pixels = (uint8_t*)ctx_calloc (stride, height + 1);

  ctx_buffer_set_data (buffer, pixels, width, height, stride, pixel_format,
                       ctx_buffer_pixels_free, NULL);
  return buffer;
}

void ctx_buffer_deinit (CtxBuffer *buffer)
{
  if (buffer->free_func)
    { buffer->free_func (buffer->data, buffer->user_data); }
  buffer->data = NULL;
  buffer->free_func = NULL;
  buffer->user_data  = NULL;
}

void ctx_buffer_free (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  free (buffer);
}

/* load the png into the buffer */
static int ctx_buffer_load_png (CtxBuffer *buffer,
                                const char *path,
                                int *tw, int *th)
{
  ctx_buffer_deinit (buffer);
#ifdef STBI_INCLUDE_STB_IMAGE_H
  int w, h, components;
  unsigned char *data = stbi_load (path, &w, &h, &components, 0);
  if (data)
  {
    buffer->width = w;
    buffer->height = h;
    buffer->data = data;
    buffer->stride = w * components;
  switch (components)
    {
      case 1:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAY8);
        break;
      case 2:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAYA8);
        break;
      case 3:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGB8);
        break;
      case 4:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGBA8);
        break;
    }
  buffer->free_func = (void *) free;
  buffer->user_data = NULL;
  if (tw) *tw = buffer->width;
  if (th) *th = buffer->height;
  return 0;
  }
  else
  {
    return -1;
  }
#else
  if (path) {};
  return -1;
#endif
}

void ctx_texture_release (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
    { return; }
  ctx_buffer_deinit (&ctx->texture[id]);
}

static int ctx_allocate_texture_id (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
    {
      for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
        if (ctx->texture[i].data == NULL)
          { return i; }
      int sacrifice = random()%CTX_MAX_TEXTURES; // better to bias towards old
      ctx_texture_release (ctx, sacrifice);
      return sacrifice;
      return -1; // eeek
    }
  return id;
}

/* load the png into the buffer */
static int ctx_buffer_load_memory (CtxBuffer *buffer,
                                   const char *data,
                                   int length,
                                   int *width,
                                   int *height)
{
  ctx_buffer_deinit (buffer);
#ifdef UPNG_H
  upng_t *upng = upng_new_from_bytes (data, length);
  int components;
  if (upng == NULL)
    { return -1; }
  upng_header (upng);
  upng_decode (upng);
  components     = upng_get_components (upng);
  buffer->width  = upng_get_width (upng);
  buffer->height = upng_get_height (upng);
  buffer->data   = upng_steal_buffer (upng);
  upng_free (upng);
  buffer->stride = buffer->width * components;
  switch (components)
    {
      case 1:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAY8);
        break;
      case 2:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAYA8);
        break;
      case 3:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGB8);
        break;
      case 4:
        buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGBA8);
        break;
    }
  buffer->free_func = (void *) free;
  buffer->user_data = NULL;
  return 0;
#else
  if (data && length) {};
  return -1;
#endif
}

int
ctx_texture_load_memory (Ctx *ctx, int id, const char *data, int length, int *width, int *height)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  if (ctx_buffer_load_memory (&ctx->texture[id], data, length, width, height) )
    {
      return -1;
    }
  return id;
}

int
ctx_texture_load (Ctx *ctx, int id, const char *path, int *width, int *height)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  if (ctx_buffer_load_png (&ctx->texture[id], path, width, height) )
    {
      return -1;
    }
  return id;
}

int ctx_texture_init (Ctx *ctx,
                      int  id,
                      int  width,
                      int  height,
                      int  stride,
                      CtxPixelFormat format,
                      uint8_t       *pixels,
                      void (*freefunc) (void *pixels, void *user_data),
                      void *user_data)
{
  /* .. how to relay? ..
   * fully serializing is one needed option - for that there is no free
   * func..
   *
   * mmap texute bank - that is one of many in compositor, prefixed with "pid-",
   * ... we want string identifiers instead of integers.
   *
   * a context to use as texturing source
   *   implemented.
   */
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  int bpp = ctx_pixel_format_bpp (format);
  ctx_buffer_deinit (&ctx->texture[id]);

  if (!stride)
  {
    stride = width * (bpp/8);
  }

  ctx_buffer_set_data (&ctx->texture[id],
                       pixels, width, height,
                       stride, format,
                       freefunc, user_data);
  return id;
}
