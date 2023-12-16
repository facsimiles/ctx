#include "s0il.h"

static int text_output = 0;
static int gfx_output  = 0;

int  s0il_output_state (void)
{
  return text_output * 1 + gfx_output * 2;
}
void s0il_output_state_reset (void)
{
  text_output = 0;
  gfx_output = 0;
}

void *s0il_ctx_new(int width, int height, const char *backend)
{
  gfx_output  = 1;
  return ctx_new(width, height, backend);
}

void s0il_ctx_destroy(void *ctx)
{
  gfx_output  = 0;
}
/////////

static char *s0il_cwd = NULL;

char *s0il_getcwd(char *buf, size_t size)
{
  if (!buf){ int size = 4; if (s0il_cwd) size = strlen(s0il_cwd)+2;
             buf = malloc(size);}
  if (!s0il_cwd)
    strncpy(buf, "/", size-1);
  else
    strncpy(buf, s0il_cwd, size-1);
  return buf;
}


static char *s0il_resolve_path(const char *pathname)
{
  char *path = (char*)pathname;

  if (pathname[0]!='/')
  {
     if (!s0il_cwd)s0il_cwd=strdup("/");
     path = malloc (strlen (pathname) + strlen (s0il_cwd) + 2);
     sprintf (path, "%s%s", s0il_cwd, pathname);
  }

  if (strstr(path, "/./"))
  {
    if (path == pathname) path = strdup (path);
    do {
    char *p = strstr (path, "/./");
    if (p==path)
    {
      path[1]=0;
      return path;
    }
    char *parent = p-1;
    while (parent!=path && *parent !='/') parent--;
      memmove(parent, p+3, strlen(p+2)+1);

    } while (strstr (path, "/./"));
  }

  if (strstr(path, "/../"))
  {
    if (path == pathname) path = strdup (path);
    do {
    char *p = strstr (path, "/../");
    if (p==path)
    {
      path[1]=0;
      return path;
    }
    char *parent = p-1;
    while (parent!=path && *parent !='/') parent--;
      memmove(parent, p+3, strlen(p+3)+1);

    } while (strstr (path, "/../"));
  }

  if (strstr(path, "/..") && strstr(path, "/..")[3]==0)
  {
    if (path == pathname) path = strdup (path);
    char *p = strstr (path, "/..");
    if (p==path)
    {
      path[1]=0;
      return path;
    }
    char *parent = p-1;
    while (parent!=path && *parent !='/') parent--;
      memmove(parent, p+3, strlen(p+2)+1);

  }


  return path;
}

int   s0il_chdir(const char *path2)
{
  char *path = s0il_resolve_path (path2);
  // XXX need better check?
  //if (!s0il_access (path, R_OK)) return -1;
  if (s0il_cwd) free (s0il_cwd);

  s0il_cwd = malloc (strlen(path)+2);
  strcpy (s0il_cwd, path);

  // append trailing / if missing
  if (s0il_cwd[strlen(s0il_cwd)-1]!='/')
  {
    s0il_cwd[strlen(s0il_cwd)+1]=0;
    s0il_cwd[strlen(s0il_cwd)]='/';
  }
  chdir(path);
  if (path2 != path) free (path);
  return 0;
}

typedef struct file_t {
  char    *path;
  char    *d_name;
  unsigned char d_type;
  bool     read_only;
  char    *data;
  size_t   size;
  size_t   pos;
} file_t;

typedef struct folder_t {
  char    *path;
  int      count;
  CtxList *files;
  int      pos;
} folder_t;

static DIR      *_s0il_internal_dir    = (void*)100;
static FILE     *_s0il_internal_file   = (void*)200;
static folder_t *_s0il_dir = NULL;
static file_t   *_s0il_file = NULL;

static CtxList *folders = NULL;

file_t *s0il_find_file(const char *path)
{
  char *parent = strdup (path);
  strrchr (parent, '/')[0]=0;
  if (parent[0]==0)
  {
    parent[0]='/';
    parent[1]=0;
  }
  folder_t *folder = NULL;

  for (CtxList *iter = folders; iter; iter=iter->next)
  {
    folder = iter->data;
    if (!strcmp (folder->path, parent))
    {
      break;
    }
  }
  if (!folder)
  {
    return NULL;
  }

  for (CtxList *iter = folder->files; iter; iter=iter->next)
  {
    file_t *file = iter->data;
    if (!strcmp (path, file->path))
    {
      return file;
    }
  }
  return NULL;
}

void s0il_add_file(const char *path, const char *contents, size_t size, s0il_file_flag flags)
{
  bool readonly = ((flags & RUN_READONLY) != 0);
  bool is_dir   = ((flags & RUN_DIR) != 0);

  char *parent = strdup (path);
  strrchr (parent, '/')[0]=0;
  folder_t *folder = NULL;
  if (size == 0 && !is_dir)
    size = strlen (contents);

  if (parent[0] == 0)
  {
    parent[0]='/';
    parent[1]=0;
  }

//printf ("adding %s to %s dir?%i\n", path, parent, is_dir);

  for (CtxList *iter = folders; iter; iter=iter->next)
  {
    folder = iter->data;
    if (!strcmp (folder->path, parent))
    {
      break;
    }
    folder = NULL;
  }
  if (!folder)
  {
    folder = calloc (sizeof (folder_t), 1);
    folder->path = parent;
    ctx_list_append (&folders, folder);
  }
  else
  {
    free(parent);
  }

  file_t *file = calloc(sizeof(file_t),1);
  file->path = (char*)(readonly?path:strdup (path));
  file->d_name = readonly?strrchr(path,'/')+1:strdup (strrchr (path, '/')+1);
  file->size = size;
  file->read_only = readonly;
  if (is_dir)
  {
    file->d_type = DT_DIR;
  }
  else
  {
    file->d_type = DT_REG;
    if (readonly)
      file->data = (char*)contents;
    else
    {
      file->data = malloc(file->size);
      memcpy(file->data, contents, file->size);
    }
  }
  if (folder)
  {
    folder->count++;
    ctx_list_append (&folder->files, file);
  }
}

#define WRAP_STDOUT 1

int s0il_putchar (int c)
{
#if WRAP_STDOUT
  text_output = 1;
  if (c == '\n')
  {
    ctx_vt_write(NULL, '\r');
    if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL)); // doing a ui iteration
                                 // per received char is a bit much
                                 // so we only do newlines
  }
  ctx_vt_write(NULL, c);
#endif
  return putchar (c);
}

int s0il_fputs (const char *s, FILE *stream)
{
  if (stream == _s0il_internal_file) return 0;
#if WRAP_STDOUT
  if (stream == stdout || stream == stderr)
  {
    text_output = 1;
    for (int i = 0; s[i]; i++)
    {
       if (s[i] == '\n')
         ctx_vt_write(NULL, '\r');
       ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  }
#endif
  int ret = fputs(s, stream);
  return ret;
}


int s0il_fputc (int c, FILE *stream)
{
  if (stream == _s0il_internal_file) return 0;
#if WRAP_STDOUT
  if (stream == stdout)
    return s0il_putchar (c);
#endif
  return fputc(c, stream);
}

ssize_t s0il_write (int fd, const void *buf, size_t count)
{
#if WRAP_STDOUT
  if (fd == 1 || fd == 2)
  {
    text_output = 1;
    uint8_t *s=(uint8_t*)buf;
    for (size_t i = 0; i< count; i++)
    {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  }
#endif
  return write(fd,buf,count);
}

int s0il_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (stream == _s0il_internal_file) return 0;
#if WRAP_STDOUT
  if (stream == stdout || stream == stderr)
  {
    text_output = 1;
    uint8_t *s=(uint8_t*)ptr;
    for (size_t i= 0; i< size*nmemb; i++)
    {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  }
#endif
  return fwrite (ptr, size, nmemb, stream);
}

int s0il_puts (const char *s)
{
  int ret = s0il_fputs(s, stdout);
  s0il_fputc ('\n', stdout);
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return ret;
}

int s0il_fprintf (FILE *stream, const char *restrict format, ...)
{
  if (stream == _s0il_internal_file) return 0;
  va_list ap;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_start (ap, format);
  va_copy(ap_copy, ap);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  ret = vsnprintf (buffer, needed, format, ap_copy);
  va_end (ap_copy);
  s0il_fputs (buffer, stream);
  free (buffer);
  return ret;
}

int s0il_vfprintf (FILE *stream, const char *format, va_list ap)
{
  if (stream == _s0il_internal_file) return 0;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_copy(ap_copy, ap);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  ret = vsnprintf (buffer, needed, format, ap_copy);
  va_end (ap_copy);
  s0il_fputs (buffer, stream);
  free (buffer);
  return ret;
}

int s0il_printf (const char *restrict format, ...)
{
  if (!format) return -1;
  va_list ap;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_start (ap, format);
  va_copy (ap_copy, ap);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  ret = vsnprintf (buffer, needed, format, ap_copy);
  va_end (ap_copy);
  s0il_fputs (buffer, stdout);
  free (buffer);
#if WRAP_STDOUT
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL)); // doing a ui iteration
#endif
  return ret;
}

int s0il_rename(const char *src, const char *dst)
{
  int ret = 0;
  if (!src || !dst)
    return -1;
  char *src_p = s0il_resolve_path(src);
  char *dst_p = s0il_resolve_path(dst);

  ret = rename(src_p, dst_p);
  if (src_p!=src)free (src_p);
  if (dst_p!=dst)free (dst_p);
  return ret;
}

FILE *s0il_fopen(const char *pathname, const char *mode)
{
  char *path = s0il_resolve_path (pathname);
  file_t *file = s0il_find_file (path);
  if (file)
  {
    _s0il_file = file;
    //_s0il_internal_file->_fileno = 1111;
    file->pos = 0;
    return _s0il_internal_file;
  }

  FILE *ret = fopen(path, mode);
  if (path != pathname) free (path);
  return ret;
}

FILE *s0il_fdopen(int fd, const char *mode)
{
  return fdopen(fd, mode);
}

int s0il_fclose(FILE *stream)
{
  if (stream == _s0il_internal_file)
  {
    _s0il_file = NULL;
    return 0;
  }
  return fclose(stream);
}


static int stdin_got_data(void)
{
  int gotdata = ctx_vt_has_data (NULL);
  if (gotdata) return 1;
#ifndef EMSCRIPTEN
  int c = s0il_fgetc(stdin);
  if (c>=0)
  {
    s0il_ungetc(c, stdin);
    return 1;
  }
#endif
  return 0;
}


static char *s0il_gets(char* buf, size_t buflen) {
    size_t count = 0;
    // used by the shell through fgets, and gets use interactive shell
    // on raw terminals - for now mostly pseudo what regular line-editing
    // mode would give.

    while (count < buflen) {
        int c;
        if (stdin_got_data())
          c = s0il_fgetc(stdin);
        else
        {
          if (s0il_is_main_thread())
            ui_iteration(ui_host(NULL));
#ifndef EMSCRIPTEN
          usleep (1000); // XXX : seems more stable with it
#endif
          continue;
        }
        if(count == 0)
        {
           switch(c)
           {
             case -1:
             case 4: //ctrl-d - exit
               buf[count]=0;
               return NULL;
             case 12://ctrl-l - clear
               s0il_fputs("\033c> ", stdout);
               continue;
           }
        }
        if (c == '\t')
           continue;

        if (c == '\n') {
            buf[count] = c;
            count++;
            break;
        } else if (c == '\b' || c == '\177') {
            if (count > 0) {
                buf[count - 1] = 0;
                count--;
                s0il_fputs("\x08 ", stdout);
                s0il_fputc('\b', stdout); /* echo */
            }
            continue;
        //} else if (c < 32) { fprintf(stdout, "{%i}", (int)c);
        } else {
            buf[count] = c;
            count++;
        }
        s0il_fputc(c, stdout); /* echo */
        if (s0il_is_main_thread())
          ui_iteration(ui_host(NULL));
    }
    s0il_fputc('\n', stdout);
    buf[count]=0;
    return buf;
}

int s0il_fgetc(FILE *stream)
{
  if (stream == stdin)
  {
    if (ctx_vt_has_data (NULL))
    {
      int c = ctx_vt_read (NULL);
      if (c == '\r') c = '\n';
      return c;
    }
  }
  if (stream == _s0il_internal_file)
  {
    int ret = EOF;
    if (_s0il_file->pos < _s0il_file->size)
    {
      ret = _s0il_file->data[_s0il_file->pos];
      _s0il_file->pos++;
    }
    return ret;
  }
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return fgetc(stream);
}

char *s0il_fgets(char *s, int size, FILE *stream)
{
  if (stream == _s0il_internal_file)
  {
    int ret = 0;
    if (_s0il_file->pos >=  _s0il_file->size)
      return NULL;
    for (; _s0il_file->pos <  _s0il_file->size; _s0il_file->pos++)
    {
       char c = _s0il_file->data[_s0il_file->pos];
       s[ret++] = c;
       if (c == '\n')
         break;
    }
    s[ret]=0;
    return s;
  }
  if (stream == stdin)
  {
    return s0il_gets(s, size);
  }
  return fgets(s, size, stream);
}

int s0il_ungetc(int c, FILE *stream)
{ // TODO : unget to ctx|term layer insteead
  if (stream == _s0il_internal_file) return 0;
  return ungetc(c, stream);
}


size_t s0il_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (stream == stdin)
  {
    char *dst = ptr;
    int read = 0;
    for (unsigned i = 0; i < size * nmemb; i++)
    {
      if (ctx_vt_has_data (NULL))
      {
        dst[i]=ctx_vt_read(NULL);
        read++;
      }
    }
    // XXX : this only works well when we can satisfy the reads..
    return read/size;
  }


  if (stream == _s0il_internal_file)
  {
    int request = size * nmemb;
    if (_s0il_file->pos + request > _s0il_file->size)
      request = _s0il_file->size - _s0il_file->pos;
    if (request <= 0)
      return 0;
    memcpy(ptr, _s0il_file->data + _s0il_file->pos, request);
    _s0il_file->pos+=request;
    return request;
  }
  //ui_iteration(ui_host(NULL));
  return fread(ptr,size,nmemb,stream);
}

int s0il_getc(FILE *stream)
{
  return s0il_fgetc(stream);
}

ssize_t s0il_read(int fildes, void *buf, size_t nbyte)
{
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return read(fildes,buf,nbyte);
}

int s0il_access(const char *pathname, int mode)
{
  if (s0il_find_file (pathname)) return F_OK;
  return access(pathname, mode);
}



int s0il_fflush (FILE *stream)
{
  if (stream == _s0il_internal_file) return 0;
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return fflush (stream);
}


// positions
int s0il_fsetpos(FILE *stream, fpos_t *pos)
{
#if 0
  if (stream == _s0il_internal_file)
  {
    _s0il_file->pos = *pos;
    return 0;
  }
#endif
  return fsetpos(stream, pos);
}

off_t s0il_lseek(int fd, off_t offset, int whence)
{
  return lseek(fd, offset, whence);
}

int s0il_fseek(FILE *stream, long offset, int whence)
{
  if (stream == _s0il_internal_file)
  {
    switch(whence)
    {
      case SEEK_SET:
        _s0il_file->pos = 0;
        return _s0il_file->pos;
      case SEEK_CUR:
        return _s0il_file->pos;
      case SEEK_END:
        _s0il_file->pos = _s0il_file->size;
        return _s0il_file->pos;
    }
  }
  return fseek(stream, offset, whence);
}

void s0il_rewind(FILE *stream)
{
  if (stream == _s0il_internal_file)
  {
    _s0il_file->pos = 0;
    return;
  }
  rewind(stream);
}

int s0il_fgetpos(FILE *s, fpos_t *pos)
{
#if 0
  if (stream == _s0il_internal_file)
  {
    *pos = _s0il_file->pos;
    return 0;
  }
#endif
  return fgetpos(s, pos);
}

long
s0il_ftell (FILE *stream) {
  if (stream == _s0il_internal_file)
    return _s0il_file->pos;
  return ftell(stream);
}

off_t
s0il_ftello (FILE *stream) {
  if (stream == _s0il_internal_file)
    return _s0il_file->pos;
  return ftello(stream);
}

// pid info

pid_t s0il_getpid(void)
{
  return 1;
}

pid_t s0il_getppid(void)
{
  return 0;
}

// fs/dir bits

DIR    *s0il_opendir(const char *name2)
{
  char *name = (char*)name2;
  if (name[strlen(name)-1]=='/')
  {
    name = strdup(name);
    name[strlen(name)-1]=0;
  }
  if (name[1]==0)
  {
    name[0]='/';
    name[1]=0;
  }
  for (CtxList *iter = folders; iter; iter=iter->next)
  {
    folder_t *folder = iter->data;
    if (!strcmp (folder->path, name))
    {
      _s0il_dir = folder;
      _s0il_dir->pos = 0;
      if (name != name2)
        free (name);
      return _s0il_internal_dir;
    }
  }
  DIR *ret = opendir(name);
  if (name != name2)
    free (name);
  return ret;
}

struct  dirent *
s0il_readdir(DIR *dirp)
{
  if (dirp == _s0il_internal_dir)
  {
     static struct dirent ent;
     if (_s0il_dir->pos < _s0il_dir->count)
     {
       int i = 0;
       file_t *file = NULL;
       for (CtxList *iter = _s0il_dir->files; iter; iter=iter->next, i++)
       {
         if (i == _s0il_dir->pos) file=iter->data;
       }
       if (!file) return NULL;
       _s0il_dir->pos++;
       strncpy (ent.d_name, file->d_name, sizeof (ent.d_name)-1);
       ent.d_type = file->d_type;
       return &ent;
     }
     return NULL;
  }
  return readdir(dirp);
}

int
s0il_closedir (DIR *dirp)
{
  if (dirp == _s0il_internal_dir) {
     _s0il_dir = NULL;
     return 0;
  }
  return closedir(dirp);
}

int
s0il_unlink (const char *pathname)
{
  char *path = s0il_resolve_path (pathname);
  int ret = 0;
  ret = unlink (path);
  if (path != pathname) free (path);
  return ret;
}

int
s0il_stat (const char *pathname, struct stat *statbuf)
{
  char *path = s0il_resolve_path (pathname);

  file_t *file = NULL;
  if ((file = s0il_find_file (path)))
  {
    statbuf->st_mode = file->d_type == DT_REG ? S_IFREG : S_IFDIR;
    statbuf->st_size = file->size;
    if (path != pathname) free (path);
    return 0;
  }
  else for (CtxList *iter = folders; iter; iter=iter->next)
  {
    folder_t *folder = iter->data;
    if (!strcmp (folder->path, path))
    {
      statbuf->st_mode = S_IFDIR;
      if (path != pathname) free (path);
      return 0;
    }
  }
  int ret = stat(path, statbuf);
  if (path != pathname) free (path);
  return ret;
}

int
s0il_fstat (int fd, struct stat *statbuf)
{
  return fstat(fd, statbuf);
}

//// bits implemented in terms of some of the above

int s0il_getchar (void)
{
  return s0il_fgetc(stdin);
}

ssize_t s0il_getline (char **lineptr, size_t *n, FILE *stream)
{
  *lineptr = realloc (*lineptr, 500);
  s0il_fgets(*lineptr, 500, stream);
  return strlen(*lineptr);
}
