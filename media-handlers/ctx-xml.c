/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "itk.h"

int interactive = 0;
static float zoom = 1.0f;
static float scroll[2] = {0,0};

void pgup_cb (CtxEvent *e, void *data1, void *data2)
{
  float *pos = data1;
  pos[1] += ctx_height (e->ctx) * 0.8;
  ctx_queue_draw (e->ctx);
}

void pgdn_cb (CtxEvent *e, void *data1, void *data2)
{
  float *pos = data1;
  pos[1] -= ctx_height (e->ctx) * 0.8;
  ctx_queue_draw (e->ctx);
}

void drag_pos (CtxEvent *e, void *data1, void *data2)
{
  if (e->type == CTX_DRAG_MOTION && e->device_no == 1)
  {
    float *pos = data1;
    pos[1] += e->delta_y;
    ctx_queue_draw (e->ctx);
  }
}

typedef struct Mr
{
  ITK  *itk;
  char *uri;
  int   mode;

} Mr;

void
browser_set_uri (Mr *mr, const char *uri)
{
  if (mr->uri)
    free (mr->uri);
  if (uri)
    mr->uri = strdup (uri);
  else
    mr->uri = NULL;

  ctx_queue_draw (itk_ctx(mr->itk));
}

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>

static void toggle_fullscreen_cb (CtxEvent *event, void *data1, void *data2)
{
  ctx_set_fullscreen (event->ctx, !ctx_get_fullscreen (event->ctx));
}
static void exit_cb (CtxEvent *event, void *data1, void *data2)
{
  ctx_exit (event->ctx);
}

static void zoom_in_cb (CtxEvent *event, void *data, void *data2)
{
  zoom *= 1.2f;
  ctx_queue_draw (event->ctx);
}
static void zoom_out_cb (CtxEvent *event, void *data, void *data2)
{
  zoom /= 1.2f;
  ctx_queue_draw (event->ctx);
}

static void href_cb (CtxEvent *event, void *src, void *data2)
{
  Mr *mr = data2;
  browser_set_uri (mr, src);
  scroll[0] = scroll[1] = 0;
}

static const char *magic_mime (const char *data, int length)
{
  unsigned char jpgsig[4]={0xff, 0xd8, 0xff, 0xe0};
  char tmpbuf[256+1];
  if (length>256)
    length = 256;
  if (!data)
    return "text/plain";
  if (length<=0)
    return "text/plain";
  memcpy (tmpbuf, data, length);
  tmpbuf[length]=0;
  if (!memcmp(tmpbuf, "\211PNG\r\n\032\n", 8))
    return "image/png";
  else if (!memcmp(tmpbuf, jpgsig, 4))
    return "image/jpeg";
  else if (strstr(tmpbuf, "html"))
    return "text/html";
  else if (strstr(tmpbuf, "svg"))
    return "text/svg";
  else if (strstr(tmpbuf, "SVG"))
    return "text/svg";
  else if (strstr(tmpbuf, "TITLE"))
    return "text/html";
  else if (strstr(tmpbuf, "HTML"))
    return "text/html";
  else if (strstr(tmpbuf, "xml"))
    return "text/xml";
  return "text/plain";
}


static int render_ui (ITK *itk, void *data)
{
  Mr *mr = data;
  Ctx *ctx = itk_ctx (itk);
  char *contents = NULL;
  long  length;

  if (interactive)
  {
    ctx_save (ctx);
    ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
    ctx_listen (ctx, CTX_DRAG, drag_pos, scroll, NULL);
    ctx_gray (ctx, 0.5f);
    ctx_paint (ctx);
    ctx_restore (ctx);
  }

//#if MRG_CAIRO
  ctx_save (ctx);
  ctx_translate (ctx, scroll[0], scroll[1]);
  ctx_scale (ctx, zoom, zoom);
//#endif
  ctx_get_contents (mr->uri, (uint8_t**)&contents, &length);

  if (contents)
  {
    const char *mime_type = magic_mime (contents, length);
    
    if (!strcmp (mime_type, "text/plain"))
    {
      itk_printf (itk, "%s", contents);
    }
    else if (!strcmp (mime_type, "text/html") ||
             !strcmp (mime_type, "text/xml") ||
             !strcmp (mime_type, "text/svg"))
    {
      //itk_stylesheet_clear (mrg);
      itk_xml_render (itk, mr->uri, href_cb, mr, NULL, NULL, contents);
    }
    else
    {
      itk_printf (itk, "\nUnhandled mimetype\n\n[%s]", mime_type);
    }
    free (contents);
  }

  ctx_restore (ctx);

  ctx_add_key_binding (ctx, "page-down", NULL, NULL, pgdn_cb, scroll);
  ctx_add_key_binding (ctx, "page-up",   NULL, NULL, pgup_cb, scroll);
  ctx_add_key_binding (ctx, "+",   NULL, NULL, zoom_in_cb, NULL);
  ctx_add_key_binding (ctx, "=",   NULL, NULL, zoom_in_cb, NULL);
  ctx_add_key_binding (ctx, "-",   NULL, NULL, zoom_out_cb, NULL);
  ctx_add_key_binding (ctx, "control-q", NULL, NULL, exit_cb, NULL);
  ctx_add_key_binding (ctx, "f", NULL, NULL, toggle_fullscreen_cb, NULL);

  return 1;
}


int browser_main (int argc, char **argv)
{
  Ctx *ctx = NULL;
  Mr *mr;
  
  mr = calloc (sizeof (Mr), 1);

  for (int i = 0; argv[i]; i++)
  {
    if (!strcmp (argv[i], "-i"))
	interactive = 1;
    else
    {
    char *tmp = realpath (argv[i], NULL);
    if (tmp)
    {
      char *uri = malloc (strlen (tmp) + 10);
      sprintf (uri, "file://%s", tmp);
      if (mr->uri) free (mr->uri);
      mr->uri = uri;
      free (tmp);
    }
    }
  }
  if (interactive)
    ctx = ctx_new (-1, -1, NULL);
  else
    ctx = ctx_new_drawlist (640, 480);
  ITK *itk = itk_new (ctx);
  mr->itk = itk;

  ctx_stylesheet_add (itk, "document { background: #ffff;}", NULL, 25, NULL);
//  ctx_stylesheet_add (itk, "svg { background: none;}", NULL, 25, NULL);

  if (interactive)
    itk_run_ui (itk, render_ui, mr);
  else
  {
    render_ui (itk, mr);
    fprintf (stdout, "\n\e[?200h");
    ctx_render_stream (ctx, stdout, 0);
    fprintf (stdout, " done\n\n\n");
  }

  itk_free (itk);


  ctx_destroy (ctx);

  free (mr->uri);
  free (mr);
  return 0;
}
