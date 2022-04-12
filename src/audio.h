#ifndef CTX_AUDIO_H
#define CTX_AUDIO_H

#if !__COSMOPOLITAN__
#include <stdint.h>
#endif

/* This enum should be kept in sync with the corresponding mmm enum.
 */
typedef enum {
  CTX_F32,
  CTX_F32S,
  CTX_S16,
  CTX_S16S
} CtxPCM;

void   ctx_pcm_set_format        (Ctx *ctx, CtxPCM format);
CtxPCM ctx_pcm_get_format        (Ctx *ctx);
int    ctx_pcm_get_sample_rate   (Ctx *ctx);
void   ctx_pcm_set_sample_rate   (Ctx *ctx, int sample_rate);
int    ctx_pcm_get_frame_chunk   (Ctx *ctx);
int    ctx_pcm_get_queued        (Ctx *ctx);
float  ctx_pcm_get_queued_length (Ctx *ctx);
int    ctx_pcm_queue             (Ctx *ctx, const int8_t *data, int frames);

#endif
