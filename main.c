#include <string.h>
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
    "  path with a .png suffix, or the string GRAY1 or GRAY8\n"
    "  to generate a terminal visualization on stdout\n"
    "\n"
    "  options:\n"
    "  --color         use color in output\n"
    "  --width  pixels sets width of canvas (default:auto)\n"
    "  --height pixels sets height of canvas (deault:auto)\n"
    "  --rows   rows   configures number of em-rows, when interpreting\n"
    "  --cols   cols   configures number of em-cols, when interpreting\n");
  return 0;
}

//int js_main (int argc, char **argv);
int terminal_main (int argc, char **argv);
int convert_main (int argc, char **argv);

int ctx_img_main (int argc, char **argv);
int ctx_gif_main (int argc, char **argv);
int stuff_main (int argc, char **argv);
int ctx_mpg_main (int argc, char **argv);
int ctx_tcp_main (int argc, char **argv);
int ctx_text_main (int argc, char **argv);

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
    else if (strstr (argv[no], "--x="))
    {
      x = atof (argv[no + strlen ("--x=")]);
    }
    else if (strstr (argv[no], "--y="))
    {
      y = atof (argv[no + strlen ("--y=")]);
    }
    else if (strstr (argv[no], "--width="))
    {
      width = atof (argv[no + strlen ("--width=")]);
    }
    else if (strstr (argv[no], "--height="))
    {
      height = atof (argv[no + strlen ("--height=")]);
    }
  }

  fprintf (stdout, "\e_C");
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
  fprintf (stdout, ";%s\e\\", argv[no]);
  return 0;
}

#include "stb_image.h"
#include "stb_image_write.h"

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
  int was_jpg = strstr(src_path, "jpg")?1:0;
  if (!data)
     return -1;
  if (dim > width && dim > height)
  {
    if (was_jpg)
      stbi_write_jpg (dst_path, width, height, 4, data, stride);
    else
      stbi_write_png (dst_path, width, height, 4, data, stride);
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
  if (was_jpg)
    stbi_write_jpg (dst_path, outw, outh, 4, tdata, outw * 4);
  else
    stbi_write_png (dst_path, outw, outh, 4, tdata, outw * 4);
  free (tdata);
  return 0;
}

char *ctx_thumb_path (const char *path)
{
  char *hex="0123456789abcdefghijkl";
  unsigned char path_hash[40];
  char path_hash_hex[41];
  CtxSHA1 *sha1 = ctx_sha1_new ();
  ctx_sha1_process (sha1, (uint8_t*)"file://", strlen ("file://"));
  ctx_sha1_process (sha1, (uint8_t*)path, strlen (path));
  ctx_sha1_done(sha1, path_hash);
  ctx_sha1_free (sha1);
  for (int j = 0; j < 20; j++)
  {
    path_hash_hex[j*2+0]=hex[path_hash[j]/16];
    path_hash_hex[j*2+1]=hex[path_hash[j]%16];
  }
  path_hash_hex[40]=0;
  return ctx_strdup_printf ("%s/.ctx-thumbnails/%s", getenv ("HOME"), path_hash_hex);
}

int thumb_main (int argc, char **argv)
{
  if (!argv[1])
  {
    fprintf (stderr, "usage: ctx thumb <path1> [path2 [path3 ..]]\n");
    return -1;
  }
  for (int i = 1; argv[i]; i++)
  {
    char *thumb_path = ctx_thumb_path (argv[i]);
    ctx_mkdir_ancestors (thumb_path, 0777);
    fprintf (stderr, "%s", argv[i]);
    make_thumb (argv[i], thumb_path);
    fprintf (stderr, "\n");
    free (thumb_path);
  }

  return 0;
}

static char *output_path = NULL;
static char *commandline = NULL;

static char *input_path = NULL;

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

#include <libgen.h>

int main (int argc, char **argv)
{
  if (!strcmp (basename(argv[0]), "stuff"))
    return stuff_main (argc, argv);
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
    else if (!strcmp (a, "-p"))
    {
       return ctx_tcp_main (argc, argv);
    }
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

  if (argv[1] && !strcmp (argv[1], "thumb"))
    return thumb_main (argc-1, argv+1);
  if (argv[1] && !strcmp (argv[1], "launch"))
    return launch_main (argc-1, argv+1);

  if (input_path)
  {
    if (!strchr (input_path, ':') ||
         ((strchr (input_path, ':') - input_path) > 6))
      {
         char *path = malloc (strlen (input_path) + 10);
         sprintf (path, "file://%s", input_path);
         input_path = path;
      }
    const char *media_type = ctx_path_get_media_type (input_path);
    CtxMediaTypeClass media_type_class = ctx_media_type_class (media_type);

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
    if (!strcmp (media_type, "video/mpeg"))
    {
      return ctx_mpg_main (argc, argv);
    }
    if (input_path[strlen(input_path)-1]=='/')
    {
      return stuff_main (argc, argv);
    }

    if (media_type_class == CTX_MEDIA_TYPE_TEXT)
    {
      return ctx_text_main (argc, argv);
    }

    return -1;
  }

#if 0
  if (argv[1])
  if ((strstr(argv[1], ".js")  && strstr(argv[1], ".js")[3]==0) ||
      (strstr(argv[1], ".html")  && strstr(argv[1], ".html")[5]==0) ||
      (!strncmp (argv[1], "http", 4)))
    return js_main (argc, argv);
#endif

  return terminal_main (argc, argv);
}
