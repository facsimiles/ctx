#include "local.conf"
#include "ctx.h"

#if CTX_EVENTS
#include "squoze/squoze.h"

#define ITK_IMPLEMENTATION 1
#include "itk.h"   // for completeness, itk wants to be built in the ctx
                   // compilation unit to be influenced by the ctx config
#endif
