#define _DEFAULT_SOURCE

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mmm.h"
#include "mmm-pset.h"

#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

#define CTX_GRADIENT_CACHE       1
#define CTX_SHAPE_CACHE          1
#define CTX_SHAPE_CACHE_DIM      64*64
#define CTX_SHAPE_CACHE_ENTRIES  2048
#define CTX_RASTERIZER_AA        3
#define CTX_RASTERIZER_FORCE_AA  0
#define CTX_IMPLEMENTATION
#include "ctx.h"
#include "ctx-string.h"

#include "ctx-vt.h"

int   do_quit      = 0;
float font_size    = 30.0;
float line_spacing = 2.0;

static Mmm *mmm = NULL;
static pid_t vt_child;
static MrgVT *vt = NULL;

void
signal_child (int signum)
{
  pid_t pid;
  int   status;
  while ((pid = waitpid(-1, &status, WNOHANG)) != -1)
    {
      if (pid)
      {
      if (pid == vt_child)
      {
	exit(0);
        do_quit = 1;
        return;
      }
      else
      {
        fprintf (stderr, "q? %i %i\n", pid, vt_child);
      }
    }
    }
}


static int16_t pcm_queue[1<<18];
static int pcm_write_pos = 0;
static int pcm_read_pos = 0;


void terminal_queue_pcm_sample (int16_t sample)
{
  if (pcm_write_pos >= (1<<18)-1)
  {
    /*  TODO  :  fix cyclic buffer */
    pcm_write_pos = 0;
    pcm_read_pos = 0;
  }
  pcm_queue[pcm_write_pos++]=sample;
  pcm_queue[pcm_write_pos++]=sample;
}

void audio_task (int click)
{
  int free_frames = mmm_pcm_get_frame_chunk (mmm)+24;
  //int free_frames = mmm_pcm_get_free_frames (mmm);
  int queued = (pcm_write_pos - pcm_read_pos)/2;
  if (free_frames > 3) free_frames --;
  int frames = queued;

#if 0
  if (click)
  {
    // do we need to queue silence at other times for the flow
    // to work? seem like an mmm bug...
    //
    float click_volume = 0.4;
    int16_t pcm_data[]={-32000 * click_volume,32000 * click_volume,0,0};
    mmm_pcm_queue (mmm, (int8_t*) pcm_data, 2);
  }
#endif

  if (frames > free_frames) frames = free_frames;
  if (frames > 0)
  {
    mmm_pcm_queue (mmm, (int8_t*)&pcm_queue[pcm_read_pos], frames);
    pcm_read_pos += frames*2;
  }
}

int vt_main(int argc, char **argv)
{
  int cw = (font_size / line_spacing) + 0.99;
  int ch = font_size;
  int old_w = 0;
  int old_h = 0;
  mmm = mmm_new (cw * DEFAULT_COLS, ch * DEFAULT_ROWS, 0, NULL);
  unsetenv ("MMM_PATH");
  vt = ctx_vt_new (argv[1]?argv[1]:ctx_vt_find_shell_command());

  ctx_vt_set_mmm (vt, mmm);
  mmm_pcm_set_sample_rate (mmm, 8000);

  int sleep_time = 10;

  vt_child = ctx_vt_get_pid (vt);
  signal (SIGCHLD, signal_child);
  while(!do_quit)
  {
      unsigned char *buffer;
      int width; int height; int stride;

      static long drawn_rev = 0;
      if (drawn_rev != ctx_vt_rev (vt))
      {
        drawn_rev = ctx_vt_rev (vt);

      mmm_client_check_size (mmm, &width, &height);

      if (old_w != width ||  old_h!=height)
      {
        ctx_vt_set_term_size (vt, width / (font_size/line_spacing), height / font_size);
	old_w = width;
	old_h = height;
      }

        buffer = mmm_get_buffer_write (mmm, &width, &height, &stride, NULL);

        Ctx *ctx = ctx_new_for_framebuffer (buffer, width, height, stride, CTX_FORMAT_BGRA8);

        ctx_vt_draw (vt, ctx, 0, 0, font_size, line_spacing);

        ctx_free (ctx);
        mmm_write_done (mmm, 0, 0, -1, -1);
        audio_task (0);
      }

      int got_event = 0;
      while (mmm_has_event (mmm))
      {
        const char *event = mmm_get_event (mmm);
	//if (!strcmp (event, "shift-return"))
	 // event = "return";
	//else
	       	if (!strcmp (event, "shift-page-up"))
	{
	  int new_scroll = ctx_vt_get_scroll (vt) + ctx_vt_get_rows (vt)/2;
	  if (new_scroll > 200) new_scroll = 200;
	  ctx_vt_set_scroll (vt, new_scroll);
	  ctx_vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-page-down"))
	{
	  int new_scroll = ctx_vt_get_scroll (vt) - ctx_vt_get_rows (vt)/2;
	  if (new_scroll < 0) new_scroll = 0;
	  ctx_vt_set_scroll (vt, new_scroll);
	  ctx_vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-control--")) {
	  font_size /= 1.15;
	  font_size = (int) (font_size);
	  if (font_size < 5) font_size = 5;

          cw = (font_size / line_spacing) + 0.99;
          ch = font_size;
          ctx_vt_set_term_size (vt, width / (font_size/line_spacing), height / font_size);
	} else if (!strcmp (event, "shift-control-=")) {
	  float old = font_size;
	  font_size *= 1.15;
	  font_size = (int)(font_size);
	  if (old == font_size) font_size = old+1;
	  if (font_size > 200) font_size = 200;
          ctx_vt_set_term_size (vt, width / (font_size/line_spacing), height / font_size);

          cw = (font_size / line_spacing) + 0.99;
          ch = font_size;
	}
#if 0
        else if (!strcmp (event, "control-q"))
        {
          do_quit = 1;
        }
#endif
        else if (!strncmp (event, "mouse-", 5))
	{
	  if (!strncmp (event + 6, "motion", 6))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        ctx_vt_mouse (vt, VT_MOUSE_MOTION, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	  else if (!strncmp (event + 6, "press", 5))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        ctx_vt_mouse (vt, VT_MOUSE_PRESS, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	  else if (!strncmp (event + 6, "drag", 4))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        ctx_vt_mouse (vt, VT_MOUSE_DRAG, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	  else if (!strncmp (event + 6, "release", 7))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        ctx_vt_mouse (vt, VT_MOUSE_RELEASE, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	}
        else
        {
          ctx_vt_feed_keystring (vt, event);
          got_event = 1;
        }
        sleep_time = 200;
      }
      if (!got_event)
      {
        audio_task (0);
        usleep (sleep_time);
        sleep_time *= 1.2;

        if (sleep_time > 8000)
          sleep_time = 8000;
        ctx_vt_poll (vt);
      }
      audio_task (got_event);
  }
  ctx_vt_destroy (vt);
  if (mmm)
    mmm_destroy (mmm);
  return 0;
}
