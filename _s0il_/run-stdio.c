static int text_output = 0;
static int gfx_output  = 0;

static char *run_cwd = NULL;

char *run_getcwd(char *buf, size_t size)
{
  if (!buf){ int size = 4; if (run_cwd) size = strlen(run_cwd)+2;
             buf = malloc(size);}
  if (!run_cwd)
    strncpy(buf, "/", size-1);
  else
    strncpy(buf, run_cwd, size-1);
  return buf;
}

int   run_chdir(const char *path)
{
  // XXX need better check?
  //if (!run_access (path, R_OK)) return -1;
  if (run_cwd) free (run_cwd);

  run_cwd = malloc (strlen(path)+2);
  strcpy (run_cwd, path);
  if (run_cwd[strlen(run_cwd)-1]!='/')
  {
    run_cwd[strlen(run_cwd)+1]=0;
    run_cwd[strlen(run_cwd)]='/';
  }
  chdir(path);
  return 0;
}

static char *run_resolve_path(const char *pathname)
{
  char *path = (char*)pathname;
  if (pathname[0]!='/')
  {
     if (!run_cwd)run_cwd=strdup("/");
     path = malloc (strlen (pathname) + strlen (run_cwd) + 2);
     sprintf (path, "%s%s", run_cwd, pathname);
  }
  return path;
}



int  run_output_state (void)
{
  return text_output * 1 + gfx_output * 2;
}
void run_output_state_reset (void)
{
  text_output = 0;
  gfx_output = 0;
}

void *run_ctx_new(int width, int height, const char *backend)
{
  gfx_output  = 1;
  return ctx_new(width, height, backend);
}

void run_ctx_destroy(void *ctx)
{
  gfx_output  = 0;
  //return ctx_new(width, height, backend);
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

static DIR      *_run_internal_dir    = (void*)100;
static FILE     *_run_internal_file   = (void*)200;
static folder_t *_run_dir = NULL;
static file_t   *_run_file = NULL;

static CtxList *folders = NULL;

file_t *run_find_file(const char *path)
{
  char *parent = strdup (path);
  strrchr (parent, '/')[0]=0;
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
    return NULL;
  for (CtxList *iter = folder->files; iter; iter=iter->next)
  {
    file_t *file = iter->data;
    if (!strcmp (path, file->path))
    {
      //printf ("found match! %s\n", path);
      return file;
    }
  }
  return NULL;
}

void run_add_dir(const char *path, bool readonly)
{
  // should add it to parent but disambiguated from file
}

void run_add_file(const char *path, const char *contents, size_t size, bool readonly)
{
  char *parent = strdup (path);
  strrchr (parent, '/')[0]=0;
  folder_t *folder = NULL;
  if (size == 0)
    size = strlen (contents);
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
  file->d_type = DT_REG;
  file->read_only = readonly;
  if (readonly)
    file->data = (char*)contents;
  else
  {
    file->data = malloc(file->size);
    memcpy(file->data, contents, file->size);
  }
  folder->count++;
  ctx_list_append (&folder->files, file);
}


#define WRAP_STDOUT 1

int run_putchar (int c)
{
#if WRAP_STDOUT
  text_output = 1;
  if (c == '\n')
  {
    ctx_vt_write(NULL, '\r');
    ui_iteration(ui_host(NULL)); // doing a ui iteration
                                 // per received char is a bit much
                                 // so we only do newlines
  }
  ctx_vt_write(NULL, c);
#endif
  return putchar (c);
}

int run_fputs (const char *s, FILE *stream)
{
  if (stream == _run_internal_file) return 0;
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
    ui_iteration(ui_host(NULL));
  }
#endif
  int ret = fputs(s, stream);
  return ret;
}


int run_fputc (int c, FILE *stream)
{
  if (stream == _run_internal_file) return 0;
#if WRAP_STDOUT
  if (stream == stdout)
    return run_putchar (c);
#endif
  return fputc(c, stream);
}

ssize_t run_write(int fd, const void *buf, size_t count)
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
    ui_iteration(ui_host(NULL));
  }
#endif
  return write(fd,buf,count);
}

int run_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (stream == _run_internal_file) return 0;
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
    ui_iteration(ui_host(NULL));
  }
#endif
  return fwrite (ptr, size, nmemb, stream);
}

int run_puts (const char *s)
{
  int ret = run_fputs(s, stdout);
  run_fputc ('\n', stdout);
  ui_iteration(ui_host(NULL));
  return ret;
}

int run_fprintf (FILE *stream, const char *restrict format, ...)
{
  if (stream == _run_internal_file) return 0;
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
  run_fputs (buffer, stream);
  free (buffer);
  return ret;
}

int run_vfprintf (FILE *stream, const char *format, va_list ap)
{
  if (stream == _run_internal_file) return 0;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_copy(ap_copy, ap);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  ret = vsnprintf (buffer, needed, format, ap_copy);
  va_end (ap_copy);
  run_fputs (buffer, stream);
  free (buffer);
  return ret;
}

int run_printf (const char *restrict format, ...)
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
  run_fputs (buffer, stdout);
  free (buffer);
#if WRAP_STDOUT
  ui_iteration(ui_host(NULL)); // doing a ui iteration
#endif
  return ret;
}

static int stdin_got_data(void)
{
  int c = run_fgetc(stdin);
  if (c>=0)
  {
    run_ungetc(c, stdin);
    return 1;
  }
  return 0;
}

static char *run_gets(char* buf, size_t buflen) {
    size_t count = 0;
    // used by the shell

    while (count < buflen) {
        int c;
        if (stdin_got_data())
          c = run_fgetc(stdin);
        else
        {
          ui_iteration(ui_host(NULL));
          usleep (1000); // XXX : seems more stable with it
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
               run_fputs("\033c> ", stdout);
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
                run_fputs("\x08 ", stdout);
                run_fputc('\b', stdout); /* echo */
            }
            continue;
        //} else if (c < 32) { fprintf(stdout, "{%i}", (int)c);
        } else {
            buf[count] = c;
            count++;
        }
        run_fputc(c, stdout); /* echo */
        ui_iteration(ui_host(NULL));
    }
    run_fputc('\n', stdout);
    buf[count]=0;
    return buf;
}


char *run_fgets(char *s, int size, FILE *stream)
{
  if (stream == _run_internal_file)
  {
    int ret = 0;
    if (_run_file->pos >=  _run_file->size)
      return NULL;
    for (; _run_file->pos <  _run_file->size; _run_file->pos++)
    {
       char c = _run_file->data[_run_file->pos];
       s[ret++] = c;
       if (c == '\n')
         break;
    }
    s[ret]=0;
    return s;
  }
  if (stream == stdin)
  {
    return run_gets(s, size);
  }
  return fgets(s, size, stream);
}

int run_rename(const char *src, const char *dst)
{
  int ret = 0;
  if (!src || !dst)
    return -1;
  char *src_p = run_resolve_path(src);
  char *dst_p = run_resolve_path(dst);

  ret = rename(src_p, dst_p);
  if (src_p!=src)free (src_p);
  if (dst_p!=dst)free (dst_p);
  return ret;
}

FILE *run_fopen(const char *pathname, const char *mode)
{
  char *path = run_resolve_path (pathname);
  printf ("fopen:%s %s\n", path, mode);
  file_t *file = run_find_file (path);
  if (file)
  {
    _run_file = file;
    //_run_internal_file->_fileno = 1111;
    file->pos = 0;
    return _run_internal_file;
  }

  FILE *ret = fopen(path, mode);
  if (path != pathname) free (path);
  return ret;
}

FILE *run_fdopen(int fd, const char *mode)
{
  return fdopen(fd, mode);
}

int run_fclose(FILE *stream)
{
  if (stream == _run_internal_file)
  {
    _run_file = NULL;
    return 0;
  }
  return fclose(stream);
}

int run_ungetc(int c, FILE *stream)
{
  if (stream == _run_internal_file) return 0;
  return ungetc(c, stream);
}

int run_fgetc(FILE *stream)
{
  if (stream == _run_internal_file)
  {
    int ret = EOF;
    if (_run_file->pos < _run_file->size)
    {
      ret = _run_file->data[_run_file->pos];
      _run_file->pos++;
    }
    return ret;
  }
  ui_iteration(ui_host(NULL));
  return fgetc(stream);
}

size_t run_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (stream == _run_internal_file)
  {
    int request = size * nmemb;
    if (_run_file->pos + request > _run_file->size)
      request = _run_file->size - _run_file->pos;
    if (request <= 0)
      return 0;
    memcpy(ptr, _run_file->data + _run_file->pos, request);
    _run_file->pos+=request;
    return request;
  }
  //ui_iteration(ui_host(NULL));
  return fread(ptr,size,nmemb,stream);
}

int run_access(const char *pathname, int mode)
{
  if (run_find_file (pathname)) return F_OK;
  return access(pathname, mode);
}


int run_getc(FILE *stream)
{
  return run_fgetc(stream);
}


int run_fflush (FILE *stream)
{
  if (stream == _run_internal_file) return 0;
  ui_iteration(ui_host(NULL));
  return fflush (stream);
}

ssize_t run_read(int fildes, void *buf, size_t nbyte)
{
  ui_iteration(ui_host(NULL));
  return read(fildes,buf,nbyte);
}

// positions
int run_fsetpos(FILE *stream, fpos_t *pos)
{
#if 0
  if (stream == _run_internal_file)
  {
    _run_file->pos = *pos;
    return 0;
  }
#endif
  return fsetpos(stream, pos);
}

off_t run_lseek(int fd, off_t offset, int whence)
{
  return lseek(fd, offset, whence);
}

int run_fseek(FILE *stream, long offset, int whence)
{
  if (stream == _run_internal_file)
  {
    switch(whence)
    {
      case SEEK_SET:
        _run_file->pos = 0;
        return _run_file->pos;
      case SEEK_CUR:
        return _run_file->pos;
      case SEEK_END:
        _run_file->pos = _run_file->size;
        return _run_file->pos;
    }
  }
  return fseek(stream, offset, whence);
}

void run_rewind(FILE *stream)
{
  if (stream == _run_internal_file)
  {
    _run_file->pos = 0;
    return;
  }
  rewind(stream);
}

int run_fgetpos(FILE *s, fpos_t *pos)
{
#if 0
  if (stream == _run_internal_file)
  {
    *pos = _run_file->pos;
    return 0;
  }
#endif
  return fgetpos(s, pos);
}

long
run_ftell (FILE *stream) {
  if (stream == _run_internal_file)
    return _run_file->pos;
  return ftell(stream);
}

off_t
run_ftello (FILE *stream) {
  if (stream == _run_internal_file)
    return _run_file->pos;
  return ftello(stream);
}

// pid info

pid_t run_getpid(void)
{
  return 1;
}

pid_t run_getppid(void)
{
  return 0;
}

// fs/dir bits

DIR    *run_opendir(const char *name2)
{
  char *name = (char*)name2;
  if (name[strlen(name)-1]=='/')
  {
    name = strdup(name);
    name[strlen(name)-1]=0;
  }
  for (CtxList *iter = folders; iter; iter=iter->next)
  {
    folder_t *folder = iter->data;
    if (!strcmp (folder->path, name))
    {
      _run_dir = folder;
      _run_dir->pos = 0;
      if (name != name2)
        free (name);
      return _run_internal_dir;
    }
  }
  DIR *ret = opendir(name);
  if (name != name2)
    free (name);
  return ret;
}

struct  dirent *run_readdir(DIR *dirp)
{
  if (dirp == _run_internal_dir)
  {
     static struct dirent ent;
     if (_run_dir->pos < _run_dir->count)
     {
       int i = 0;
       file_t *file = NULL;
       for (CtxList *iter = _run_dir->files; iter; iter=iter->next, i++)
       {
         if (i == _run_dir->pos) file=iter->data;
       }
       if (!file) return NULL;
       _run_dir->pos++;
       strncpy (ent.d_name, file->d_name, sizeof (ent.d_name)-1);
       ent.d_type = file->d_type;
       return &ent;
     }
     return NULL;
  }
  return readdir(dirp);
}

int run_closedir(DIR *dirp)
{
  if (dirp == _run_internal_dir) {
     _run_dir = NULL;
     return 0;
  }
  return closedir(dirp);
}

int
run_unlink(const char *pathname)
{
  char *path = run_resolve_path (pathname);
  int ret = 0;
  ret = unlink (path);
  if (path != pathname) free (path);
  return ret;
}

int
run_stat(const char *pathname, struct stat *statbuf)
{
  char *path = run_resolve_path (pathname);

  file_t *file = NULL;
  if ((file = run_find_file (pathname)))
  {
    statbuf->st_mode = S_IFREG;
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
run_fstat(int fd, struct stat *statbuf)
{
  return fstat(fd, statbuf);
}

//// bits implemented in terms of some of the above

int run_getchar(void)
{
  return run_fgetc(stdin);
}

ssize_t run_getline(char **lineptr, size_t *n, FILE *stream)
{
  *lineptr = realloc (*lineptr, 500);
  run_fgets(*lineptr, 500, stream);
  return strlen(*lineptr);
}
