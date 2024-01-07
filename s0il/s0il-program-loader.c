#include "port_config.h"
#include "s0il.h"
Ctx *ctx_host(void);
#define MAX_THREADS 4
void *thread_data[MAX_THREADS] = {
    NULL,
};
int thread_pid[MAX_THREADS] = {
    0,
};
void *_s0il_thread_id(void);

void *_s0il_main_thread = NULL;
static CtxList *proc = NULL;

void s0il_program_runner_init(void) {
  if (_s0il_main_thread)
    return;
  thread_data[0] = _s0il_main_thread = _s0il_thread_id();

  s0il_process_t *info = calloc(1, sizeof(s0il_process_t));
  info->ppid = 0;
  info->pid = 0;
  info->cwd = strdup("/");
  info->program = strdup("s0il");
  ctx_list_append(&proc, info);
}

int s0il_thread_no(void) {
  void *id = _s0il_thread_id();
  for (int i = 0; i < MAX_THREADS; i++)
    if (thread_data[i] == id)
      return i;
  // exit(2);
  return 0;
}

static int peak_pid = 0;

s0il_process_t *s0il_process(void) {
  int curpid = thread_pid[s0il_thread_no()];
  for (CtxList *iter = proc; iter; iter = iter->next) {
    s0il_process_t *info = iter->data;
    if (info->pid == curpid)
      return info;
  }
  // assert(0);
  if (proc)
    return proc->data;
  s0il_process_t *info = calloc(1, sizeof(s0il_process_t));
  info->ppid = 0;
  info->pid = 0;
  info->cwd = strdup("/");
  info->program = strdup("s0il");
  ctx_list_append(&proc, info);
  return info;
}

#include <unistd.h>

int output_state = 0;
int pre_exec(const char *path, int same_stack) {
  s0il_process_t *pinfo = s0il_process();
  s0il_process_t *info = calloc(1, sizeof(s0il_process_t));

  if (pinfo) {
    info->ppid = pinfo->pid;
    info->cwd = strdup(pinfo->cwd);
    info->redir_stdin = pinfo->redir_stdin;
    info->redir_stdout = pinfo->redir_stdout;
    info->redir_stderr = pinfo->redir_stderr;
  } else {
    info->ppid = 0;
    info->cwd = strdup("/");
  }
  info->program = strdup(path);

  info->pid = ++peak_pid;
  ctx_list_append(&proc, info);

  return info->pid;
}

void post_exec(int pid, int same_stack) {
  output_state = 0;
  s0il_process_t *info = NULL;
  for (CtxList *iter = proc; iter; iter = iter->next) {
    info = iter->data;
    if (info->pid == pid)
      break;
    info = NULL;
  }
  if (info) {
    if (info->cwd)
      free(info->cwd);
    if (info->program)
      free(info->program);
    ctx_list_remove(&proc, info);
  }
}
int s0il_thread_no(void);

int ps_main(int argc, char **argv) {
  for (CtxList *iter = proc; iter; iter = iter->next) {
    s0il_process_t *info = iter->data;
    s0il_printf("%i", info->pid);
    if (info->program)
      s0il_printf(" %s", info->program);
    if (info->cwd)
      s0il_printf("   %s", info->cwd);
    s0il_printf("\n");
  }
  return 0;
}

typedef struct inlined_program_t {
  char *base;
  char *path;
  int (*main)(int argc, char **argv);
} inlined_program_t;

static CtxList *inlined_programs = NULL;

void s0il_bundle_main(const char *name, int (*main)(int argc, char **argv)) {
  inlined_program_t *program = calloc(sizeof(inlined_program_t), 1);
  program->base = strdup(name);
  program->path = malloc(strlen(name) + 10);
  program->main = main;
  sprintf(program->path, "/bin/%s", name);
  ctx_list_append(&inlined_programs, program);
  static const char busy_magic[6] = {0, 's', '0', 'i', 'l'};

  if (s0il_access(program->path, R_OK) != F_OK)
    s0il_add_file(program->path, busy_magic, sizeof(busy_magic), S0IL_READONLY);
}
#include <libgen.h>

static void bundled_list(void) {
  for (CtxList *iter = inlined_programs; iter; iter = iter->next) {
    inlined_program_t *program = iter->data;
    s0il_printf("%s ", program->base);
  }
}

char *ui_basename(const char *in) {
  if (strchr(in, '/'))
    return strrchr(in, '/') + 1;
  return (char *)in;
}

int main_bundled(int argc, char **argv) {
  char *tmp = strdup(argv[0]);
  char *base = ui_basename(tmp);

  if (!strcmp(base, "bundled")) {
    if (argv[1] == NULL) {
      s0il_printf("Usage: bundled <command> [args ..]\n  commands: ");
      bundled_list();
      s0il_printf("\n");
      return 0;
    } else {
      return main_bundled(argc - 1, &argv[1]);
    }
  }

  for (CtxList *iter = inlined_programs; iter; iter = iter->next) {
    inlined_program_t *program = iter->data;
    if (!strcmp(base, program->base)) {
      int ret = program->main(argc, argv);
      free(tmp);
      return ret;
    }
  }
  printf("no matching bunlde %s\nValid commands: ", base);
  bundled_list();
  printf("\n");
  free(tmp);
  return -1;
}

static char *resolve_cmd(const char *cmd) {
  if (!strcmp(cmd, "foo"))
    return "foo thing";
  if (!strcmp(cmd, "ls"))
    return "bin sd";
  return "";
}

typedef enum {
  S0IL_CMD_DEFAULT = 0,
  S0IL_CMD_IN_ARG,
  S0IL_CMD_STRING,
  S0IL_CMD_QUOT,
  S0IL_CMD_DQUOT,
  S0IL_CMD_STRING_VAR,
  S0IL_CMD_DQUOT_VAR,
  S0IL_CMD_RQUOT,
  S0IL_CMD_STRING_ESCAPE,
  S0IL_CMD_QUOT_ESCAPE,
  S0IL_CMD_DQUOT_ESCAPE,
  S0IL_CMD_RQUOT_ESCAPE,
  S0IL_CMD_TERMINATOR,
  S0IL_CMD_WHITE_SPACE,
} argv_state;

static char **s0il_parse_cmdline(const char *input, char *terminator,
                                 const char **rest) {
  /*
    still missing:  `` inside strings
                       wildcard expansion
                       more valid escapes
  */

  char **argv = NULL;
  int arg_count = 0;

  char *out = 0;

#define case_TERMINATORS                                                       \
  case '>':                                                                    \
  case '&':                                                                    \
  case '\n':                                                                   \
  case '\0':                                                                   \
  case '|':                                                                    \
  case '#':                                                                    \
  case ';'

#define case_VAR_BREAKER                                                       \
  case '-':                                                                    \
  case ' ':                                                                    \
  case '+':                                                                    \
  case '='

  for (int write = 0; write < 2; write++) // first round is non write
  {
    char variable[32];
    int varlen = 0;
    char cmd[256];
    int cmdlen = 0;
    int total_length = 0;
    int arg_length = 0;
    int state = S0IL_CMD_DEFAULT;
    const char *p = input;
    for (; (p == input || p[-1]) && state != S0IL_CMD_TERMINATOR; p++) {
      switch (state) {
      case S0IL_CMD_DEFAULT:
        switch (*p) {
        case '$':
          state = S0IL_CMD_STRING_VAR;
          varlen = 0;
          break;
        case '\'':
          state = S0IL_CMD_QUOT;
          break;
        case '"':
          state = S0IL_CMD_DQUOT;
          break;
        case '`':
          state = S0IL_CMD_RQUOT;
          cmdlen = 0;
          cmd[cmdlen] = 0;
          break;
        case ' ':
          break;
        case_TERMINATORS:
          state = S0IL_CMD_TERMINATOR;
          if (terminator)
            *terminator = *p;
          break;
        default:
          state = S0IL_CMD_STRING;
          if (write)
            *(out++) = *p;
          arg_length++;
          break;
        }
        break;
      case S0IL_CMD_STRING:
        switch (*p) {
        case ' ':
          state = S0IL_CMD_DEFAULT;
          total_length += arg_length + 1;
          arg_count++;
          out++;
          if (write)
            argv[arg_count] = out;
          arg_length = 0;
          break;
          //    case '\\':
          //      state = S0IL_CMD_STRING_ESCAPE;
          //      break;
        case '"':
          state = S0IL_CMD_DQUOT;
          break;
        case '`':
          state = S0IL_CMD_RQUOT;
          cmdlen = 0;
          cmd[cmdlen] = 0;
          break;
        case_TERMINATORS:
          total_length += arg_length + 1;
          arg_count++;
          if (write)
            argv[arg_count] = out;
          arg_length = 0;
          state = S0IL_CMD_TERMINATOR;
          if (terminator)
            *terminator = *p;
          break;
        default:
          if (write)
            *(out++) = *p;
          arg_length++;
          break;
        }
        break;

      case S0IL_CMD_STRING_VAR:
        switch (*p) {
        case_TERMINATORS:
          if (terminator)
            *terminator = *p;
          state = S0IL_CMD_TERMINATOR;
          {
            const char *value = s0il_getenv(variable);
            if (value) {
              for (int i = 0; value[i]; i++) {
                if (write)
                  *(out++) = value[i];
                arg_length++;
              }
            }
          }
          break;
        case_VAR_BREAKER:
          if (state != S0IL_CMD_TERMINATOR)
            state = S0IL_CMD_STRING;
          {
            const char *value = s0il_getenv(variable);
            if (value) {
              for (int i = 0; value[i]; i++) {
                if (write)
                  *(out++) = value[i];
                arg_length++;
              }
            }
          }
          break;
        default:
          if (varlen + 1 < sizeof(variable))
            variable[varlen++] = *p;
          variable[varlen] = 0;
        }
        break;
      case S0IL_CMD_DQUOT_VAR:
        switch (*p) {
        case_TERMINATORS:
        case_VAR_BREAKER:
        case '"':
          state = S0IL_CMD_DQUOT;
          {
            const char *value = s0il_getenv(variable);
            if (value) {
              for (int i = 0; value[i]; i++) {
                if (write)
                  *(out++) = value[i];
                arg_length++;
              }
            }
          }
          if (*p == '"')
            p--;
          break;
        default:
          if (varlen + 1 < sizeof(variable))
            variable[varlen++] = *p;
          variable[varlen] = 0;
        }
        break;

      case S0IL_CMD_IN_ARG:
        switch (*p) {
        case '\'':
          state = S0IL_CMD_QUOT;
          break;
        case '"':
          state = S0IL_CMD_DQUOT;
          break;
        case '`':
          state = S0IL_CMD_RQUOT;
          break;
        case ' ':
          state = S0IL_CMD_DEFAULT;
          arg_count++;
          out++;
          if (write)
            argv[arg_count] = ++out;
          total_length += arg_length + 1;
          arg_length = 0;
          break;
        case_TERMINATORS:
          total_length += arg_length + 1;
          arg_length = 0;
          arg_count++;
          out++;
          if (write)
            argv[arg_count] = out;
          state = S0IL_CMD_TERMINATOR;
          break;
        default:
          state = S0IL_CMD_STRING;
          if (write)
            *(out++) = *p;
          break;
        }
        break;
      case S0IL_CMD_QUOT:
        switch (*p) {
        case '\'':
          state = S0IL_CMD_IN_ARG;
          break;
        default:
          if (write)
            *(out++) = *p;
          else
            arg_length++;
          break;
        }
        break;
      case S0IL_CMD_DQUOT:
        switch (*p) {
        case '$':
          state = S0IL_CMD_DQUOT_VAR;
          varlen = 0;
          break;
        case '"':
          state = S0IL_CMD_IN_ARG;
          break;
        case '\\':
          state = S0IL_CMD_DQUOT_ESCAPE;
          break;
        default:
          if (write)
            *(out++) = *p;
          break;
        }
        break;
      case S0IL_CMD_RQUOT:
        switch (*p) {
        case '`':
          state = S0IL_CMD_IN_ARG;

          {
            const char *value = resolve_cmd(cmd);
            if (value) {
              for (int i = 0; value[i]; i++) {
                if (write)
                  *(out++) = value[i];
                arg_length++;
              }
            }
          }
          break;
        case '\\':
          state = S0IL_CMD_RQUOT_ESCAPE;
          break;
        default:
          if (cmdlen + 1 < sizeof(cmd))
            cmd[cmdlen++] = *p;
          cmd[cmdlen] = 0;
          break;
        }
        break;
      case S0IL_CMD_RQUOT_ESCAPE:
        switch (*p) {
        case '\n':
          break;
        default:
          state = S0IL_CMD_RQUOT;
          if (cmdlen + 1 < sizeof(cmd))
            cmd[cmdlen++] = *p;
          cmd[cmdlen] = 0;
          break;
        }
        break;
      case S0IL_CMD_DQUOT_ESCAPE:
        switch (*p) {
        case '\n':
          break;
        case '"':
        case '`':
        case '\\':
          state = S0IL_CMD_DQUOT;
          if (write)
            *(out++) = *p;
          else
            arg_length++;
          break;
        default:
          state = S0IL_CMD_DQUOT;
          if (write) {
            *(out++) = '\\';
            *(out++) = *p;
          } else {
            arg_length += 2;
          }
          break;
        }
        break;
      case S0IL_CMD_STRING_ESCAPE:
        switch (*p) {
        case '\n':
          break;
        default:
          state = S0IL_CMD_STRING;
          if (write)
            *(out++) = *p;
          else
            arg_length++;
          break;
        }
        break;
      }
    }
#undef case_TERMINATORS

    // TODO : check if this is no longer needed with \0 incorporated in
    // state machine
    if (state == S0IL_CMD_STRING_VAR) {
      state = S0IL_CMD_STRING;
      {
        const char *value = s0il_getenv(variable);
        if (value) {
          for (int i = 0; value[i]; i++) {
            if (write)
              *(out++) = value[i];
            arg_length++;
          }
        }
      }
    }
    if (arg_length) {
      total_length += arg_length + 1;
      arg_count++;
      if (write)
        argv[arg_count] = NULL;
      arg_length = 0;
    }

    if (write == 0) {
      // XXX : the 8 is to make it valgrind clean.. overshoots of 2 have been
      // seen
      argv = calloc(1, total_length + (arg_count + 1) * sizeof(void *) + 1 + 8);
      out = (char *)(argv + (arg_count + 1));
      argv[0] = out;
      arg_count = 0;
    }

    if (rest)
      *rest = p;
  }
  argv[arg_count] = NULL;
  if (rest)
    if (**rest == 0)
      *rest = NULL;
  return argv;
}

#if !defined(PICO_BUILD)
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

void elf_close(elf_handle_t *elf) {
  esp_elf_deinit(&elf->elf);
  if (elf->path)
    free(elf->path);
  free(elf);
  ctx_list_remove(&elf_handles, elf);
}

elf_handle_t *elf_open(const char *path) {
  if (!strstr(path, "picoc"))
    for (CtxList *iter = elf_handles; iter; iter = iter->next) {
      elf_handle_t *elf = iter->data;
      if (!strcmp(path, elf->path))
        return elf;
    }

  FILE *file = s0il_fopen(path, "rb");
  if (file) {
    s0il_fseek(file, 0, SEEK_END);
    int length = s0il_ftell(file);
    s0il_fseek(file, 0, SEEK_SET);
    elf_handle_t *elf = calloc(sizeof(elf_handle_t), 1);
    elf->path = strdup(path);
    if (!elf)
      return NULL;
    uint8_t *data = malloc(length + 1);
    if (data) {
      s0il_fread(data, length, 1, file);
      s0il_fclose(file);
      esp_elf_init(&elf->elf);
      if (esp_elf_relocate(&elf->elf, data)) {
        free(data);
        return NULL;
      }
      memset(data, 0, length);
      free(data);
    }
    ctx_list_append(&elf_handles, elf);
    return elf;
  }
  return NULL;
}

static int esp_elf_runv(char *path, char **argv, int same_stack) {
  elf_handle_t *elf = elf_open(path);
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
  else {
    argc = 1;
    argv = fake_argv;
  }

  int pid = pre_exec(path, same_stack);
  int old_pid = thread_pid[s0il_thread_no()];
  thread_pid[s0il_thread_no()] = pid;

  retval = esp_elf_request(&elf->elf, 0 /* request-opt*/, argc, argv);

  thread_pid[s0il_thread_no()] = old_pid;
  post_exec(pid, same_stack);

  if (retval != 42) // TSR
  {
    elf_close(elf);
  }
  return retval;
}
#elif S0IL_NATIVE || defined(EMSCRIPTEN)
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

static int dlopen_runv(char *path2, char **argv, int same_stack) {
  char *path = path2;
  int argc = 0;
  if ((!path) && argv && argv[0])
    path = argv[0];
  char *fake_argv[2] = {path, NULL};
  if (argv)
    for (int i = 0; argv[i]; i++)
      argc++;
  else {
    argc = 1;
    argv = fake_argv;
  }

  if (!strncmp(path, "/bin/", 5)) {
    // XXX : we expect this to be an internal dir where dlopen
    //       does not work, thus make copies.
    char *tmp = malloc(strlen(path) + 20);
    sprintf(tmp, "/tmp/_s0il_elf_%s", path);
    for (int i = 8; tmp[i]; i++) {
      switch (tmp[i]) {
      case '/':
        tmp[i] = '_';
      }
    }
    if (access(tmp, R_OK) != F_OK) {
      char *cmd = malloc(strlen(tmp) * 2 + 10);
      FILE *in = s0il_fopen(path, "rb");
      FILE *out = fopen(tmp, "w");
      s0il_fseek(in, 0, SEEK_END);
      int length = s0il_ftell(in);
      s0il_fseek(in, 0, SEEK_SET);
      uint8_t *data = malloc(length + 1);
      if (data) {
        s0il_fread(data, length, 1, in);
        fwrite(data, length, 1, out);
      }
      s0il_fclose(in);
      fclose(out);
      free(cmd);
    }
    path = tmp;
  }

  void *dlhandle = dlopen(path, RTLD_NOW | RTLD_NODELETE);
  // if (path!= path2)
  //   unlink(path);

  if (dlhandle) {
    int (*main)(int argc, char **argv) = dlsym(dlhandle, "main");
    if (main) {

      int pid = pre_exec(path, same_stack);
      int old_pid = thread_pid[s0il_thread_no()];
      thread_pid[s0il_thread_no()] = pid;

      int ret = main(argc, argv);

      thread_pid[s0il_thread_no()] = old_pid;
      post_exec(pid, same_stack);

      if (ret != 42)
        dlclose(dlhandle);
    }
  }
  if (path != path2) {
    unlink(path);
    free(path);
  }

  return 0;
}
#else

///

#endif
#endif

static int program_runv(inlined_program_t *program, char **argv,
                        int same_stack) {
  char *path = program->path;
  int argc = 0;
  int ret = -1;
  char *fake_argv[2] = {path, NULL};
  if (argv)
    for (int i = 0; argv[i]; i++)
      argc++;
  else {
    argc = 1;
    argv = fake_argv;
  }
  {
    int pid = pre_exec(path, same_stack);
    int old_pid = thread_pid[s0il_thread_no()];
    thread_pid[s0il_thread_no()] = pid;

    ret = program->main(argc, argv);

    thread_pid[s0il_thread_no()] = old_pid;
    post_exec(pid, same_stack);
  }
  return ret;
}

int s0il_runv(char *path, char **argv) {
  s0il_program_runner_init();
  // printf (":::%s %s\n", path, argv?argv[0]:NULL);
  for (CtxList *iter = inlined_programs; iter; iter = iter->next) {
    inlined_program_t *program = iter->data;
    if (!strcmp(path, program->path)) {
      int retval = program_runv(program, argv, 1);
      ctx_reset_has_exited(ctx_host());
      return retval;
    }
  }

#if 1
  {
    FILE *f = s0il_fopen(path, "r");
    if (f) {
      uint8_t sector[512];
      s0il_fread(sector, 512, 1, f);
      s0il_fclose(f);

      if ((sector[0] == '/' && sector[1] == '/' && sector[2] == '!') ||
          (sector[0] == '#' && sector[1] == '!') ||
          (sector[0] == '/' && sector[1] == '/' && sector[2] == ' ' &&
           sector[3] == '!')) {
        int argc = 0;
        int diff = strchr((char *)sector, '!') - (char *)sector + 1;
        int ilen = (strchr((char *)sector, '\n') - ((char *)&sector[0])) - diff;
        char interpreter[ilen + 1];
        memcpy(interpreter, sector + diff, ilen);
        interpreter[ilen] = 0;

        for (; argv && argv[argc]; argc++)
          ;
        if (argc == 0)
          argc = 1;
        char *argv_expanded[argc + 4];
        argv_expanded[0] = interpreter;
        argv_expanded[1] = path;
        if (strstr(interpreter, "picoc") != NULL) {
          argv_expanded[2] = "-";
          for (int i = 1; i < argc; i++)
            argv_expanded[i + 2] = argv[i];
          argc += 2;
        } else {
          for (int i = 1; i < argc; i++)
            argv_expanded[i + 1] = argv[i];
          argc += 1;
        }
        argv_expanded[argc] = NULL;
        return s0il_runvp(interpreter, argv_expanded);
      } else
#if defined(EMSCRIPTEN)
          if (!(sector[0] == 0x0 && sector[1] == 'a' && sector[2] == 's' &&
                sector[3] == 'm'))
#else
          if (!(sector[0] == 0x7f && sector[1] == 'E' && sector[2] == 'L' &&
                sector[3] == 'F'))
#endif
      {
        s0il_printf("wrong magic: %c%c%c\n", sector[0], sector[1], sector[2]);
        return -3;
      }
    } else {
      return -2;
    }
  }
#endif

  int ret = 0;
#if CTX_FLOW3R
  ret = esp_elf_runv(path, argv, 1);
#elif S0IL_NATIVE || defined(EMSCRIPTEN)
  ret = dlopen_runv(path, argv, 1);
#endif
  ctx_reset_has_exited(ctx_host());
  return ret;
}

char *s0il_path_lookup(Ui *ui, const char *file) {
  const char *path[] = {"/sd/bin/", "/bin/", ":", NULL};
  char temp[512];

  for (int i = 0; path[i]; i++) {
    if (!strcmp(path[i], ":")) {
      for (CtxList *iter = inlined_programs; iter; iter = iter->next) {
        inlined_program_t *program = iter->data;
        if (!strcmp(program->base, file)) {
          return strdup(program->path);
        }
      }
    } else {
      snprintf(temp, sizeof(temp), "%s%s", path[i], file);
      if (s0il_access(temp, R_OK) == F_OK) {
        return strdup(temp);
      }
    }
  }
  return NULL;
}

int s0il_runvp(char *file, char **argv) {
  if (file)
    if (file[0] == ':')
      file++;
  char *path = file;
  if (path[0] != '/')
    path = s0il_path_lookup(NULL, file);
  if (path) {
    int retval = s0il_runv(path, argv);
    if (path != file)
      free(path);
    return retval;
  }
  return -1;
}

#ifndef PICO_BUILD

static void *s0il_thread(void *data) {
  char **cargv = data;
  int thread_no = 0;
  for (; thread_data[thread_no] && thread_no < MAX_THREADS; thread_no++)
    ;

  thread_data[thread_no] = _s0il_thread_id();
  int ret = s0il_runvp(cargv[0], cargv);
  thread_data[thread_no] = NULL;
  pthread_exit((void *)((size_t)ret));
  return (void *)((size_t)ret);
}

pthread_attr_t attr;
int s0il_spawnp(char **argv) {
  pthread_t tid;
  int argc = 0;
  for (; argv[argc]; argc++)
    ;
  char **argv_copy = calloc(sizeof(char *) * (argc + 1), 1);
  for (int i = 0; i < argc; i++)
    argv_copy[i] = strdup(argv[i]);
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 8 * 1024);
  if (pthread_create(&tid, &attr, s0il_thread, argv_copy))
    printf("failed spawning thread\n");
  return 23;
}
#else

int s0il_spawnp(char **argv) { return 23; }
#endif

FILE *s0il_popen(const char *cmdline, const char *type) {
  const char *rest = cmdline;
  char terminator = 0;
  if (!cmdline)
    return NULL;
  char **cargv = s0il_parse_cmdline(rest, &terminator, &rest);
  FILE *out_stream = NULL;
  if (cargv) {
    if (cargv[0]) {
      char path[] = "/tmp/s0il_popen_0";
      out_stream = s0il_fopen(path, "w");

      s0il_redirect_io(NULL, out_stream);
      s0il_runvp(cargv[0], cargv);

      s0il_redirect_io(NULL, NULL);
      s0il_rewind(out_stream);
    }
    free(cargv);
  }
  return out_stream;
}

int s0il_pclose(FILE *stream) { return s0il_fclose(stream); }

int s0il_system(const char *cmdline) {
  char **cargv = NULL;
  int cargc;
  char terminator = 0;
  const char *rest;
  int ret = 0;
  if (!cmdline)
    return -4;
  rest = cmdline;

  int pipe_no = 0;
  do {
    cargv = s0il_parse_cmdline(rest, &terminator, &rest);
    if (cargv) {
      for (cargc = 0; cargv[cargc]; cargc++)
        ;

      if (cargv[0]) {
        if (strchr(cargv[0], '=')) {
          char *key = cargv[0];
          char *value = strchr(cargv[0], '=') + 1;
          value[-1] = 0;
          setenv(key, value, 1);
        } else {

          FILE *in_stream = NULL;
          FILE *out_stream = NULL;
          if (pipe_no) {
            char path[] = "/tmp/s0il_pipe_0"; // todo : use mktemp
            path[15] = pipe_no - 1 + '0';
            in_stream = s0il_fopen(path, "r");
          }

          switch (terminator) {
          case '|': {
            char path[] = "/tmp/s0il_pipe_0";
            path[15] = pipe_no + '0';
            out_stream = s0il_fopen(path, "w");
            pipe_no++;
          } break;
          case '>': {
            int append = 0;
            if (rest[0] == '>') {
              append = 1;
              rest++;
            }
            const char *path = rest;
            while (*path == ' ')
              path++;
            out_stream = s0il_fopen(path, append ? "w" : "w+");
            pipe_no = 0;
          } break;
          default:
            pipe_no = 0;
          }

          if (in_stream || out_stream)
            s0il_redirect_io(in_stream, out_stream);
          // XXX : redirect should be handled in spawn..
          if (terminator == '&') {
            s0il_spawnp(cargv);
          } else {
            ret = s0il_runvp(cargv[0], cargv);
          }

          s0il_redirect_io(NULL, NULL);
          if (in_stream)
            s0il_fclose(in_stream);
          if (out_stream)
            s0il_fclose(out_stream);
        }
      }
      free(cargv);
    }
  } while (terminator && rest && rest[0]);
  ctx_reset_has_exited(ctx_host());
  return ret;
}
