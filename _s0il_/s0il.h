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

#if CTX_FLOW3R
#include "lwip/sockets.h"
#include "lwip/igmp.h"
#include "lwip/ip4.h"
#include "lwip/netdb.h"

#else
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
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
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "ctx.h"
#include "ui.h"
#include "s0il-magic.h"
#include "s0il-run.h"
#include <libgen.h>

pid_t gettid(void);

extern void *_s0il_main_thread;
static inline void *_s0il_thread_id(void)
{
#if 1
#if CTX_FLOW3R
  return xTaskGetCurrentTaskHandle();
#else
  return (void*)((size_t)gettid());
#endif
#else
  return 0;
#endif
}

static inline bool s0il_is_main_thread()
{
#if CTX_FLOW3R
  return _s0il_thread_id() == _s0il_main_thread;
#else
  return gettid() == getpid();
#endif
}

Ctx *ctx_host(void);
#endif

#ifdef MAIN
#undef MAIN
#endif
#ifdef S0IL_BUNDLE
#define MAIN(name) int name##_main(int argc, char **argv)
#else
#define MAIN(name) int main (int argc, char **argv)
#endif
