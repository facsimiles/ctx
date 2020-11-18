/* This is a declaration for building with extra CFLAGS adding sse2
 */

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#define CTX_GRADIENT_CACHE       1
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_MATH                 1

#define CTX_COMPOSITE 1
#define CTX_COMPOSITE_SUFFIX(a)  a##_sse2
#include "ctx.h"
