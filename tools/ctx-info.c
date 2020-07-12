#include <stdlib.h>
#include <libgen.h>

#define CTX_MAX_RENDERSTREAM_SIZE 4096000
#define CTX_BACKEND_TEXT 0
#define CTX_AVX2 0
#define CTX_PARSER       1
#define CTX_GLYPH_CACHE  1
#define CTX_ENABLE_CMYK  0
#define CTX_ENABLE_CM    0
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

  printf ("sizeof(CtxRasterizer) = %li\n",      sizeof(CtxRasterizer));
  printf ("sizeof(Ctx) = %li\n", sizeof(Ctx));
  printf ("sizeof(CtxParser) = %li\n",            sizeof(CtxParser));
  printf ("\n");
  printf ("sizeof(CtxState) = %li\n",             sizeof(CtxState));
  printf ("sizeof(CtxGState) = %li\n",            sizeof(CtxGState));
  printf ("sizeof(CtxColor) = %li\n",             sizeof(CtxColor));
  printf ("sizeof(CtxFont) = %li\n",              sizeof(CtxFont));
  printf ("sizeof(CtxFontEngine) = %li\n",        sizeof(CtxFontEngine));
  printf ("sizeof(CtxEntry) = %li\n",             sizeof(CtxEntry));
  printf ("sizeof(CtxSource) = %li\n",            sizeof(CtxSource));
  printf ("sizeof(CtxIterator) = %li\n",          sizeof(CtxIterator));
  printf ("sizeof(CtxGradient) = %li\n",          sizeof(CtxGradient));
  printf ("sizeof(CtxBuffer) = %li\n",            sizeof(CtxBuffer));
  printf ("sizeof(CtxRenderstream) = %li\n",      sizeof(CtxRenderstream));
  printf ("sizeof(CtxPixelFormatInfo) = %li\n", sizeof(CtxPixelFormatInfo));
  printf ("sizeof(CtxEdge) = %li\n", sizeof(CtxEdge));

  return 0;
}
