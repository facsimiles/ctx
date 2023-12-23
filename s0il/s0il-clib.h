#ifndef S0IL_CLIB_H
#define S0IL_CLIB_H

// this set stdin and stdout to be aliases for the given streams,
// pass in NULL to reset
void s0il_redirect_io(FILE *in_stream, FILE *out_stream);

void   *s0il_malloc   (size_t size);
void    s0il_free     (void *ptr);
void   *s0il_calloc   (size_t nmemb, size_t size);
void   *s0il_realloc  (void *ptr, size_t size);
char   *s0il_strdup   (const char *s);
char   *s0il_strndup  (const char *s, size_t n);

int     s0il_atexit   (void (*function)(void));
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
int s0il_remove(const char *pathname);
long s0il_telldir(DIR *dir);
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
int     s0il_rmdir    (const char *path);
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
char   *s0il_realpath (const char *path, char *resolved_path);

int s0il_fsync(int fd);
int s0il_ftruncate(int fd, int length);
int s0il_mkdir(const char *pathname, int mode);
int s0il_truncate(const char *path, int length);

char *s0il_getenv (const char *name);
int   s0il_setenv (const char *name, const char *value, int overwrite);

typedef struct {
  size_t gl_pathc;
  char **gl_pathv;
  size_t gl_offs;
} s0il_glob_t;

#ifndef GLOB_ERR
#define GLOB_ERR (1<<0)
#define GLOB_MARK (1<<1)
#define GLOB_NOSORT (1<<2)
#define GLOB_DOOFFS (1<<3)
#define GLOB_NOCHECK (1<<4)
#define GLOB_APPEND  (1<<5)
#define GLOB_NOESCAPE (1<<6)
#define GLOB_PERIOD   (1<<7)
#define GLOB_TILDE    (1<<8)
#define GLOB_NOSPACE  1
#define GLOB_ABORTED  2
#define GLOB_NOMATCH  3
#define GLOB_NOSYS    4
#endif

int s0il_glob (const char *pattern, int flags, int(*errfunc)(char*,int),
               void *glob_buf);

#ifdef S0IL_REDEFINE_CLIB

#define realpath(a,b) s0il_realpath(a,b)
#define glob(a,b,c,d) s0il_glob(a,b,c,d)
#define getenv(a)     s0il_getenv(a) 
#define setenv(a,b,c) s0il_setenv(a,b,c) 

#define popen(a,b)    s0il_popen(a,b)
#define pclose(a)     s0il_pclose(a)
#define exit(r)       s0il_exit(r)
#define access(a,b)   s0il_access(a,b)
#define lseek(a,b,c)  s0il_lseek(a,b,c)
#define opendir(a)    s0il_opendir(a)
#define readdir(a)    s0il_readdir(a)
#define closedir(a)   s0il_closedir(a)
#define getpid()      s0il_getpid()
#define getppid()     s0il_getppid()
#define unlink(a)     s0il_unlink(a)
#define remove(a)     s0il_remove(a)
#define stat(p,b)     s0il_stat(p,b)
#define rename(p,b)   s0il_rename(p,b)
#define system(c)     s0il_system(c)

#define select(n,r,w,e,t) s0il_select(n,r,w,e,t)
#define signal(a,b)       s0il_signal(a,b)
#define fgets(a,b,c)      s0il_fgets(a,b,c)
#define fsetpos           s0il_fsetpos
#define fgetpos           s0il_fgetpos
#define fflush(a)         s0il_fflush(a)
#define fopen(a,b)        s0il_fopen(a,b)
#define freopen(a,b,c)    s0il_freopen(a,b,c)
#define fdopen(a,b)       s0il_fdopen(a,b)
#define fclose(a)         s0il_fclose(a)
#define fread(a,b,c,d)    s0il_fread(a,b,c,d)
#define fgetc(a)          s0il_fgetc(a)
#define getc(a)           s0il_getc(a)
#define getchar()         s0il_getchar()
#define ungetc(a,b)       s0il_ungetc(a,b)
#define fseek(a,b,c)      s0il_fseek(a,b,c)
#define ftell(a)          s0il_ftell(a)
#define rewind(a)         s0il_rewind(a)
#define read(a,b,c)       s0il_read(a,b,c)
#define getcwd(a,b)       s0il_getcwd(a,b)
#define chdir(a)          s0il_chdir(a)

#define putchar(a)        s0il_putchar(a)
#define fputs(a,b)        s0il_fputs(a,b)
#define fputc(a,b)        s0il_fputc(a,b)
#define putc(a,b)         s0il_fputc(a,b)
#define write(a,b,c)      s0il_write(a,b,c)
#define fwrite(a,b,c,d)   s0il_fwrite(a,b,c,d)
#define puts(a)           s0il_puts(a)

#define ctx_new(a,b,c)    s0il_ctx_new(a,b,c)
#define ctx_destroy(a)    s0il_ctx_destroy(a)
#define getline(a,b,c)    s0il_getline(a,b,c)
#define rmdir(p)          s0il_rmdir(p)
#define telldir(p)          s0il_telldir(p)

#define truncate(p,l)     s0il_truncate(p,l)
#define fsync(fd)         s0il_fsync(fd)
#define ftruncate(fd, l)  s0il_ftruncate(fd,l)
#define mkdir(p,m)        s0il_mkdir(p,m)

#define fprintf           s0il_fprintf
#define vfprintf          s0il_vfprintf
#define printf            s0il_printf

#define malloc(a) s0il_malloc(a)
#define free(a) s0il_free(a)
#define atexit(a) s0il_atexit(a)
#define calloc(a,b) s0il_calloc(a,b)
#define realloc(a,b) s0il_realloc(a,b)
#define strdup(a) s0il_strdup(a)
#define strndup(a,n) s0il_strdup(a,n)


#endif

#if PICO_BUILD
#define usleep(us) sleep_us(us)
#endif

#endif
