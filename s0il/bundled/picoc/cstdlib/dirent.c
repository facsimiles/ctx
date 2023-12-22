/*  */
#include <stdlib.h>

#include "../interpreter.h"
#include <unistd.h>
//#include <dirent.h>
#if EMSCRIPTEN
#define S0IL_REDEFINE_CLIB
#endif
#include "s0il.h"


void DirentOpendir(struct ParseState *Parser, struct PcValue *ReturnValue,
    struct PcValue **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = opendir(Param[0]->Val->Pointer);
}

void DirentReaddir(struct ParseState *Parser, struct PcValue *ReturnValue,
    struct PcValue **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = readdir(Param[0]->Val->Pointer);
}

void DirentClosedir(struct ParseState *Parser, struct PcValue *ReturnValue,
    struct PcValue **Param, int NumArgs)
{
    ReturnValue->Val->Integer = closedir(Param[0]->Val->Pointer);
}

/* handy structure definitions */

#if EMSCRIPTEN
 // same as native
const char DirentDefs[] = 
  "typedef struct _DIR DIR;\n"
  "struct dirent {"
  "int d_ino;"
  "int d_ino_pad;"
  "int d_off;"
  "int d_off_pad;"
  "unsigned char d_reclen[2];"
  "unsigned char d_type; "
  "char d_name[256];};";
#else

#if NATIVE
const char DirentDefs[] = 
  "typedef struct _DIR DIR;\n"
  "struct dirent {"
  "int d_ino;"
  "int d_ino_pad;"
  "int d_off;"
  "int d_off_pad;"
  "unsigned char d_reclen[2];"
  "unsigned char d_type; "
  "char d_name[256];};";
#else
const char DirentDefs[] = 
  "typedef struct _DIR DIR;\n"
  "struct dirent {"
  //"int d_ino_off;"
  //"int d_off;"
  "unsigned char d_reclen[2];"
  "unsigned char d_type; "
  "char d_name[256];};";
#endif

#endif

/* all stdlib.h functions */
const struct LibraryFunction DirentFunctions[] =
{
    {DirentOpendir, "DIR*opendir(char*);"},
    {DirentReaddir, "struct dirent*readdir(DIR*);"},
    {DirentClosedir, "int closedir(DIR*);"},
    {NULL, NULL}
};

/* creates various system-dependent definitions */
void DirentSetupFunc(Picoc *pc)
{
}

