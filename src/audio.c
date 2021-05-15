#if CTX_AUDIO

//#include <string.h>
//#include "ctx-internal.h"
//#include "mmm.h"

#include <pthread.h>
#if CTX_ALSA_AUDIO
#include <alsa/asoundlib.h>
#endif
#include <alloca.h>

#define DESIRED_PERIOD_SIZE 1000

int ctx_pcm_bytes_per_frame (CtxPCM format)
{
  switch (format)
  {
    case CTX_f32:  return 4;
    case CTX_f32S: return 8;
    case CTX_s16:  return 2;
    case CTX_s16S: return 4;
    default: return 1;
  }
}

static float    ctx_host_freq     = 48000;
static CtxPCM   ctx_host_format   = CTX_s16S;
static float    client_freq   = 48000;
static CtxPCM   ctx_client_format = CTX_s16S;
static int      ctx_pcm_queued    = 0;
static int      ctx_pcm_cur_left  = 0;
static CtxList *ctx_pcm_list;                 /* data is a blob a 32bit uint first, followed by pcm-data */


//static long int ctx_pcm_queued_ticks = 0;  /*  the number of ticks into the future
  //                                      *  we've queued audio for
                                       


int
ctx_pcm_channels (CtxPCM format)
{
  switch (format)
  {
    case CTX_s16:
    case CTX_f32:
      return 1;
    case CTX_s16S:
    case CTX_f32S:
      return 2;
  }
  return 0;
}

/* todo: only start audio thread on first write - enabling dynamic choice
 * of sample-rate? or is it better to keep to opening 48000 as a standard
 * and do better internal resampling for others?
 */

#if CTX_ALSA_AUDIO
static snd_pcm_t *alsa_open (char *dev, int rate, int channels)
{
   snd_pcm_hw_params_t *hwp;
   snd_pcm_sw_params_t *swp;
   snd_pcm_t *h;
   int r;
   int dir;
   snd_pcm_uframes_t period_size_min;
   snd_pcm_uframes_t period_size_max;
   snd_pcm_uframes_t period_size;
   snd_pcm_uframes_t buffer_size;

   if ((r = snd_pcm_open(&h, dev, SND_PCM_STREAM_PLAYBACK, 0) < 0))
           return NULL;

   hwp = alloca(snd_pcm_hw_params_sizeof());
   memset(hwp, 0, snd_pcm_hw_params_sizeof());
   snd_pcm_hw_params_any(h, hwp);

   snd_pcm_hw_params_set_access(h, hwp, SND_PCM_ACCESS_RW_INTERLEAVED);
   snd_pcm_hw_params_set_format(h, hwp, SND_PCM_FORMAT_S16_LE);
   snd_pcm_hw_params_set_rate(h, hwp, rate, 0);
   snd_pcm_hw_params_set_channels(h, hwp, channels);
   dir = 0;
   snd_pcm_hw_params_get_period_size_min(hwp, &period_size_min, &dir);
   dir = 0;
   snd_pcm_hw_params_get_period_size_max(hwp, &period_size_max, &dir);

   period_size = DESIRED_PERIOD_SIZE;

   dir = 0;
   r = snd_pcm_hw_params_set_period_size_near(h, hwp, &period_size, &dir);
   r = snd_pcm_hw_params_get_period_size(hwp, &period_size, &dir);
   buffer_size = period_size * 4;
   r = snd_pcm_hw_params_set_buffer_size_near(h, hwp, &buffer_size);
   r = snd_pcm_hw_params(h, hwp);
   swp = alloca(snd_pcm_sw_params_sizeof());
   memset(hwp, 0, snd_pcm_sw_params_sizeof());
   snd_pcm_sw_params_current(h, swp);
   r = snd_pcm_sw_params_set_avail_min(h, swp, period_size);
   snd_pcm_sw_params_set_start_threshold(h, swp, 0);
   r = snd_pcm_sw_params(h, swp);
   r = snd_pcm_prepare(h);

   return h;
}

static  snd_pcm_t *h = NULL;
static void *ctx_alsa_audio_start(Ctx *ctx)
{
//  Lyd *lyd = aux;
  int c;

  /* The audio handler is implemented as a mixer that adds data on top
   * of 0s, XXX: it should be ensured that minimal work is there is
   * no data available.
   */
  for (;;)
  {
    int client_channels = ctx_pcm_channels (ctx_client_format);
    int is_float = 0;
    int16_t data[81920*8]={0,};

    if (ctx_client_format == CTX_f32 ||
        ctx_client_format == CTX_f32S)
      is_float = 1;

    c = snd_pcm_wait(h, 1000);

    if (c >= 0)
       c = snd_pcm_avail_update(h);

    if (c > 1000) c = 1000; // should use max mmm buffer sizes

    if (c == -EPIPE)
      snd_pcm_prepare(h);

    if (c > 0)
    {
      int i;
      for (i = 0; i < c && ctx_pcm_cur_left; i ++)
      {
        if (ctx_pcm_cur_left)  //  XXX  this line can be removed
        {
          uint32_t *packet_sizep = (ctx_pcm_list->data);
          uint32_t packet_size = *packet_sizep;
          uint16_t left = 0, right = 0;

          if (is_float)
          {
            float *packet = (ctx_pcm_list->data);
            packet += 4;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;
            left = right = packet[0] * (1<<15);
            if (client_channels > 1)
              right = packet[0] * (1<<15);
          }
          else // s16
          {
            uint16_t *packet = (ctx_pcm_list->data);
            packet += 8;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;

            left = right = packet[0];
            if (client_channels > 1)
              right = packet[1];
          }
          data[i * 2 + 0] = left;
          data[i * 2 + 1] = right;

          ctx_pcm_cur_left--;
          ctx_pcm_queued --;
          if (ctx_pcm_cur_left == 0)
          {
            void *old = ctx_pcm_list->data;
            ctx_list_remove (&ctx_pcm_list, ctx_pcm_list->data);
            free (old);
            ctx_pcm_cur_left = 0;
            if (ctx_pcm_list)
            {
              uint32_t *packet_sizep = (ctx_pcm_list->data);
              uint32_t packet_size = *packet_sizep;
              ctx_pcm_cur_left = packet_size;
            }
          }
        }
      }

    c = snd_pcm_writei(h, data, c);
    if (c < 0)
      c = snd_pcm_recover (h, c, 0);
     }else{
      if (getenv("LYD_FATAL_UNDERRUNS"))
        {
          printf ("dying XXxx need to add API for this debug\n");
          //printf ("%i", lyd->active);
          exit(0);
        }
      fprintf (stderr, "ctx alsa underun\n");
      //exit(0);
    }
  }
}
#endif

static char MuLawCompressTable[256] =
{
   0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static unsigned char LinearToMuLawSample(int16_t sample)
{
  const int cBias = 0x84;
  const int cClip = 32635;
  int sign = (sample >> 8) & 0x80;

  if (sign)
    sample = (int16_t)-sample;

  if (sample > cClip)
    sample = cClip;

  sample = (int16_t)(sample + cBias);

  int exponent = (int)MuLawCompressTable[(sample>>7) & 0xFF];
  int mantissa = (sample >> (exponent+3)) & 0x0F;

  int compressedByte = ~ (sign | (exponent << 4) | mantissa);

  return (unsigned char)compressedByte;
}

void ctx_ctx_pcm (Ctx *ctx)
{
    int client_channels = ctx_pcm_channels (ctx_client_format);
    int is_float = 0;
    uint8_t data[81920*8]={0,};
    int c;

    if (ctx_client_format == CTX_f32 ||
        ctx_client_format == CTX_f32S)
      is_float = 1;

    c = 2000;

    if (c > 0)
    {
      int i;
      for (i = 0; i < c && ctx_pcm_cur_left; i ++)
      {
        if (ctx_pcm_cur_left)  //  XXX  this line can be removed
        {
          uint32_t *packet_sizep = (ctx_pcm_list->data);
          uint32_t packet_size = *packet_sizep;
          int left = 0, right = 0;

          if (is_float)
          {
            float *packet = (ctx_pcm_list->data);
            packet += 4;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;
            left = right = packet[0] * (1<<15);
            if (client_channels > 1)
              right = packet[1] * (1<<15);
          }
          else // s16
          {
            uint16_t *packet = (ctx_pcm_list->data);
            packet += 8;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;

            left = right = packet[0];
            if (client_channels > 1)
              right = packet[1];
          }
          data[i] = LinearToMuLawSample((left+right)/2);

          ctx_pcm_cur_left--;
          ctx_pcm_queued --;
          if (ctx_pcm_cur_left == 0)
          {
            void *old = ctx_pcm_list->data;
            ctx_list_remove (&ctx_pcm_list, ctx_pcm_list->data);
            free (old);
            ctx_pcm_cur_left = 0;
            if (ctx_pcm_list)
            {
              uint32_t *packet_sizep = (ctx_pcm_list->data);
              uint32_t packet_size = *packet_sizep;
              ctx_pcm_cur_left = packet_size;
            }
          }
        }
      }

    char encoded[81920*8]="";

    int encoded_len = ctx_a85enc (data, encoded, i);
    fprintf (stdout, "\033_Af=%i;", i);
    fwrite (encoded, 1, encoded_len, stdout);
    fwrite ("\e\\", 1, 2, stdout);
    fflush (stdout);
    }
}

int ctx_pcm_init (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return 0;
  }
  else
#endif
  if (ctx_renderer_is_ctx (ctx))
  {
     ctx_host_freq = 8000;
     ctx_host_format = CTX_s16;
#if 0
     pthread_t tid;
     pthread_create(&tid, NULL, (void*)ctx_audio_start, ctx);
#endif
  }
  else
  {
#if CTX_ALSA_AUDIO
     pthread_t tid;
     h = alsa_open("default", ctx_host_freq, ctx_pcm_channels (ctx_host_format));
  if (!h) {
    fprintf(stderr, "ctx unable to open ALSA device (%d channels, %f Hz), dying\n",
            ctx_pcm_channels (ctx_host_format), ctx_host_freq);
    return -1;
  }
  pthread_create(&tid, NULL, (void*)ctx_alsa_audio_start, ctx);
#endif
  }
  return 0;
}

int ctx_pcm_queue (Ctx *ctx, const int8_t *data, int frames)
{
  static int inited = 0;
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_queue (ctx->backend_data, data, frames);
  }
  else
#endif
  {
    if (!inited)
    {
      ctx_pcm_init (ctx);
      inited = 1;
    }
    float factor = client_freq * 1.0 / ctx_host_freq;
    int   scaled_frames = frames / factor;
    int   bpf = ctx_pcm_bytes_per_frame (ctx_client_format);

    uint8_t *packet = malloc (scaled_frames * ctx_pcm_bytes_per_frame (ctx_client_format) + 16);
    *((uint32_t *)packet) = scaled_frames;

    if (factor > 0.999 && factor < 1.0001)
    {
       memcpy (packet + 16, data, frames * bpf);
    }
    else
    {
      /* a crude nearest / sample-and hold resampler */
      int i;
      for (i = 0; i < scaled_frames; i++)
      {
        int source_frame = i * factor;
        memcpy (packet + 16 + bpf * i, data + source_frame * bpf, bpf);
      }
    }
    if (ctx_pcm_list == NULL)     // otherwise it is another frame at front
      ctx_pcm_cur_left = scaled_frames;  // and current cur_left is valid

    ctx_list_append (&ctx_pcm_list, packet);
    ctx_pcm_queued += scaled_frames;

    return frames;
  }
  return 0;
}

static int ctx_pcm_get_queued_frames (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_queued_frames (ctx->backend_data);
  }
#endif
  return ctx_pcm_queued;
}

int ctx_pcm_get_queued (Ctx *ctx)
{
  return ctx_pcm_get_queued_frames (ctx);
}

float ctx_pcm_get_queued_length (Ctx *ctx)
{
  return 1.0 * ctx_pcm_get_queued_frames (ctx) / ctx_host_freq;
}

int ctx_pcm_get_frame_chunk (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_frame_chunk (ctx->backend_data);
  }
#endif
  if (ctx_renderer_is_ctx (ctx))
  {
    // 300 stuttering
    // 350 nothing
    // 380 slight buzz
    // 390  buzzing
    // 400 ok - but sometimes falling out
    // 410 buzzing
    // 420 ok - but odd latency
    // 450 buzzing

    if (ctx_pcm_get_queued_frames (ctx) > 400)
      return 0;
    else
      return 400 - ctx_pcm_get_queued_frames (ctx);

  }

  if (ctx_pcm_get_queued_frames (ctx) > 1000)
    return 0;
  else
    return 1000 - ctx_pcm_get_queued_frames (ctx);
}

void ctx_pcm_set_sample_rate (Ctx *ctx, int sample_rate)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    mmm_pcm_set_sample_rate (ctx->backend_data, sample_rate);
  }
  else
#endif
    client_freq = sample_rate;
}

void ctx_pcm_set_format (Ctx *ctx, CtxPCM format)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    mmm_pcm_set_format (ctx->backend_data, format);
  }
  else
#endif
    ctx_client_format = format;
}

CtxPCM ctx_pcm_get_format (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_format (ctx->backend_data);
  }
#endif
  return ctx_client_format;
}

int ctx_pcm_get_sample_rate (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_sample_rate (ctx->backend_data);
  }
#endif
  return client_freq;
}

#endif
