#include <stdbool.h>
#ifndef _RUN_H
#define _RUN_H

#include <sys/types.h>
#include <dirent.h>

int   runv (char *pathname, char **argv);
int   runvp (char *file, char **argv);

void  run_inline_main (const char *name, int(*main)(int argc, char **argv));

int   runs (const char *cmdline);


void run_add_file(const char *path, const char *contents, size_t size, bool readonly);

// returns pid or -1 on fail // XXX : needs work - not fully working
int   spawnp (char **argv);


void run_output_state_reset (void);
int  run_output_state (void); // 1 text 2 graphics 3 both

char *run_fgets(char *s, int size, FILE *stream);
int run_access(const char *pathname, int mode);
int     run_putchar (int c);
int     run_fputs (const char *s, FILE *stream);
int     run_fputc (int c, FILE *stream);
ssize_t run_write(int fd, const void *buf, size_t count);
int     run_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream);
int     run_puts (const char *s);
int     run_fprintf (FILE *stream, const char *restrict format, ...);
int     run_vfprintf (FILE *stream, const char *format, va_list ap);
int     run_printf (const char *restrict format, ...);
void   *run_ctx_new(int width, int height, const char *backend);
void    run_ctx_destroy(void *ctx);
int     run_access (const char *pathname, int mode);
ssize_t run_getline(char **lineptr, size_t *n, FILE *stream);

off_t   run_lseek(int fd, off_t offset, int whence);
DIR    *run_opendir(const char *name);
int     run_closedir(DIR *dirp);
struct  dirent *run_readdir(DIR *dirp);
pid_t  run_getpid(void);
pid_t  run_getppid(void);
int    run_stat(const char *pathname, struct stat *statbuf);
int    run_fstat(int fd, struct stat *statbuf);



FILE *run_fopen(const char *pathname, const char *mode);
FILE *run_fdopen(int fd, const char *mode);
int run_fclose(FILE *stream);
size_t run_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int run_fgetc(FILE *stream);
int run_getc(FILE *stream);
int run_getchar(void);
int run_ungetc(int c, FILE *stream);
int run_fseek(FILE *stream, long offset, int whence);
long run_ftell(FILE *stream);
void run_rewind(FILE *stream);
int run_fflush (FILE *stream);
ssize_t run_read(int fildes, void *buf, size_t nbyte);

int run_fgetpos(FILE *s, fpos_t *pos);
int run_fsetpos(FILE *s, fpos_t *pos);


// 
void run_signal(int sig, void(*func)(int));

char *run_getcwd(char *buf, size_t size);
int   run_chdir(const char *path);

#ifdef _RUN_REDEFINES_
#define access run_access
#define lseek run_lseek
#define opendir run_opendir
#define readdir run_readdir
#define closedir run_closedir
#define getpid run_getpid
#define getppid run_getppid
#define stat(p,b) run_stat(p,b)

//#define signal      run_signal
#define fgets       run_fgets
#define fsetpos     run_fsetpos
#define fgetpos     run_fgetpos
#define fflush      run_fflush
#define fopen       run_fopen
#define fdopen      run_fdopen
#define fclose      run_fclose
#define fread       run_fread
#define fgetc       run_fgetc
#define getc        run_getc
#define getchar     run_getchar
#define ungetc      run_ungetc
#define fseek       run_fseek
#define ftell       run_ftell
#define rewind      run_rewind
#define read        run_read
#define getcwd      run_getcwd
#define chdir       run_chdir

#define putchar     run_putchar
#define fputs       run_fputs
#define fputc       run_fputc
#define putc        run_fputc
#define write       run_write
#define fwrite      run_fwrite
#define puts        run_puts
#define fprintf     run_fprintf
#define vfprintf    run_vfprintf
#define printf      run_printf
#define ctx_new     run_ctx_new
#define ctx_destroy run_ctx_destroy

#endif

#endif
