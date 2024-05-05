#include <string.h>
#include "local.conf"
#include "ctx.h"
#include <sys/stat.h>
#include <sys/types.h>

static int usage_main (int argc, char **argv)
{
  printf (
    "Usage: ctx [-e \"command-line with args\"]\n"
    "  launch a terminal, if no command is specified a new instance of\n"
    "  the users shell is invoked.\n"
    "\n"
    "or: ctx [options] <source.ctx> -o <destination>\n"
    "  convert source.ctx to destination, where destination is a\n"
    "  path with a .png, .pdf, .ctx or .ctxc suffix or one of the\n"
    "  strings GRAY1, GRAY2, GRAY4, GRAY8, RGBA8, RGB332 or RGB565 to generate\n"
    "  a terminal visualization of the corresponding pixel encoding.\n"
    "\n"
    "  options:\n"
    "  --width  pixels sets width of canvas (default:auto)\n"
    "  --height pixels sets height of canvas (deault:auto)\n"
    "  --rows   rows   configures number of em-rows, when interpreting\n"
    "  --cols   cols   configures number of em-cols, when interpreting\n");
  return 0;
}

//int js_main (int argc, char **argv);
int terminal_main (int argc, char **argv);
int convert_main (int argc, char **argv);

int ctx_tinyvg_main (int argc, char **argv);
int ctx_img_main (int argc, char **argv);
int ctx_gif_main (int argc, char **argv);
int browser_main (int argc, char **argv);
int ctx_mpg_main (int argc, char **argv);
int ctx_hexview_main (int argc, char **argv);

int launch_main (int argc, char **argv)
{
  // check that we have a term
  // and that we can launch
  int   can_launch = 0;
  int   no_title = 0;
  int   no_move = 0;
  int   no_resize = 0;
 // int   lock_relative_size = 0; // will make a fullscreen continue being so,
 //                               // and panels or other things git 
  int   layer = 0;
  // escape subsequent arguments so that we dont have to pass a string?
  int   no = 1;
  float x = -1.0;
  float y = -1.0;
  float width = -1.0;
  float height = -1.0;
  for (no = 1; argv[no] && argv[no][0]=='-'; no++)
  {
    if (!strcmp (argv[no], "--no-title"))
    {
      no_title = 1;
    }
    if (!strcmp (argv[no], "--no-move"))
    {
      no_move = 1;
    }
    if (!strcmp (argv[no], "--no-resize"))
    {
      no_resize = 1;
    }
    if (!strcmp (argv[no], "--can-launch"))
    {
      can_launch = 1;
    }
    if (!strcmp (argv[no], "--can-launch=1"))
    {
      can_launch = 1;
    }
    else if (!strcmp (argv[no], "-z=0"))
    {
      layer = 0;
    }
    else if (!strcmp (argv[no], "-z=-1"))
    {
      layer = -1;
    }
    else if (!strcmp (argv[no], "-z=1"))
    {
      layer = 1;
    }
    else if (!strcmp (argv[no], "--layer=background"))
    {
      layer = -1;
    }
    else if (!strcmp (argv[no], "--layer=foreground"))
    {
      layer = 1;
    }
    else if (ctx_strstr (argv[no], "--x="))
    {
      x = atof (argv[no + strlen ("--x=")]);
    }
    else if (ctx_strstr (argv[no], "--y="))
    {
      y = atof (argv[no + strlen ("--y=")]);
    }
    else if (ctx_strstr (argv[no], "--width="))
    {
      width = atof (argv[no + strlen ("--width=")]);
    }
    else if (ctx_strstr (argv[no], "--height="))
    {
      height = atof (argv[no + strlen ("--height=")]);
    }
  }

  fprintf (stdout, "\033_C");
  if (layer == -1) fprintf (stdout, "z=-1,");
  if (layer ==  1) fprintf (stdout, "z=1,");
  if (can_launch)  fprintf (stdout, "can_launch=1,");
  if (x>0)         fprintf (stdout, "x=%.0f,", x);
  if (y>0)         fprintf (stdout, "y=%.0f,", y);
  if (width>0)     fprintf (stdout, "width=%.0f,", x);
  if (height>0)    fprintf (stdout, "height=%.0f,", y);
  if (no_title)    fprintf (stdout, "no_title=1,");
  if (no_move)     fprintf (stdout, "no_move=1,");
  if (no_resize)   fprintf (stdout, "no_resize=1,");
  fprintf (stdout, ";%s\033\\", argv[no]);
  return 0;
}

#if CTX_IMAGE_WRITE

#include "stb_image.h"

typedef struct _CtxSHA1 CtxSHA1;
CtxSHA1 *ctx_sha1_new (void);
void ctx_sha1_free (CtxSHA1 *sha1);
int ctx_sha1_process(CtxSHA1 *sha1, const unsigned char * msg, unsigned long len);
int ctx_sha1_done(CtxSHA1 * sha1, unsigned char *out);

void ctx_mkdir_ancestors (const char *path, unsigned int mode)
{
  char *tmppaths=strdup (path);
  char *sl = strchr (tmppaths, '/');
  while (sl && *sl)
  {
    sl ++;
    sl = strchr (sl, '/');
    if (sl)
    {
      *sl = '\0';
      mkdir (tmppaths, mode);
      *sl = '/';
    }
  }
  free (tmppaths);
}

int make_thumb (const char *src_path, const char *dst_path)
{
  /* XXX  does nearest neighbor which is horrid for thumbs  */
  int width, height, components, stride;
  uint8_t *data = stbi_load (src_path, &width, &height, &components, 4);
  int idim = 256; // largest width or height
  stride = width * 4;
  float dim = idim;
  //int was_jpg = ctx_strstr(src_path, "jpg")?1:0;
  if (!data)
     return -1;
  if (dim > width && dim > height)
  {
    //if (was_jpg)
    //  stbi_write_jpg (dst_path, width, height, 4, data, stride);
    //else
      _ctx_write_png (dst_path, width, height, 4, data);
    return 0;
  }
  uint8_t *tdata = calloc (idim * idim, 4);

  float factor = width / dim;
  if (height / dim >  factor) factor = height / dim;

  int outw = width / factor;
  int outh = height / factor;

  /* this is the crudest thumbnailer that almost works :]
   * missing any color management or nicer than nearest
   * neighbor.
   */

  int i = 0;
  for (int y = 0; y < outh; y ++)
  for (int x = 0; x < outw; x ++, i+=4)
  {
    int u = x * factor;
    int v = y * factor;
    if (u < 0 ||  v < 0 || u >= width ||
        v >= height)
    {
      // leave pixel blank - doesn't happen now that we keep aspect
    }
    else
    for (int c = 0; c < 4; c ++)
    {
      tdata[i+c] = data[v * stride + u * 4 + c];
    }
  }
  //if (was_jpg)
  //  stbi_write_jpg (dst_path, outw, outh, 4, tdata, outw * 4);
  //else
    _ctx_write_png (dst_path, outw, outh, 4, tdata);
  free (tdata);
  return 0;
}

#endif

static char *output_path = NULL;
static char *commandline = NULL;

static char *input_path = NULL;

#if 0
static const char *get_suffix (const char *path)
{
  if (!path)
    { return ""; }
  const char *p = strrchr (path, '.');
  if (p && *p)
    { return p; }
  else
    {
      p = strrchr (path, '/');
      if (p && p[0] && p[1])
        return p+1;
      return "";
    }
}
#endif

int ctx_path_is_dir (const char *path);

#include <libgen.h>
extern int ctx_media_matched_content;
int file_main (int argc, char **argv)
{
  for (int i = 1; argv[i]; i++)
  {
     fprintf (stdout, "%s:\t", argv[i]);
     fprintf (stdout, "%s",  ctx_path_get_media_type (realpath(argv[i], NULL)));
     fprintf (stdout, " %s\n", ctx_media_matched_content?"*":"");
  }
  return 0;
}

static int lsfonts_main (int argc, char **argv)
{
  Ctx *ctx = ctx_new (0,0,NULL);

  for (int i = 0; i < CTX_MAX_FONTS; i++)
  {
    const char *name = ctx_get_font_name (ctx, i);
    if (name)
      printf ("%s\n", name);
  }
  return 0;
}

int main (int argc, char **argv)
{
  if (!strcmp (basename(argv[1]), "lsfonts"))
    return lsfonts_main (argc-1, argv+1);
  if (!strcmp (basename(argv[1]), "hexview"))
    return ctx_hexview_main (argc-1, argv+1);
  for (int i = 1; argv[i]; i++)
  {
    char *a = argv[i];
    if (!strcmp ( a, "--help"))
      return usage_main (argc, argv);
    else if (!strcmp (a, "-o"))
    {
      output_path = argv[i+1];
      i++;
    }
    //else if (!strcmp (a, "-p"))
    //{
    //   return ctx_tcp_main (argc, argv);
    //}
    else if (!strcmp (a, "-e"))
    {
      commandline = argv[i+1];
      i++;
    } else {
      if (!input_path && argv[i][0] != '-')
        input_path = argv[i];
    }
  }

  if (output_path)
    return convert_main (argc, argv);

  if (argv[1] && !strcmp (argv[1], "file"))
    return file_main (argc-1, argv+1);
  if (argv[1] && !strcmp (argv[1], "launch"))
    return launch_main (argc-1, argv+1);

  if (input_path && !commandline)
  {
    if (strchr (input_path, ':'))
    {
      input_path = strchr (input_path, ':') + 1;
      if (input_path[0] == '/') input_path++;
      if (input_path[0] == '/') input_path++;
    }

    input_path = realpath (input_path, NULL);

    const char *media_type = input_path?ctx_path_get_media_type (input_path):
	    "file/octet-stream";
    CtxMediaTypeClass media_type_class = ctx_media_type_class (media_type);

    if (media_type_class){};

#if CTX_TINYVG
    if (!strcmp (media_type, "image/tinyvg"))
    {
      return ctx_tinyvg_main (argc, argv);
    }
#endif
#if CTX_STB_IMAGE
    if (!strcmp (media_type, "image/gif"))
    {
      return ctx_gif_main (argc, argv);
    }
    if (!strcmp (media_type, "image/jpeg") ||
        !strcmp (media_type, "image/exr") ||
        !strcmp (media_type, "image/png"))
    {
      return ctx_img_main (argc, argv);
    }
#endif
#if CTX_PL_MPEG
    if (!strcmp (media_type, "video/mpeg"))
    {
      return ctx_mpg_main (argc, argv);
    }
#endif
    if (   !strcmp (media_type, "image/svg+xml") 
        || !strcmp (media_type, "text/html"))
    {
      return browser_main (argc, argv);
    }

    return -1;
  }

#if 0
  if (argv[1])
  if ((ctx_strstr(argv[1], ".js")  && ctx_strstr(argv[1], ".js")[3]==0) ||
      (ctx_strstr(argv[1], ".html")  && ctx_strstr(argv[1], ".html")[5]==0) ||
      (!strncmp (argv[1], "http", 4)))
    return js_main (argc, argv);
#endif

#if CTX_VT
  return terminal_main (argc, argv);
#else
  fprintf (stderr, "ctx built without terminal support\n");
  return 0;
#endif
}
