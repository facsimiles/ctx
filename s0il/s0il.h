#ifndef _S0IL_H_
#define _S0IL_H_

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// we include all relevant headers, the overhead is
// not that large

#if defined(PICO_BUILD)
  // what for sockets etc?
#define DT_DIR 4
#define DT_REG 8

 struct dirent {
               int            d_ino;       /* Inode number */
               int            d_off;       /* Not an offset; see below */
               unsigned short d_reclen;    /* Length of this record */
               unsigned char  d_type;      /* Type of file; not supported
                                              by all filesystem types */
               char           d_name[256]; /* Null-terminated filename */
           };
#define PATH_MAX 256

#elif defined(S0IL_NATIVE) || defined(WASM) || defined(RISCV)
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else

 #include "lwip/err.h"
 #include "lwip/sys.h"

 #include "lwip/sockets.h"
 #include "lwip/igmp.h"
 #include "lwip/ip4.h"
 #include "lwip/netdb.h"


#endif

#if defined(RISCV) || defined(CTX_ESP)  || defined(PICO_BUILD)
 struct winsize
  {
    unsigned short int ws_row;
    unsigned short int ws_col;
    unsigned short int ws_xpixel;
    unsigned short int ws_ypixel;
  };

#define TIOCSWINSZ 0x5414
#define TIOCGWINSZ 0x5413
#endif

#if defined(PICO_BUILD)

#define DIR void

#else
#include <dirent.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ctx.h"
#include "s0il-ui.h"
#include "s0il-clib.h"
#include <libgen.h>

pid_t gettid(void);

bool s0il_is_main_thread();
void *_s0il_thread_id(void);
Ctx *ctx_host(void);

typedef enum {
  S0IL_READONLY = (1<<0),
  S0IL_DIR      = (1<<1)
} s0il_file_flag;
void *s0il_add_file(const char *path, const char *contents, size_t size, s0il_file_flag flags);
void  s0il_bundle_main (const char *name, int(*main)(int argc, char **argv));

int   s0il_runv (char *pathname, char **argv);
int   s0il_runvp (char *file, char **argv);
int   s0il_spawnp (char **argv);

void   s0il_add_magic (const char *mime_type,
                  const char *ext,
                  const char *magic_data,
                  int magic_len,
                  int is_text);

bool s0il_has_mime(const char *mime_type);

const char *s0il_detect_media_sector512 (const char *path, const char *sector512);

const char *s0il_detect_media_path (const char *location);

void  s0il_output_state_reset (void);
int   s0il_output_state       (void); // 1 text 2 graphics 3 both
char *s0il_path_lookup        (Ui *ui, const char *command);

typedef struct _file_t   file_t;
typedef struct _folder_t folder_t;

typedef struct s0il_process_t {
  int       ppid;
  int       pid;
  char     *program;
  char     *cwd;
  CtxList  *atexits;
  folder_t *dir; // currently open folder
  FILE     *redir_stdout;
  FILE     *redir_stderr;
  FILE     *redir_stdin;
} s0il_process_t;

s0il_process_t *s0il_process(void);
int s0il_thread_no(void);

#endif

#ifdef MAIN
#undef MAIN
#endif
#ifdef S0IL_BUNDLE
#define MAIN(name) int name##_main(int argc, char **argv)
#else
#define MAIN(name) int main (int argc, char **argv)
#endif

#if EMSCRIPTEN
#include <emscripten.h>
#endif
