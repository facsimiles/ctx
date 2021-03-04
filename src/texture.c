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

void ctx_buffer_pixels_free (void *pixels, void *userdata)
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
    buffer->free_func (buffer->data, buffer->user_data);
  if (buffer->eid)
    free (buffer->eid);
  buffer->eid = NULL;
  buffer->data = NULL;
  buffer->free_func = NULL;
  buffer->user_data  = NULL;
}

void ctx_buffer_free (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  free (buffer);
}

static int
ctx_texture_check_eid (Ctx *ctx, const char *eid, int *tw, int *th)
{
  for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
  {
    if (ctx->texture[i].data &&
        ctx->texture[i].eid  &&
        !strcmp (ctx->texture[i].eid, eid))
    {
      if (tw) *tw = ctx->texture[i].width;
      if (th) *th = ctx->texture[i].height;
      ctx->texture[i].frame = ctx->frame;
      return i;
    }
  }
  return -1;
}

const char* ctx_texture_init (Ctx           *ctx,
                              const char    *eid,
                              int            width,
                              int            height,
                              int            stride,
                              CtxPixelFormat format,
                              uint8_t       *pixels,
                              void (*freefunc) (void *pixels, void *user_data),
                              void *user_data)
{
  int id = 0;
  if (eid)
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data &&
          ctx->texture[i].eid &&
          !strcmp (ctx->texture[i].eid, eid))
      {
        ctx->texture[i].frame = ctx->frame;
        if (freefunc && user_data != (void*)23)
          freefunc (pixels, user_data);
        return ctx->texture[i].eid;
      }
      if (ctx->texture[i].data == NULL 
          ||   (ctx->frame - ctx->texture[i].frame > 2))
        id = i;
    }
  } else
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data == NULL 
          || (ctx->frame - ctx->texture[i].frame > 2))
        id = i;
    }
  }
  int bpp = ctx_pixel_format_bpp (format);
  ctx_buffer_deinit (&ctx->texture[id]);

  if (stride<=0)
  {
    stride = width * (bpp/8);
  }

  if (freefunc == ctx_buffer_pixels_free && user_data == (void*)23)
  {
     uint8_t *tmp = malloc (height * stride);
     memcpy (tmp, pixels, height * stride);
     pixels = tmp;
  }

  ctx_buffer_set_data (&ctx->texture[id],
                       pixels, width, height,
                       stride, format,
                       freefunc, user_data);
  ctx->texture[id].frame = ctx->frame;
  if (eid)
  {
    /* we got an eid, this is the fast path */
    ctx->texture[id].eid = strdup (eid);
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
    ctx->texture[id].eid = strdup (ascii);
  }
  return ctx->texture[id].eid;
}

void
ctx_texture_load (Ctx *ctx, const char *path, int *tw, int *th, char *reid)
{
  int id = ctx_texture_check_eid (ctx, path, tw, th);
  if (id>=0)
  {
    return ctx->texture[id].eid;
  }

#ifdef STBI_INCLUDE_STB_IMAGE_H
  int w, h, components;
  unsigned char *data = stbi_load (path, &w, &h, &components, 0);
  if (data)
  {
    CtxPixelFormat pixel_format = CTX_FORMAT_RGBA8;
    switch (components)
    {
      case 1: pixel_format = CTX_FORMAT_GRAY8; break;
      case 2: pixel_format = CTX_FORMAT_GRAYA8; break;
      case 3: pixel_format = CTX_FORMAT_RGB8; break;
      case 4: pixel_format = CTX_FORMAT_RGBA8; break;
    }
    if (tw) *tw = w;
    if (th) *th = h;
    ctx_define_texture (ctx, path, w, h, w * components, pixel_format, data, 
                             reid);
    free (data);
  }
#endif
  return NULL;
}
