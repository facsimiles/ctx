#include <stdlib.h>
#include <libgen.h>

#define CTX_MAX_DRAWLIST_SIZE 4096000
#define CTX_BACKEND_TEXT 0 // we keep then non-backend code paths
                           // for code handling aroud, this should
                           // be run-time to permit doing text_to_path
#define CTX_RASTERIZER         0

#define CTX_BITPACK_PACKER     1  // pack vectors
#define CTX_BITPACK            1
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <sys/time.h>
#define CTX_EXTRAS 1
#define CTX_AVX2 0
#define CTX_IMPLEMENTATION
#define CTX_EVENTS 1
#define CTX_FONTS_FROM_FILE 1
#define CTX_PARSER 1
#include "ctx-nofont.h"
#include <sys/stat.h>
#include <sys/types.h>


CtxDrawlist font={NULL,};

static int find_glyph (CtxDrawlist *drawlist, int unichar)
{
  for (int i = 0; i < drawlist->count; i++)
  {
    if (drawlist->entries[i].code == CTX_DEFINE_GLYPH &&
        drawlist->entries[i].data.u32[0] == unichar)
    {
       return i;
       // XXX this could be prone to insertion of valid header
       // data in included bitmaps.. is that an issue?
    }
  }
  fprintf (stderr, "Eeeek %i\n", unichar);
  return -1;
}

int usage (int argc, char **argv)
{
  fprintf (stderr, "usage: %s <font.ctxf> <output/base/path/>\n", basename (argv[0]));
  return 0;
}

void mkdir_ancestors (const char *path)
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
      mkdir (tmppaths, 0777);
      fprintf (stderr, "[%s]", tmppaths);
      *sl = '/';
    }
  }
  free (tmppaths);
}

int main (int argc, char **argv)
{
  CtxFormatter formatter;
  formatter.longform = 1;
  formatter.indent = 0;
  formatter.add_str = _ctx_stream_addstr;

  int binary       = 0;
  char *path       = NULL;
  char *output_dir = NULL;

  Ctx *ctx;
  path = argv[1];
  if (path)
    output_dir = argv[2];
  if (!path || !output_dir)
  {
    return usage(argc, argv);
  }

  int length = 0;
  ctx_get_contents (path, &font.entries, &length);
  if (length % 9)
  {
    fprintf (stderr, "file size not a multiple of 9\n");
    return 1;
  }

  if (font.entries)
    font.count = length / 9;

  if (output_dir[strlen(output_dir)-1]=='/')
    output_dir[strlen(output_dir)-1]=0;

  fprintf (stderr, "length: %i\n", length);
  mkdir_ancestors (output_dir);

  FILE *output_file = NULL;
  for (int i = 0; i < font.count; i++)
  {
     if (font.entries[i].code == CTX_DEFINE_GLYPH)
     {
       char utf8[10];
       utf8[ctx_unichar_to_utf8 (font.entries[i].data.u32[0], utf8)]=0;
       if (output_file)
         fclose (output_file);
       char output_path[100];
       sprintf (output_path, "%s/%010x", output_dir, font.entries[i].data.u32[0]);
       fprintf (stderr, "%s\n", output_path);
       output_file = fopen (output_path, "w");
       formatter.target = output_file;

       CtxIterator iterator;
       CtxEntry *entry;
       ctx_iterator_init (&iterator, &font, i, CTX_ITERATOR_EXPAND_BITPACK);
       entry = ctx_iterator_next (&iterator);
       fprintf (stderr, "%c %u \n", entry->code, entry->data.u32[0]);
       ctx_formatter_process (&formatter, entry);
       while (entry = ctx_iterator_next (&iterator))
       {
         if (entry->code == CTX_DEFINE_GLYPH)
           break;
         ctx_formatter_process (&formatter, entry);
       }
     }
     else
     {
     }
  }


  fprintf (stderr, "%s %s\n", argv[1], argv[2]);

  return 0;
}
