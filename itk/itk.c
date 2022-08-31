#include "ctx.h"

#include "squoze/squoze.h"

#define ITK_IMPLEMENTATION 1
#include "itk.h"   // for completeness, itk wants to be built in the ctx
                   // compilation unit to be influenced by the ctx config
