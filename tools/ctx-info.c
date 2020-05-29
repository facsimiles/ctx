#include <stdlib.h>
#include <libgen.h>

#define CTX_MAX_JOURNAL_SIZE 4096000
#define CTX_BACKEND_TEXT 0
#define CTX_PARSER       1
#define CTX_GLYPH_CACHE 0
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <sys/time.h>
#define CTX_EXTRAS 1
#define CTX_IMPLEMENTATION
#include "ctx.h"

CtxRenderstream output_font={NULL,};
uint32_t glyphs[65536];
int n_glyphs = 0;

int main (int argc, char **argv)
{
  printf ("CTX_MAX_STATES %d\n", CTX_MAX_STATES);
  printf ("CTX_MAX_EDGES %d\n", CTX_MAX_EDGES);

  printf ("sizeof(Ctx) = %li\n", sizeof(Ctx));
  printf ("sizeof(CtxState) = %li\n", sizeof(CtxState));
  printf ("sizeof(CtxGState) = %li\n", sizeof(CtxGState));
  printf ("sizeof(CtxEntry) = %li\n", sizeof(CtxEntry));
  printf ("sizeof(CtxSource) = %li\n", sizeof(CtxSource));
  printf ("sizeof(CtxParser) = %li\n", sizeof(CtxParser));
  printf ("sizeof(CtxRenderer) = %li\n", sizeof(CtxRenderer));
  printf ("sizeof(CtxEdge) = %li\n", sizeof(CtxEdge));

  return 0;
}
