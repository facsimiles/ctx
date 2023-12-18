#include "port_config.h"
#include "s0il.h"
Ctx *ctx_host(void);

#define MAX_EXEC_DEPTH  8

typedef struct _exec_state_t exec_state_t;
struct _exec_state_t {
  char *cwd;
  FILE *stdin_copy;
  FILE *stdout_copy;
  FILE *stderr_copy;
};

typedef struct pidinfo_t {
  int   ppid;
  int   pid;
  char *program;
  char *cwd;
  FILE *std_in; // < cannot use real name - due to #defines
  FILE *std_out;
  FILE *std_err;
  
} pidinfo_t;

static int peak_pid = 0;

static CtxList *proc = NULL;
#if 0
static exec_state_t exec_state[MAX_EXEC_DEPTH];
static int exec_depth = 0;
#endif

#include <unistd.h>

int output_state = 0;
int pre_exec (int same_stack)
{
  pidinfo_t *info = calloc (1, sizeof (pidinfo_t));
  info->pid = ++peak_pid;
  ctx_list_append (&proc, info);
  return info->pid;
#if 0
  char tmp[512];
  if (same_stack)
  {
  // TODO : store more info, and implement getpid
  //        most of this info can be shared between a 
  //        synchronous implementation and task/pthread one
  exec_state[exec_depth].cwd=strdup(getcwd (tmp,512)); 
#ifndef WASM
  exec_state[exec_depth].stdin_copy=stdin;
  exec_state[exec_depth].stdout_copy=stdout;
  exec_state[exec_depth].stderr_copy=stderr;
#endif
  exec_depth++;
  }
#endif
}

void post_exec (int pid, int same_stack)
{
        output_state = 0;
#if 0
  if (same_stack)
  {
  exec_depth--;
  chdir(exec_state[exec_depth].cwd);
#ifndef WASM
  stdin=exec_state[exec_depth].stdin_copy;
  stdout=exec_state[exec_depth].stdout_copy;
  stderr=exec_state[exec_depth].stderr_copy;
#endif
  free (exec_state[exec_depth].cwd);
  }
#endif
}

typedef struct inlined_program_t {
  char *base;
  char *path;
  int(*main)(int argc, char **argv);
} inlined_program_t;

static CtxList *inlined_programs = NULL;

void s0il_bundle_main (const char *name, int(*main)(int argc, char **argv))
{
  inlined_program_t *program = calloc (sizeof (inlined_program_t), 1);
  program->base = strdup (name);
  program->path = malloc (strlen (name) + 10);
  program->main = main;
  sprintf (program->path, ":%s", name);
  ctx_list_append (&inlined_programs, program);
  printf ("bundled %s\n", name);
}
#include <libgen.h>


static void busywarp_list (void)
{
  for (CtxList *iter = inlined_programs; iter; iter = iter->next)
  {
    inlined_program_t *program = iter->data;
    s0il_printf ("%s ", program->base);
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
      s0il_printf ("Usage: %s <command> [args ..]\n  commands: ", base);
      busywarp_list ();
      s0il_printf ("\n");
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

#ifndef WASM
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
  esp_elf_t  elf;
  char      *path;
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
  
  FILE *file = s0il_fopen (path, "rb");
  if (file)
  {
     s0il_fseek(file, 0, SEEK_END);
     int length = s0il_ftell(file);
     s0il_fseek(file, 0, SEEK_SET);
     elf_handle_t *elf = calloc (sizeof (elf_handle_t), 1);
     elf->path = strdup (path);
     if (!elf) return NULL;
     uint8_t *data = malloc(length + 1);
     if (data)
     {
       s0il_fread(data, length, 1, file);
       s0il_fclose(file);
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

static int esp_elf_runv (char *path, char **argv, int same_stack)
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
  int pid = pre_exec(same_stack);
  retval = esp_elf_request(&elf->elf, 0 /* request-opt*/, argc, argv);
  post_exec(pid, same_stack);

  if (retval != 42) // TSR
  {
    elf_close(elf);
  }
  return retval;
}
#elif NATIVE
#include <dlfcn.h>

#if 0
void s0il_output_state_reset (void)
{
  output_state = 0;
}
int  s0il_output_state (void){
  return output_state;
}
#endif

static int dlopen_runv (char *path2, char **argv, int same_stack)
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
      FILE *in = s0il_fopen(path, "rb");
      FILE *out = s0il_fopen(tmp, "w");
      s0il_fseek(in, 0, SEEK_END);
      int length = s0il_ftell(in);
      s0il_fseek(in, 0, SEEK_SET);
      uint8_t *data = malloc(length + 1);
      if (data)
      {
        s0il_fread(data, length, 1, in);
        s0il_fwrite(data, length, 1, out);
      }
      s0il_fclose (in);
      s0il_fclose (out);
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
        int pid = pre_exec(same_stack);
        int ret = main (argc, argv);
        post_exec(pid, same_stack);
        if (ret != 42)
          dlclose (dlhandle);
     }
  }
  if (path!= path2)
    free (path);

  return 0;
}
#else

 /// 

#endif
#endif

static int program_runv (inlined_program_t *program, char **argv, int same_stack)
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
     int pid = pre_exec(same_stack);
     ret = program->main(argc, argv);
     post_exec(pid, same_stack);
  }
  return ret;
}

int s0il_runv (char *path, char **argv)
{
  //printf (":::%s %s\n", path, argv?argv[0]:NULL);
  for (CtxList *iter = inlined_programs; iter; iter = iter->next)
  {
    inlined_program_t *program = iter->data;
    if (!strcmp (path, program->path))
    {
      int retval =
      program_runv (program, argv, 1);
      ctx_reset_has_exited (ctx_host());
      return retval;
    }
  }

#if 1
  {
    FILE *f = s0il_fopen (path, "r");
    if (f)
    {
       uint8_t sector[512];
       s0il_fread(sector, 512, 1, f);
       s0il_fclose (f);
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
         return s0il_runvp(interpreter, argv_expanded);
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
         return s0il_runvp(interpreter, argv_expanded);
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

 int ret = 0;
#ifndef WASM
#if CTX_FLOW3R
  ret = esp_elf_runv (path, argv, 1);
#elif NATIVE
  ret = dlopen_runv (path, argv, 1);
#endif
#else
  ret = -4;
#endif
  ctx_reset_has_exited (ctx_host());
  return ret;
}

char *s0il_path_lookup(Ui *ui, const char *file)
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
      if (s0il_access (temp, R_OK) == F_OK)
      {
        return strdup(temp);
      }
    }
  }
  return NULL;
}

int
s0il_runvp (char *file, char **argv)
{
  if (file)
  if (file[0]==':')file++;
  char *path = file;
  if (path[0]!='/')
    path = s0il_path_lookup(NULL, file);
  if (path)
  {
    int retval = s0il_runv (path, argv);
    if (path != file)
      free (path);
    return retval;
  }
  return -1;
}

static void *s0il_thread(void *data)
{
  char **cargv = data;
  int ret = s0il_runvp (cargv[0], cargv);
  pthread_exit((void*)((size_t)ret));
  return (void*)((size_t)ret);
}

pthread_attr_t attr;
int s0il_spawnp (char **argv)
{
  pthread_t tid;
  int argc = 0;
  for (;argv[argc];argc++);
  char **argv_copy = calloc (sizeof(char*)*(argc+1), 1);
  for (int i = 0; i < argc; i++)
     argv_copy[i]=strdup(argv[i]);
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 8*1024);
  if (pthread_create(&tid, &attr, s0il_thread, argv_copy))
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
                o[j] != '>' &&
                o[j] != '|' &&
                o[j] != '&' &&
                o[j] != ';')
            j++;
        }

      if (o[j] == 0 ||
          o[j] == '>' ||
          o[j] == '|' ||
          o[j] == '&' ||
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

FILE *s0il_popen(const char *cmdline, const char *type)
{
  char *cargv[32];
  int   cargc;
  char *rest, *copy;
  if (!cmdline)
    return NULL;
  copy = calloc (strlen (cmdline)+3, 1);
  strcpy (copy, cmdline);
  rest = copy;
  FILE *out_stream = NULL;

    cargc = 0;
    while (rest && cargc < 30 && (rest[0] != ';' &&
                                  rest[0] != '&' &&
                                  rest[0] != '>' &&
                                  rest[0] != '|') )
      {
        cargv[cargc++] = rest;
        rest = string_chop_head (rest);
      }
    cargv[cargc] = NULL;
    if (cargv[0])
    {
      {

      char path[] = "/tmp/s0il_popen_0";
      out_stream = s0il_fopen(path, "w");

      s0il_redirect_io(NULL, out_stream);
      s0il_runvp (cargv[0], cargv);

      s0il_redirect_io(NULL, NULL);
      s0il_rewind (out_stream);
    }
    if (rest && (rest[0]==';'
              || rest[0]=='&'
              || rest[0]=='|'
              || rest[0]=='>'))
      {
        rest++;
        while (rest[0] == ' ')rest++;
      }
  } while (rest && rest[0]);
  free (copy);
  return out_stream;
}

int s0il_pclose(FILE *stream)
{
  return s0il_fclose (stream);
}

int
s0il_system (const char *cmdline)
{
  char *cargv[32];
  int   cargc;
  char *rest, *copy;
  int ret = 0;
  if (!cmdline)
    return -4;
  copy = calloc (strlen (cmdline)+3, 1);
  strcpy (copy, cmdline);
  rest = copy;

  int pipe_no = 0;
  do {
    cargc = 0;
    while (rest && cargc < 30 && (rest[0] != ';' &&
                                  rest[0] != '&' &&
                                  rest[0] != '>' &&
                                  rest[0] != '|') )
      {
        cargv[cargc++] = rest;
        rest = string_chop_head (rest);
      }
    cargv[cargc] = NULL;
    if (cargv[0])
    {
      if(strchr(cargv[0],'='))
      {
         char *key = cargv[0];
         char *value = strchr(cargv[0],'=')+1;
         value[-1]=0;
         setenv(key, value, 1);
      }
      else
      {

      FILE *in_stream = NULL;
      FILE *out_stream = NULL;
      if (pipe_no)
      {
        char path[] = "/tmp/s0il_pipe_0";
        path[15] = pipe_no-1+'0';
        in_stream = s0il_fopen(path, "r");
      }

      if (rest && rest[0]=='|')
      {
         char path[] = "/tmp/s0il_pipe_0";
         path[15] = pipe_no +'0';
         out_stream = s0il_fopen(path, "w");
         pipe_no++;
      }
      else if (rest && rest[0]=='>' && rest[1]=='>')
      {
         char *path = rest + 1;
         while (*path == ' ')path++;
         out_stream = s0il_fopen(path, "w+");
         pipe_no = 0;
      }
      else if (rest && rest[0]=='>')
      {
         char *path = rest + 1;
         while (*path == ' ')path++;
         out_stream = s0il_fopen(path, "w");
         pipe_no = 0;
      }
      else
      {
         pipe_no = 0;
      }

      if (in_stream || out_stream)
         s0il_redirect_io(in_stream, out_stream);

      if (rest && rest[0]=='&')
      {
         int pid = s0il_spawnp (cargv);
         if (pid <= 0) printf ("spawn failed\n");
         else {
           printf("got pid:%i\n", pid);
           ret = pid;
         }
      }
      else
      {
        ret = s0il_runvp (cargv[0], cargv);
      }

      s0il_redirect_io(NULL, NULL);
      if (in_stream)
        s0il_fclose (in_stream);
      if (out_stream)
        s0il_fclose (out_stream);

      }
    }
    if (rest && (rest[0]==';'
              || rest[0]=='&'
              || rest[0]=='|'
              || rest[0]=='>'))
      {
        rest++;
        while (rest[0] == ' ')rest++;
      }
  } while (rest && rest[0]);
  free (copy);
  ctx_reset_has_exited (ctx_host());
  return ret;
}
