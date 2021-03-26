/* This is a declaration for building only AVX2 compositor support, with a
 * generated pixel_format db array suffixed with _avx2 compositors with all
 * pixel formats get compiled with avx2 support including the crucial
 * conditional intrinsics based RGBA8 fast path.
 */

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#define CTX_GRADIENT_CACHE       0
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_MATH                 1
#include <immintrin.h> // is detected by ctx, and enables AVX2

#define CTX_COMPOSITE 1
#define CTX_COMPOSITE_SUFFIX(a)  a##_avx2
#include "ctx.h"
