#include "port_config.h"
#include "ui.h"
#include <dirent.h>

#define MAX_EXEC_DEPTH  8

typedef struct _exec_state_t exec_state_t;
struct _exec_state_t {
  char *cwd;
  FILE *stdin_copy;
  FILE *stdout_copy;
  FILE *stderr_copy;
};

static exec_state_t exec_state[MAX_EXEC_DEPTH];
static int exec_depth = 0;

#include <unistd.h>

void pre_exec (void)
{
  char tmp[512];
  // TODO : store more info, and implement getpid
  //        most of this info can be shared between a 
  //        synchronous implementation and task/pthread one
  exec_state[exec_depth].cwd=strdup(getcwd (tmp,512)); 
  exec_state[exec_depth].stdin_copy=stdin;
  exec_state[exec_depth].stdout_copy=stdout;
  exec_state[exec_depth].stderr_copy=stderr;
  exec_depth++;
}
void post_exec (void)
{
  exec_depth--;
  chdir(exec_state[exec_depth].cwd);
  stdin=exec_state[exec_depth].stdin_copy;
  stdout=exec_state[exec_depth].stdout_copy;
  stderr=exec_state[exec_depth].stderr_copy;
  free (exec_state[exec_depth].cwd);
}

typedef struct inlined_program_t {
  char *base;
  char *path;
  int(*main)(int argc, char **argv);
} inlined_program_t;

static CtxList *inlined_programs = NULL;

void run_inline_main (const char *name, int(*main)(int argc, char **argv))
{
  inlined_program_t *program = calloc (sizeof (inlined_program_t), 1);
  program->base = strdup (name);
  program->path = malloc (strlen (name) + 10);
  program->main = main;
  sprintf (program->path, ":%s", name);
  ctx_list_append (&inlined_programs, program);
}
#include <libgen.h>


static void busywarp_list (void)
{
  for (CtxList *iter = inlined_programs; iter; iter = iter->next)
  {
    inlined_program_t *program = iter->data;
    printf ("%s ", program->base);
  }
}

char *ui_basename (const char *in)
{
  if (strchr (in, '/'))
    return strrchr (in, '/')+1;
  return (char*)in;
}


int busywarp (int argc, char **argv)
{
  char *tmp = strdup (argv[0]);
  char *base = ui_basename (tmp);

  if (!strcmp (base, "busywarp"))
  {
    if (argv[1] == NULL)
    {
    printf ("Usage: %s <command> [args ..]\n  commands: ", base);
    busywarp_list ();
    printf ("\n");
    return 0;
    }
    else
    {
      return busywarp (argc - 1, &argv[1]);
    }
  }

  for (CtxList *iter = inlined_programs; iter; iter = iter->next)
  {
    inlined_program_t *program = iter->data;
    if (!strcmp (base, program->base))
    {
       int ret = program->main (argc, argv);
       free (tmp);
       return ret;
    }
  }
  printf ("no matching internal %s\nAvailable commands: ", base);
  busywarp_list ();
  printf ("\n");
  free (tmp);
  return -1;
}

#if CTX_FLOW3R

#include "esp_log.h"
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_elf.h"

typedef struct _elf_handle_t elf_handle_t;

struct _elf_handle_t {
     esp_elf_t elf;
     char *path;
};

static CtxList *elf_handles = NULL;

void elf_close (elf_handle_t *elf)
{
  esp_elf_deinit (&elf->elf);
  if (elf->path)
    free (elf->path);
  free (elf);
  ctx_list_remove (&elf_handles, elf);
}

elf_handle_t *elf_open (const char *path)
{
  if (!strstr(path, "picoc"))
  for (CtxList *iter = elf_handles; iter; iter=iter->next)
  {
    elf_handle_t *elf = iter->data;
    if (!strcmp (path, elf->path))
      return elf;
  }
  
  FILE *file = run_fopen (path, "rb");
  if (file)
  {
     run_fseek(file, 0, SEEK_END);
     int length = run_ftell(file);
     run_fseek(file, 0, SEEK_SET);
     elf_handle_t *elf = calloc (sizeof (elf_handle_t), 1);
     elf->path = strdup (path);
     if (!elf) return NULL;
     uint8_t *data = malloc(length + 1);
     if (data)
     {
       run_fread(data, length, 1, file);
       run_fclose(file);
       esp_elf_init(&elf->elf);
       if (esp_elf_relocate(&elf->elf, data))
       {
           free (data);
           return NULL;
       }
       memset(data, 0, length);
       free (data);
     }
     ctx_list_append (&elf_handles, elf);
     return elf;
   }
   return NULL;
}


static int esp_elf_runv (char *path, char **argv)
{
  elf_handle_t *elf = elf_open (path);
  int retval = -1;
  if (!elf)
    return retval;

  if ((!path) && argv && argv[0])
    path = argv[0];
  char *fake_argv[2] = {path, NULL};
  int argc = 0;
  if (argv)
  for (int i = 0; argv[i]; i++)
    argc++;
  else
  {
    argc = 1;
    argv = fake_argv;
  }
  pre_exec();
  retval = esp_elf_request(&elf->elf, 0 /* request-opt*/, argc, argv);
  post_exec();

  if (retval != 42) // TSR
  {
    elf_close(elf);
  }
  return retval;
}
#else
#include <dlfcn.h>

int output_state = 0;
#if 0
void run_output_state_reset (void)
{
  output_state = 0;
}
int  run_output_state (void){
  return output_state;
}
#endif

static int dlopen_runv (char *path2, char **argv)
{
  char *path = path2;
  int argc = 0;
  if ((!path) && argv && argv[0])
    path = argv[0];
  char *fake_argv[2] = {path, NULL};
  if (argv)
    for (int i = 0; argv[i]; i++)
      argc++;
  else
  {
    argc = 1;
    argv = fake_argv;
  }

  if (!strncmp (path, "/bin/", 5))
  {
    // XXX : we expect this to be an internal dir where dlopen
    //       does not work, thus make copies.
    char *tmp = malloc (strlen(path) + 20);
    sprintf (tmp, "/tmp/_s0il_elf_%s", path);
    for (int i = 8; tmp[i]; i++)
    {
      switch (tmp[i])
      {
        case '/':tmp[i]='_';
      }
    }
    tmp[1]='t';
    tmp[2]='m';
    tmp[3]='p';
    if (access (tmp, R_OK) != F_OK)
    {
    char *cmd = malloc(strlen(tmp)*2+10);
    FILE *in = run_fopen(path, "rb");
    FILE *out = run_fopen(tmp, "w");
     run_fseek(in, 0, SEEK_END);
     int length = run_ftell(in);
     run_fseek(in, 0, SEEK_SET);
     uint8_t *data = malloc(length + 1);
     if (data)
     {
       run_fread(data, length, 1, in);
       run_fwrite(data, length, 1, out);
    }
    run_fclose (in);
    run_fclose (out);
    free (cmd);
    }
    path = tmp;
  }

  void *dlhandle = dlopen (path, RTLD_NOW|RTLD_NODELETE);
  //if (path!= path2)
  //  unlink(path);
  if (dlhandle)
  {
     int (*main)(int argc, char **argv) = dlsym (dlhandle, "main");
     if (main)
     {
        output_state = 3;
        pre_exec();
        int ret = main (argc, argv);
        post_exec();
        if (ret != 42)
          dlclose (dlhandle);
        output_state = 0;
     }
  }
  if (path!= path2)
    free (path);

  return 0;
}
#endif

static int program_runv (inlined_program_t *program, char **argv)
{
  char *path = program->path;
  int argc = 0;
  int ret = -1;
  char *fake_argv[2] = {path, NULL};
  if (argv)
    for (int i = 0; argv[i]; i++)
      argc++;
  else
  {
    argc = 1;
    argv = fake_argv;
  }
  {
//   output_state = 3;
     pre_exec();
     ret = program->main(argc, argv);
     post_exec();
//   output_state = 0;
  }
  return ret;
}

int runv (char *path, char **argv)
{
  //printf (":::%s %s\n", path, argv?argv[0]:NULL);
  for (CtxList *iter = inlined_programs; iter; iter = iter->next)
  {
    inlined_program_t *program = iter->data;
    if (!strcmp (path, program->path))
    {
      int retval =
      program_runv (program, argv);
      return retval;
    }
  }

#if 1
  {
    FILE *f = run_fopen (path, "r");
    if (f)
    {
       uint8_t sector[512];
       run_fread(sector, 512, 1, f);
       run_fclose (f);
       if (sector[0]=='/' && sector[1]=='/' && sector[2]=='!')
       {
         int argc = 0;
         int ilen = (strchr((char*)sector, '\n') - ((char*)&sector[0])) - 3;
         char interpreter[ilen+1];
         memcpy(interpreter, sector+3, ilen);
         interpreter[ilen]=0;

         for (;argv && argv[argc];argc++);
         if (argc == 0)
           argc = 1;
         char *argv_expanded[argc+4];
         argv_expanded[0]=interpreter;
         argv_expanded[1]=path;
         if (strstr(interpreter, "picoc") != NULL)
         {
           argv_expanded[2]="-";
           for (int i = 1; i < argc; i++)
             argv_expanded[i+2] = argv[i];
           argc+=2;
         }
         else
         {
           for (int i = 1; i < argc; i++)
             argv_expanded[i+1] = argv[i];
           argc+=1;
         }
         argv_expanded[argc]=NULL;
         return runvp(interpreter, argv_expanded);
       }
       else if (sector[0]=='#' && sector[1]=='!')
       {
         int argc = 0;
         int ilen = (strchr((char*)sector, '\n') - ((char*)&sector[0])) - 2;
         char interpreter[ilen+1];
         memcpy(interpreter, sector+2, ilen);
         interpreter[ilen]=0;

         for (;argv && argv[argc];argc++);
         if (argc == 0)
           argc = 1;
         char *argv_expanded[argc+4];
         argv_expanded[0]=interpreter;
         argv_expanded[1]=path;
         if (strstr(interpreter, "picoc") != NULL)
         {
           argv_expanded[2]="-";
           for (int i = 1; i < argc; i++)
             argv_expanded[i+2] = argv[i];
           argc+=2;
         }
         else
         {
           for (int i = 1; i < argc; i++)
             argv_expanded[i+1] = argv[i];
           argc+=1;
         }
         argv_expanded[argc]=NULL;
         return runvp(interpreter, argv_expanded);
       }
       else
       if (!(sector[0]==0x7f &&
             sector[1]=='E' &&
             sector[2]=='L' &&
             sector[3]=='F'))
       {
         printf ("wrong magic: %c%c%c\n", sector[0], sector[1], sector[2]);
         return -3;
       }
    }
    else
    {
       return -2;
    }
  }
#endif

#if CTX_FLOW3R
  return esp_elf_runv (path, argv);
#else
  return dlopen_runv (path, argv);
#endif
}

char *ui_find_executable(Ui *ui, const char *file)
{
  char *path[]={"/sd/bin/", "/bin/", ":", NULL};
  char  temp[512];
  
  for (int i = 0; path[i]; i++)
  {
    if (!strcmp (path[i], ":"))
    {
      for (CtxList *iter = inlined_programs; iter; iter = iter->next)
      {
        inlined_program_t *program = iter->data;
        if (!strcmp (program->base, file))
        {
          return strdup(program->path);
        }
      }
    }
    else
    {
      snprintf (temp, sizeof(temp), "%s%s", path[i], file);
      if (run_access (temp, R_OK) == F_OK)
      {
        return strdup(temp);
      }
    }
  }


  return NULL;
}

int
runvp (char *file, char **argv)
{
  char *path = file;
  if (path[0]!='/')
    path = ui_find_executable(NULL, file);
  //printf ("%s %s\n", file, path);
  if (path)
  {
    int retval = runv (path, argv);
    if (path != file)
      free (path);
    return retval;
  }
  return -1;
}

static void *run_thread(void *data)
{
  char **cargv = data;
  int ret = runvp (cargv[0], cargv);
  printf ("got %i as ret in thread\n", ret);
  return (void*)((size_t)ret);
}

  pthread_attr_t attr;
int spawnp (char **argv)
{
  pthread_t tid;
  int argc = 0;
  for (;argv[argc];argc++);
  char **argv_copy = calloc (sizeof(char*)*(argc+1), 1);
  for (int i = 0; i < argc; i++)
     argv_copy[i]=strdup(argv[i]);
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 32*1024);
  if (pthread_create(&tid, &attr, run_thread, argv_copy))
    printf ("failed spawning thread\n");
  return 23;
}


static char *string_chop_head (char *orig) /* return pointer to reset after arg */
{
  int j=0;
  int eat=0; /* number of chars to eat at start */
  
  if(orig)
    {
      int got_more;
      char *o = orig;
      while(o[j] == ' ')
        {j++;eat++;}
      
      if (o[j]=='"')
        {
          eat++;j++;
          while(o[j] != '"' &&
                o[j] != 0)
            j++;
          o[j]='\0';
          j++;
        }
      else if (o[j]=='\'')
        {
          eat++;j++;
          while(o[j] != '\'' &&
                o[j] != 0)
            j++;
          o[j]='\0';
          j++;
        }
      else
        {
          while(o[j] != ' ' &&
                o[j] != 0 &&
                o[j] != ';')
            j++;
        }

      if (o[j] == 0 ||
          o[j] == ';')
        got_more = 0;
      else
        got_more = 1;
      o[j]=0; /* XXX: this is where foo;bar won't work but foo ;bar works*/

      if(eat)
       {
         int k;
         for (k=0; k<j-eat; k++)
           orig[k] = orig[k+eat];
       }
      if (got_more)
        return &orig[j+1];
    }
  return NULL;
}

int
runs (const char *cmdline)
{
  char *cargv[32];
  int   cargc;
  char *rest, *copy;
  int ret = 0;
  if (!cmdline)
    return -4;
  copy = calloc (strlen (cmdline)+3, 1);
  strcpy (copy, cmdline);
  //copy[strlen (cmdline)]=' ';
  //copy[strlen (cmdline)+1]=0;
  rest = copy;

  do {
    cargc = 0;
    while (rest && cargc < 30 && (rest[0] != ';' && rest[0] != '&' && rest[0]!='>' && rest[0] != '|') )
      {
        cargv[cargc++] = rest;
        rest = string_chop_head (rest);
      }
    cargv[cargc] = NULL;
//  printf (":::%i %s\n", cargc, cargv[0]);
    if (cargv[0])
    {
      if (rest && rest[0]=='|')
      {
         printf ("pipes NYI\n");
      }
      else if (rest && rest[0]=='>')
      {
         printf ("redirect NYI\n");
      }
      else if (rest && rest[0]=='&')
      {
         int pid = spawnp (cargv);
         if (pid <= 0) printf ("spawn failed\n");
         else {
           printf("got pid:%i\n", pid);
           ret = pid;
         }
      }
      else
      ret = runvp (cargv[0], cargv);
    }

    if (rest && (rest[0]==';' || rest[0]=='&'))
      {
        rest++;
        while (rest[0] == ' ')rest++;
      }
  } while (rest && rest[0]);
  //done:
  free (copy);
  return ret;
}

