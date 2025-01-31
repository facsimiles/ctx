#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>


#include "../test-size/tiny-config.h"
#undef CTX_PARSER
#define CTX_PARSER          1
#undef CTX_EVENTS
#define CTX_EVENTS 1
#define CTX_TERMINAL_EVENTS 0
#define CTX_IMPLEMENTATION

#include "ctx.h"

int main (int argc, char **argv)
{
  printf ("CTX_MAX_STATES %d\n", CTX_MAX_STATES);
  printf ("CTX_MAX_EDGES %d\n", CTX_MAX_EDGES);

  printf ("sizeof(CtxRasterizer) = %i\n",      (int)sizeof(CtxRasterizer));
  printf ("sizeof(Ctx) = %i\n", (int)sizeof(Ctx));
  printf ("sizeof(CtxParser) = %i\n",            (int)sizeof(CtxParser));
  printf ("\n");
  printf ("sizeof(CtxState) = %i\n",             (int)sizeof(CtxState));
  printf ("sizeof(CtxGState) = %i\n",            (int)sizeof(CtxGState));
  printf ("sizeof(CtxColor) = %i\n",             (int)sizeof(CtxColor));
  printf ("sizeof(CtxFont) = %i\n",              (int)sizeof(CtxFont));
  printf ("sizeof(CtxFontEngine) = %i\n",        (int)sizeof(CtxFontEngine));
  printf ("sizeof(CtxEntry) = %i\n",             (int)sizeof(CtxEntry));
  printf ("sizeof(CtxSource) = %i\n",            (int)sizeof(CtxSource));
  printf ("sizeof(CtxIterator) = %i\n",          (int)sizeof(CtxIterator));
  printf ("sizeof(CtxGradient) = %i\n",          (int)sizeof(CtxGradient));
  printf ("sizeof(CtxBuffer) = %i\n",            (int)sizeof(CtxBuffer));
  printf ("sizeof(CtxDrawlist) = %i\n",          (int)sizeof(CtxDrawlist));
  printf ("sizeof(CtxSegment) = %i\n",          (int)sizeof(CtxSegment));
  printf ("sizeof(CtxPixelFormatInfo) = %i\n", (int)sizeof(CtxPixelFormatInfo));

  return 0;
}
