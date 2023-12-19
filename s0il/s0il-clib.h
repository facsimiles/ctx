#ifndef S0IL_CLIB_H
#define S0IL_CLIB_H

// this set stdin and stdout to be aliases for the given streams,
// pass in NULL to reset
void s0il_redirect_io(FILE *in_stream, FILE *out_stream);

int     s0il_select   (int nfds, fd_set *read_fds,
                       fd_set *write_fds,
                       fd_set *except_fds,
                       struct timeval *timeout);
char   *s0il_fgets    (char *s, int size, FILE *stream);
int     s0il_access   (const char *pathname, int mode);
int     s0il_putchar  (int c);
int     s0il_fputs    (const char *s, FILE *stream);
int     s0il_fputc    (int c, FILE *stream);
ssize_t s0il_write    (int fd, const void *buf, size_t count);
int     s0il_fwrite   (const void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE   *s0il_popen    (const char *cmdline, const char *mode);
int     s0il_pclose   (FILE *stream);
int     s0il_puts     (const char *s);
int     s0il_unlink   (const char *s);
int     s0il_fprintf  (FILE *stream, const char *restrict format, ...);
int     s0il_vfprintf (FILE *stream, const char *format, va_list ap);
int     s0il_printf   (const char *restrict format, ...);
void   *s0il_ctx_new  (int width, int height, const char *backend);
void    s0il_ctx_destroy (void *ctx);
int     s0il_access   (const char *pathname, int mode);
ssize_t s0il_getline  (char **lineptr, size_t *n, FILE *stream);

int     s0il_system   (const char *cmdline);
off_t   s0il_lseek    (int fd, off_t offset, int whence);
DIR    *s0il_opendir  (const char *name);
int     s0il_closedir (DIR *dirp);
struct dirent *
        s0il_readdir  (DIR *dirp);
pid_t   s0il_getpid   (void);
pid_t   s0il_getppid  (void);
int     s0il_stat     (const char *pathname, struct stat *statbuf);
int     s0il_fstat    (int fd, struct stat *statbuf);
int     s0il_rename   (const char *src, const char *dst);
long    s0il_ftell    (FILE *stream);
off_t   s0il_ftello   (FILE *stream);

FILE   *s0il_fopen    (const char *pathname, const char *mode);
FILE   *s0il_freopen  (const char *pathname, const char *mode, FILE *stream);
FILE   *s0il_fdopen   (int fd, const char *mode);
int     s0il_fclose   (FILE *stream);
size_t  s0il_fread    (void *ptr, size_t size, size_t nmemb, FILE *stream);
int     s0il_fgetc    (FILE *stream);
int     s0il_getc     (FILE *stream);
int     s0il_getchar  (void);
int     s0il_ungetc   (int c, FILE *stream);
int     s0il_fseek    (FILE *stream, long offset, int whence);
long    s0il_ftell    (FILE *stream);
void    s0il_rewind   (FILE *stream);
int     s0il_fflush   (FILE *stream);
ssize_t s0il_read     (int fildes, void *buf, size_t nbyte);

int     s0il_fgetpos  (FILE *s, fpos_t *pos);
int     s0il_fsetpos  (FILE *s, fpos_t *pos);

void    s0il_exit     (int retval);
void    s0il_signal   (int sig, void(*func)(int));

char   *s0il_getcwd   (char *buf, size_t size);
int     s0il_chdir    (const char *path);

#ifdef S0IL_REDEFINE_CLIB

#define popen(a,b)  s0il_popen(a,b)
#define pclose(a)   s0il_pclose(a)
#define exit(r)     s0il_exit(r)
#define access(a,b) s0il_access(a,b)
#define lseek       s0il_lseek
#define opendir     s0il_opendir
#define readdir     s0il_readdir
#define closedir    s0il_closedir
#define getpid      s0il_getpid
#define unlink      s0il_unlink
#define getppid     s0il_getppid
#define stat(p,b)   s0il_stat(p,b)
#define rename(p,b) s0il_rename(p,b)
#define system(c)   s0il_system(c)

#define select(n,r,w,e,t) s0il_select(n,r,w,e,t)
#define signal(a,b) s0il_signal(a,b)
#define fgets       s0il_fgets
#define fsetpos     s0il_fsetpos
#define fgetpos     s0il_fgetpos
#define fflush      s0il_fflush
#define fopen       s0il_fopen
#define freopen     s0il_freopen
#define fdopen      s0il_fdopen
#define fclose      s0il_fclose
#define fread       s0il_fread
#define fgetc       s0il_fgetc
#define getc        s0il_getc
#define getchar     s0il_getchar
#define ungetc      s0il_ungetc
#define fseek       s0il_fseek
#define ftell       s0il_ftell
#define rewind      s0il_rewind
#define read(a,b,c) s0il_read(a,b,c)
#define getcwd      s0il_getcwd
#define chdir       s0il_chdir

#define putchar     s0il_putchar
#define fputs       s0il_fputs
#define fputc       s0il_fputc
#define putc        s0il_fputc
#define write       s0il_write
#define fwrite      s0il_fwrite
#define puts        s0il_puts
#define fprintf     s0il_fprintf
#define vfprintf    s0il_vfprintf
#define printf      s0il_printf
#define ctx_new     s0il_ctx_new
#define ctx_destroy s0il_ctx_destroy
#define getline     s0il_getline

#endif

#endif
