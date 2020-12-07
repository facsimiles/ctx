#ifndef __TERMINAL__H
#define __TERMINAL__H
typedef struct VtPty
{
  int        pty;
  pid_t      pid;
  int        done;
} VtPty;
ssize_t vtpty_read     (void *vtpty, void *buf, size_t count);
ssize_t vtpty_write    (void *vtpty, const void *buf, size_t count);
void    vtpty_resize   (void *vtpty, int cols, int rows,
                        int px_width, int px_height);
int     vtpty_waitdata (void  *vtpty, int timeout);
extern  CtxList *vts;

#endif
