#if CTX_STB_TT
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#if CTX_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#if CTX_IMAGE_WRITE
#else
#define MINIZ_NO_ARCHIVE_APIS  1
#define MINIZ_NO_DEFLATE_APIS  1
#endif

//#define MINIZ_NO_ARCHIVE_WRITING_APIS 1
#define MINIZ_NO_STDIO                1
#include "miniz.c"
