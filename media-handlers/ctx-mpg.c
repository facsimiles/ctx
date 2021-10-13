/*
PL_MPEG Example - Video player using SDL2/OpenGL for rendering
refactored to use ctx for audio output/rendering by Øyvind Kolås, 2020

Dominic Szablewski - https://phoboslab.org


-- LICENSE: The MIT License(MIT)

Copyright(c) 2019 Dominic Szablewski

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


-- Usage

plmpeg-player <video-file.mpg>

Use the arrow keys to seek forward/backward by 3 seconds. Click anywhere on the
window to seek to seek through the whole file.


-- About

This program demonstrates a simple video/audio player using plmpeg for decoding
and SDL2 with OpenGL for rendering and sound output. It was tested on Windows
using Microsoft Visual Studio 2015 and on macOS using XCode 10.2

This program can be configured to either convert the raw YCrCb data to RGB on
the GPU (default), or to do it on CPU. Just pass APP_TEXTURE_MODE_RGB to
app_create() to switch to do the conversion on the CPU.

YCrCb->RGB conversion on the CPU is a very costly operation and should be
avoided if possible. It easily takes as much time as all other mpeg1 decoding
steps combined.

*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//#include <SDL2/SDL.h>

//#define CTX_RASTERIZER  1
//#define CTX_ALSA_AUDIO  1
#include "ctx.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define APP_TEXTURE_MODE_YCRCB 1
#define APP_TEXTURE_MODE_RGB 2

typedef struct {
	plm_t   *plm;
	double   last_time;
	int      wants_to_quit;
	int      texture_mode;
	uint8_t *rgb_data;
        Ctx     *ctx;
} app_t;

int yuv420 = 1; 
int smoothing = 0;

app_t * app_create(const char *filename, int texture_mode);
void app_update(app_t *self);
void app_destroy(app_t *self);

void app_on_video(plm_t *player, plm_frame_t *frame, void *user);
void app_on_audio(plm_t *player, plm_samples_t *samples, void *user);

#define SDL_Log(fmt, args...) fprintf(stderr, fmt, ##args)

app_t * app_create(const char *filename, int texture_mode) {
	app_t *self = (app_t *)malloc(sizeof(app_t));
	memset(self, 0, sizeof(app_t));
	
	self->texture_mode = texture_mode;
	
	// Initialize plmpeg, load the video file, install decode callbacks
	self->plm = plm_create_with_filename(filename);
	if (!self->plm) {
		SDL_Log("Couldn't open %s", filename);
		exit(1);
	}

	int samplerate = plm_get_samplerate(self->plm);

	SDL_Log(
		"Opened %s - framerate: %f, samplerate: %d, duration: %f",
		filename, 
		plm_get_framerate(self->plm),
		plm_get_samplerate(self->plm),
		plm_get_duration(self->plm)
	);

        self->ctx = ctx_new_ui (-1, -1);
	
	plm_set_video_decode_callback(self->plm, app_on_video, self);
	plm_set_audio_decode_callback(self->plm, app_on_audio, self);
	
	plm_set_loop(self->plm, TRUE);
	plm_set_audio_enabled(self->plm, TRUE);
	plm_set_audio_stream(self->plm, 0);

	if (plm_get_num_audio_streams(self->plm) > 0) {
                /*
		SDL_AudioSpec audio_spec;
		audio_spec.freq = samplerate;
		audio_spec.format = AUDIO_F32;
		audio_spec.channels = 2;
		audio_spec.samples = 4096;
                */
                ctx_pcm_set_sample_rate (self->ctx, samplerate);
                ctx_pcm_set_format (self->ctx, CTX_f32S);

		plm_set_audio_lead_time(self->plm, (double)4096.0 / (double)samplerate);
	}
        {
		int num_pixels = plm_get_width(self->plm) * plm_get_height(self->plm);
		self->rgb_data = (uint8_t*)malloc(num_pixels * 3);
	}
	
	return self;
}

void app_destroy(app_t *self) {
	plm_destroy(self->plm);
	
	if (self->texture_mode == APP_TEXTURE_MODE_RGB) {
		free(self->rgb_data);
	}
        ctx_free (self->ctx);
	
	free(self);
}

int paused = 0;

void app_update(app_t *self) {
	double seek_to = -1;

        const CtxEvent *event = NULL;

        while ((event = ctx_get_event(self->ctx)))
        {
          switch (event->type)
          {
            case CTX_KEY_PRESS:
              if (!strcmp (event->string, "q")) {
                self->wants_to_quit = TRUE;
              }
              else if (!strcmp (event->string, "left")) {
		 seek_to = plm_get_time(self->plm) - 2;
              }
              else if (!strcmp (event->string, "right")) {
		 seek_to = plm_get_time(self->plm) + 2;
              }
              else if (!strcmp (event->string, "down")) {
		 seek_to = plm_get_time(self->plm) - 10;
              }
              else if (!strcmp (event->string, "up")) {
		 seek_to = plm_get_time(self->plm) + 10;
              }
              else if (!strcmp (event->string, "space")) {
		 paused = !paused;
              }
              break;
            default:
              break;
          }
        }

	// Compute the delta time since the last app_update(), limit max step to 
	// 1/30th of a second
	double current_time = (double)ctx_ticks() / 1000.0 / 1000.0;
	double elapsed_time = current_time - self->last_time;
#if 1
	if (elapsed_time > 1.0 / 25.0) {
		elapsed_time = 1.0 / 25.0;
	}

        if (elapsed_time < 1.0 / 60.0) usleep (1000);
#endif
        if (paused) elapsed_time = 0;
	self->last_time = current_time;

	// Seek or advance decode
	if (seek_to != -1) {
//		SDL_ClearQueuedAudio(self->audio_device);
		plm_seek(self->plm, seek_to, FALSE);
	}
	else {
		plm_decode(self->plm, elapsed_time);
	}

	if (plm_has_ended(self->plm)) {
		self->wants_to_quit = TRUE;
	}
}

static int frame_no = 0;
static int frame_drop = 1;

void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
	app_t *self = (app_t *)user;

        frame_no ++;
        char eid[16];
        sprintf (eid, "%i", frame_no);
        if (frame_no % frame_drop != 0) return;
        ctx_reset (self->ctx);
        ctx_save (self->ctx);
        ctx_rectangle (self->ctx, 0, 0, ctx_width (self->ctx), ctx_height (self->ctx));
  float scale = ctx_width (self->ctx) * 1.0 / frame->width;
  float scaleh = ctx_height (self->ctx) * 1.0 / frame->height;
  if (scaleh < scale) scale = scaleh;
  ctx_translate (self->ctx, (ctx_width(self->ctx)-frame->width*scale)/2.0, 
                      (ctx_height(self->ctx)-frame->height*scale)/2.0);
  ctx_scale (self->ctx, scale, scale);

  if (yuv420)
  {
     int data_len = frame->width *  frame->height +
                 2 *((frame->width/2) * (frame->height/2));
     uint8_t *data=malloc (data_len);
     memcpy (data, frame->y.data, frame->width *  frame->height);
     memcpy (data + frame->width *  frame->height, frame->cb.data, (frame->width/2) * (frame->height/2));
     memcpy (data + frame->width *  frame->height + (frame->width/2)*(frame->height/2), frame->cr.data, (frame->width/2) * (frame->height/2));

  ctx_define_texture (self->ctx,
                      eid, // by passing in a unique eid
                           // we avoid having to hash
                      frame->width, frame->height,
                      frame->width,
                      CTX_FORMAT_YUV420,
                      data,
                      NULL);
    free (data);
  }
  else {
    plm_frame_to_rgb(frame, self->rgb_data, frame->width * 3);
    ctx_define_texture (self->ctx,
                        eid, // by passing in a unique eid
                             // we avoid having to hash
                        frame->width, frame->height,
                        frame->width * 3,
                        CTX_FORMAT_RGB8,
                        self->rgb_data,
                        NULL);
  }
  ctx_image_smoothing (self->ctx, smoothing);
  ctx_compositing_mode (self->ctx, CTX_COMPOSITE_COPY);
  ctx_fill (self->ctx);
  ctx_restore (self->ctx);
  ctx_flush (self->ctx);
}

void app_on_audio(plm_t *mpeg, plm_samples_t *samples, void *user) {
	app_t *self = (app_t *)user;
        ctx_pcm_queue (self->ctx, (const signed char*)(samples->interleaved), samples->count);
}

int ctx_mpg_main(int argc, char *argv[]) {
        const char *path = NULL;

	if (argc < 2) {
		SDL_Log("Usage: ctx mpg <file.mpg>");
		exit(1);
	}
        if (getenv ("FRAMEDROP"))frame_drop = atoi (getenv("FRAMEDROP"));
        if (frame_drop < 1) frame_drop = 1;
        if (frame_drop > 32) frame_drop = 32;

        //char *path = NULL;
       
        for (int i = 1; i <  argc; i++)
        {
          if (argv[i][0] == '-')
          {
            if (argv[i][1] == 'r')
            {
              yuv420 = 0;
            }
            else if (argv[i][1] == 's')
            {
              smoothing = 1;
            }
          }
          else
          {
            path = argv[i];
          }
        }
         //= argv[1];
        if (path && strchr (path, ':'))
        {
          path = strchr (path, ':');
          if (path[1] == '/') path++;
          if (path[1] == '/') path++;
        }
	
        app_t *app = app_create(path,
                         yuv420?APP_TEXTURE_MODE_YCRCB:APP_TEXTURE_MODE_RGB);

	while (!app->wants_to_quit) {
		app_update(app);
	}
	app_destroy(app);
	
	return EXIT_SUCCESS;
}
