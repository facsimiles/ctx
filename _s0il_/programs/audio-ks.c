#include "ui.h"

float render_time = 0.0;
float render_fps  = 0.0;

static uint64_t prev_ticks = 0;

static void clear (Ctx *ctx)
{
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
  ctx_rgba8 (ctx, 0,0,0,255);
  ctx_fill (ctx);
}

#define MAX_PERIOD 1600

typedef struct _Voice Voice;
struct _Voice
{
 int16_t (*render)(void *data);
};

typedef struct _VoiceOsc VoiceOsc;
struct _VoiceOsc
{
 Voice   voice;
 float   hz;
 int     pos;
 int     vel;
 int     period;
 
 float   target_hz;
 int     target_deadline;
};

typedef struct _VoiceString VoiceString;
struct _VoiceString
{
 Voice   voice;
 float   hz;
 int     pos;
 int     vel;
 int     period;

 float   target_hz;
 int     target_deadline;
 float   slide_hz_per_sample;

 int     pluck_mode;

 int16_t buffer[MAX_PERIOD];
 int     bpos;
 int     leap;
 int     leap_limit;


 float   alpha;
 float   dampening;
 float   velocity;
 float   octave;
 float   hit_release;
};

void string_pluck (VoiceString *string)
{
  switch (string->pluck_mode)
  {
    default: case 0:  /* xor */
      for (int i = 0; i < string->period; i++)
        string->buffer[i] = (i * 32412341251) ^ 1234567;
      break;
    case 1:  /* random */
      for (int i = 0; i < string->period; i++)
        string->buffer[i] = (random()&65535)-32767;
      break;
    case 2:  /* code */
      for (int i = 0; i < string->period; i++)
        string->buffer[i] = ((int16_t*)string_pluck)[i];
      break;
  }
  if (string->velocity < 1.0f)
  {
    float velocity = string->velocity; // XXX turn into integer math
    for (int i = 0; i < string->period; i++)
      string->buffer[i] *= velocity;
  }
}

void string_set_hz (VoiceString *string, float hz);
int16_t string_render (VoiceString *string)
{
  int pos      = string->bpos;
  int next_pos = (pos+1)%string->period;

  if (string->target_deadline>0)
  {
    string_set_hz (string, string->hz + string->slide_hz_per_sample);
    string->target_deadline--;
  }

  if (string->leap_limit)
  {
    string->leap--;
    if (string->leap<=0)
    {
      string->leap = string->leap_limit;
      return string->buffer[next_pos];
    }
  }

  {
    float alpha  = string->alpha;
    int ret =   string->buffer[pos     ] * (alpha)
            + string->buffer[next_pos] * (1.0-alpha);

    if (string->dampening) ret *= (1.0f-string->dampening);
    string->buffer[next_pos] = ret;
    string->bpos = next_pos;
    return ret;
  }
}

int sample_rate = 48000;

#include <assert.h>

void string_set_hz (VoiceString *string, float hz)
{
  string->hz = hz;
  float period = sample_rate / hz;
  if (period > MAX_PERIOD)
  {
    period = MAX_PERIOD;
    string->hz = sample_rate / period;
  } else if (period < 16)
  {
    period = 16;
    string->hz = sample_rate / period;
  }
  
  float leap_factor = string->hz - (int)string->hz;
  string->leap_limit = leap_factor!=0.0?(1.0/leap_factor) * period:0;
  //printf ("%.1fhz period:%.11f leap_limit:%i\n", string->hz, period, string->leap_limit);
  if (period > 4800) period = 4800;

  if (string->period < period)
  {
    if (string->period)
    {
      for (int i = string->period; i < period; i++)
        string->buffer[i]=string->buffer[(i-string->period)%string->period];
    }
  }
  string->period = period;
  string->vel = (32768*2)/period; // used for oscillator mode
}

static inline float semitone_to_hz (float tone)
{
  #define NAT_LOG_SEMITONE 0.05776226504666215
  return 440.0f * expf(tone * NAT_LOG_SEMITONE);
}

void string_set_semitone (VoiceString *string, float tone)
{
  string_set_hz (string, semitone_to_hz(tone));
}

void string_set_target_semitone (VoiceString *string, float deadline, float tone)
{
  string->target_hz       = semitone_to_hz(tone);
  if (string->target_hz == string->hz)
  {
    string->target_deadline = 0;
  }
  else
  {
  string->slide_hz_per_sample = ((string->target_hz-string->hz)/(deadline*sample_rate));
  string->target_deadline = deadline * sample_rate;
  }
}

void *string_create (void)
{
  VoiceString *string = calloc(sizeof(VoiceString),1);
  string->voice.render=(void*)string_render;
  string->pos      = 0;
  string->velocity = 1.0f;
  string->alpha    = 0.2f;

  string_set_hz(string, 440);
  return string;
}

void *render_data = NULL;

static void do_pluck (CtxEvent *event, void *data1, void *data2)
{
  VoiceString *string = (VoiceString*)render_data;
  float semitone = data1?(float)atof(data1):0.0f;
  if (string->hit_release > 0.0f)
  {
    string_set_semitone (string, string->octave*12+semitone+12);
    string_pluck (string);
    string_set_target_semitone (string, string->hit_release,
       string->octave*12+semitone);
  }
  else
  {
    string_set_semitone (string, string->octave*12+semitone);
    string_pluck (string);
  }
}

static void do_slide (CtxEvent *event, void *data1, void *data2)
{
  VoiceString *string = (VoiceString*)render_data;
  float semitone = data1?(float)atof(data1):0.0f;
  string_set_target_semitone (string, 0.1, string->octave*12+semitone);
}

static void do_adjust_alpha (CtxEvent *event, void *data1, void *data2)
{
  VoiceString *string = (VoiceString*)render_data;
  float adjustment = data1?(float)atof(data1):0.0f;
  string->alpha+=adjustment;
  printf("alpha: %f\n", (double)string->alpha);
}

static void do_adjust_octave (CtxEvent *event, void *data1, void *data2)
{
  VoiceString *string = (VoiceString*)render_data;
  float adjustment = data1?(float)atof(data1):0.0f;
  string->octave+=adjustment;
  printf("octave: %f\n", (double)string->octave);
}

static void render_audio (Ctx *ctx)
{
   float qlen = ctx_pcm_get_queued_length (ctx);
   int sample_rate = ctx_pcm_get_sample_rate (ctx);

   if (!render_data) render_data = string_create();
   int16_t (*render_fun)(void *data) = ((Voice*)render_data)->render;

#define AUDIO_RENDERAHEAD   0.05

   int width = ctx_width (ctx);
   int height = ctx_height (ctx);
   //((VoiceString*)render_data)->pos = 0;
   ctx_line_width(ctx, -1.0f);
   ctx_gray(ctx, 1.0f);

   ctx_move_to(ctx, 0.0, height * 0.5);

   if (qlen < (AUDIO_RENDERAHEAD))
   {
     float render_len = ((AUDIO_RENDERAHEAD)-qlen)*1.1f;
     int frames = render_len * sample_rate;
     int16_t buf[frames];
     for (int i = 0; i < frames; i++)
     {
       buf[i] = 0;
       buf[i] += render_fun(render_data);
       ctx_line_to(ctx, i*1.0/frames*width, height * 0.5 - (buf[i]/32767.0)*height*0.5f);
     }
     ctx_pcm_queue (ctx, (void*)buf, frames);
   }

   ctx_stroke (ctx);

  ctx_add_key_binding (ctx, "left", NULL, "foo",  do_pluck, "0.0");
  ctx_add_key_binding (ctx, "space", NULL, "foo",  do_pluck, "1.0");
  ctx_add_key_binding (ctx, "right", NULL, "foo",  do_slide, "2.0");
  ctx_add_key_binding (ctx, "page-up", NULL, "foo",  do_adjust_octave, "1");
  ctx_add_key_binding (ctx, "page-down", NULL, "foo",  do_adjust_octave, "-1");
  ctx_add_key_binding (ctx, "up", NULL, "foo",  do_adjust_alpha, "0.01");
  ctx_add_key_binding (ctx, "down", NULL, "foo",  do_adjust_alpha, "-0.01");
}


float _ctx_pause = 0.0;

static void do_fullscreen (CtxEvent *event, void *data1, void *data2)
{
 Ctx *ctx = event->ctx;
 if (ctx_get_fullscreen (ctx))
    ctx_set_fullscreen (ctx, 0);
  else
    ctx_set_fullscreen (ctx, 1);
}

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new (240, 240, NULL);
  prev_ticks = ctx_ticks();
  ctx_pcm_set_format      (ctx, CTX_S16);
  ctx_pcm_set_sample_rate (ctx, sample_rate);

  while(!ctx_has_exited(ctx))
  {
    uint64_t ticks = ctx_ticks ();

    render_time = (ticks - prev_ticks) / 1000.0f/ 1000.0f / 10.0f;
    render_fps = 1.0 / render_time;
    prev_ticks = ticks;

    ctx_start_frame (ctx);

    ctx_save (ctx);
    clear (ctx);
    render_audio (ctx);
    ctx_restore (ctx);
 
    ctx_add_key_binding (ctx, "escape", "exit", "foo",  ui_cb_do, ui_host(ctx));
    ctx_add_key_binding (ctx, "backspace", "exit", "foo",  ui_cb_do, ui_host(ctx));
    ctx_add_key_binding (ctx, "F11", NULL, "foo",   do_fullscreen, NULL);
    ctx_end_frame (ctx);
  }
  ctx_destroy(ctx);
  return 0;
}
