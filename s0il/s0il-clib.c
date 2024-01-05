#include "port_config.h"
#include "s0il.h"
extern void *_s0il_main_thread;

#if defined(PICO_BUILD)
#define S0IL_HAVE_FS 0
// the APIs still work but only target the ram-disk,
// this permits running in isolation in RAM in a process
#else
#define S0IL_HAVE_FS 1
#endif

#define S0IL_HAVE_SELECT 1

#if defined(S0IL_NATIVE)
#define S0IL_LIBCURL 1
#else
#endif

#if defined(S0IL_LIBCURL)
#include <curl/curl.h>
#endif

void *_s0il_thread_id(void) {
#if PICO_BUILD
  return 0;
#elif EMSCRIPTEN
  return 0;
#elif CTX_ESP
  return xTaskGetCurrentTaskHandle();
#elif S0IL_NATIVE
  return (void *)((size_t)gettid());
#else
  return 0;
#endif
}

bool s0il_is_main_thread(void) {
#if PICO_BUILD
  return 1;
#elif EMSCRIPTEN
  return 1;
#elif CTX_ESP
  return _s0il_thread_id() == _s0il_main_thread;
#else
  return gettid() == getpid();
#endif
}

static int text_output = 0;
static int gfx_output = 0;

int s0il_output_state(void) { return text_output * 1 + gfx_output * 2; }

void s0il_output_state_reset(void) {
  text_output = 0;
  gfx_output = 0;
}

void *s0il_ctx_new(int width, int height, const char *backend) {
  gfx_output = 1;
  return ctx_new(width, height, backend);
}

void s0il_ctx_destroy(void *ctx) { gfx_output = 0; }
/////////

char *s0il_getenv(const char *name) { return getenv(name); }
int s0il_setenv(const char *name, const char *value, int overwrite) {
  return setenv(name, value, overwrite);
}
int s0il_putenv(char *string) { return putenv(string); }
int s0il_unsetenv(const char *name) { return unsetenv(name); }
int s0il_clearenv(void) {
#if !defined(CTX_ESP) && !defined(PICO_BUILD)
  return clearenv();
#else
  return 0;
#endif
}

char *s0il_getcwd(char *buf, size_t size) {
  s0il_process_t *info = s0il_process();
  if (!buf) {
    int size = 4;
    if (info->cwd)
      size = strlen(info->cwd) + 2;
    buf = malloc(size);
  }
  strncpy(buf, info->cwd, size - 1);
  return buf;
}

static char *s0il_resolve_path(const char *pathname) {
  char *path = (char *)pathname;
  const char *cwd = s0il_process()->cwd;

  if (pathname[0] != '/') {
    path = malloc(strlen(pathname) + strlen(cwd) + 2);

    if (cwd[strlen(cwd) - 1] == '/')
      sprintf(path, "%s%s", cwd, pathname);
    else
      sprintf(path, "%s/%s", cwd, pathname);
  }

  if (strstr(path, "/./")) {
    if (path == pathname)
      path = strdup(path);
    do {
      char *p = strstr(path, "/./");
      if (p == path) {
        path[1] = 0;
        return path;
      }
      char *parent = p - 1;
      while (parent != path && *parent != '/')
        parent--;
      memmove(parent, p + 3, strlen(p + 2) + 1);

    } while (strstr(path, "/./"));
  }

  if (strstr(path, "/../")) {
    if (path == pathname)
      path = strdup(path);
    do {
      char *p = strstr(path, "/../");
      if (p == path) {
        path[1] = 0;
        return path;
      }
      char *parent = p - 1;
      while (parent != path && *parent != '/')
        parent--;
      memmove(parent, p + 3, strlen(p + 3) + 1);

    } while (strstr(path, "/../"));
  }

  if (strstr(path, "/..") && strstr(path, "/..")[3] == 0) {
    if (path == pathname)
      path = strdup(path);
    char *p = strstr(path, "/..");
    if (p == path) {
      path[1] = 0;
      return path;
    }
    char *parent = p - 1;
    while (parent != path && *parent != '/')
      parent--;
    memmove(parent, p + 3, strlen(p + 2) + 1);
  }
  return path;
}

int s0il_chdir(const char *path2) {
  s0il_process_t *info = s0il_process();

  char *path = s0il_resolve_path(path2);
  // XXX need better check?
  // if (!s0il_access (path, X_OK)) return -1;
  if (info->cwd)
    free(info->cwd);

  info->cwd = malloc(strlen(path) + 2);
  strcpy(info->cwd, path);

#if 0
  // append trailing / if missing
  if (info->cwd[strlen(info->cwd) - 1] != '/') {
    info->cwd[strlen(info->cwd) + 1] = 0;
    info->cwd[strlen(info->cwd)] = '/';
  }
#endif
  return 0;

#if S0IL_HAVE_FS
  chdir(path);
  if (path2 != path)
    free(path);
#endif
  return 0;
}

struct _file_t {
  char *path;
  char *d_name;
  unsigned char d_type;
  int flags;
  char *data;
  size_t size;
  size_t capacity;
  size_t pos;
};

struct _folder_t {
  char *path;
  int count;
  CtxList *files;
  int pos;
};

#define S0IL_MAX_FILES 16 // shared among processes

static DIR *_s0il_internal_dir = (void *)256;

static FILE *_s0il_internal_file = (void *)512;

// static folder_t *_s0il_dir = NULL;
static file_t *_s0il_file[S0IL_MAX_FILES] = {
    NULL,
};

static int s0il_fileno(FILE *stream) {
  if ((char *)stream >= (char *)_s0il_internal_file &&
      (char *)stream <= ((char *)_s0il_internal_file) + S0IL_MAX_FILES)
    return ((char *)stream - (char *)_s0il_internal_file);
  return -1;
}

static bool s0il_stream_is_internal(FILE *stream) {
  if ((char *)stream >= (char *)_s0il_internal_file &&
      (char *)stream <= ((char *)_s0il_internal_file) + S0IL_MAX_FILES)
    return true;
  return false;
}

static CtxList *folders = NULL;

file_t *s0il_find_file(const char *path) {
  char *parent = strdup(path);
  strrchr(parent, '/')[0] = 0;
  if (parent[0] == 0) {
    parent[0] = '/';
    parent[1] = 0;
  }
  folder_t *folder = NULL;

  for (CtxList *iter = folders; iter; iter = iter->next) {
    folder = iter->data;
    if (!strcmp(folder->path, parent)) {
      break;
    }
  }
  free(parent);
  if (!folder) {
    return NULL;
  }

  for (CtxList *iter = folder->files; iter; iter = iter->next) {
    file_t *file = iter->data;
    if (!strcmp(path, file->path)) {
      return file;
    }
  }
  return NULL;
}

static int s0il_unlink_internal(const char *path) {
  file_t *file = s0il_find_file(path);
  if (!file)
    return -2;
  char *parent = strdup(path);
  strrchr(parent, '/')[0] = 0;
  folder_t *folder = NULL;

  if (parent[0] == 0) {
    parent[0] = '/';
    parent[1] = 0;
  }

  // printf ("adding %s to %s dir?%i\n", path, parent, is_dir);

  for (CtxList *iter = folders; iter; iter = iter->next) {
    folder = iter->data;
    if (!strcmp(folder->path, parent)) {
      break;
    }
    folder = NULL;
  }
  if (!folder) {
    return 1;
  } else {
    free(parent);
  }

  ctx_list_remove(&folder->files, file);

  if ((file->flags & S0IL_READONLY) == 0) {
    free(file->d_name);
    free(file->path);
    free(file->data);
  }
  free(file);

  return 0;
}

void *s0il_add_file(const char *path, const char *contents, size_t size,
                    s0il_file_flag flags) {
  bool readonly = ((flags & S0IL_READONLY) != 0);
  bool is_dir = ((flags & S0IL_DIR) != 0);

  char *parent = strdup(path);
  strrchr(parent, '/')[0] = 0;
  folder_t *folder = NULL;
  if (size == 0 && !is_dir)
    size = strlen(contents);

  if (parent[0] == 0) {
    parent[0] = '/';
    parent[1] = 0;
  }

  // printf ("adding %s to %s dir?%i\n", path, parent, is_dir);

  for (CtxList *iter = folders; iter; iter = iter->next) {
    folder = iter->data;
    if (!strcmp(folder->path, parent)) {
      break;
    }
    folder = NULL;
  }
  if (!folder) {
    folder = calloc(sizeof(folder_t), 1);
    folder->path = parent;
    ctx_list_append(&folders, folder);
  } else {
    free(parent);
  }

  file_t *file = calloc(sizeof(file_t), 1);
  file->path = (char *)(readonly ? path : strdup(path));
  file->d_name =
      readonly ? strrchr(path, '/') + 1 : strdup(strrchr(path, '/') + 1);
  file->size = size;
  file->flags = flags;
  if (is_dir) {
    file->d_type = DT_DIR;
  } else {
    file->d_type = DT_REG;
    if (readonly) {
      file->data = (char *)contents;
      file->capacity = 0;
    } else {
      file->data = malloc(file->size);
      file->capacity = file->size;
      memcpy(file->data, contents, file->size);
    }
  }
  if (folder) {
    folder->count++;
    ctx_list_append(&folder->files, file);
  }
  return file;
}

void s0il_redirect_io(FILE *in_stream, FILE *out_stream) {
  s0il_process_t *pi = s0il_process();
  if (!pi)
    return;
  pi->redir_stdin = in_stream;
  pi->redir_stdout = out_stream;
}

int s0il_putchar(int c) {
  if (s0il_process()->redir_stdout)
    return fputc(c, s0il_process()->redir_stdout);

  text_output = 1;
  if (c == '\n') {
    ctx_vt_write(NULL, '\r');
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL)); // doing a ui iteration
                                   // per received char is a bit much
                                   // so we only do newlines
  }
  ctx_vt_write(NULL, c);
  return putchar(c);
}

int s0il_fputc(int c, FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    if (file->pos + 1 >= file->capacity) {
      file->capacity += 256;
      file->data = realloc(file->data, file->capacity);
    }
    file->data[file->pos] = c;
    file->pos++;
    if (file->pos > file->size)
      file->size = file->pos;
    return c;
  }
  if (stream == stdout) {
    if (s0il_process()->redir_stdout)
      return s0il_fputc(c, s0il_process()->redir_stdout);
    return s0il_putchar(c);
  }
  if (stream == stderr) {
    if (s0il_process()->redir_stderr)
      return s0il_fputc(c, s0il_process()->redir_stderr);
    return s0il_putchar(c);
  }
  return fputc(c, stream);
}

int s0il_fputs(const char *s, FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    int count = 0;
    for (int i = 0; s[i]; i++) {
      if (s0il_fputc(s[i], stream) != EOF)
        count++;
    }
    return count;
  }
  if (stream == stdout || stream == stderr) {
    if (stream == stdout && s0il_process()->redir_stdout)
      return s0il_fputs(s, s0il_process()->redir_stdout);
    text_output = 1;
    for (int i = 0; s[i]; i++) {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL));
  }
  int ret = fputs(s, stream);
  return ret;
}

size_t s0il_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    uint8_t *s = (uint8_t *)ptr;
    int count = 0;
    for (size_t i = 0; i < size * nmemb; i++) {
      // if (s[i] == '\n')
      //   ctx_vt_write(NULL, '\r');
      count += (s0il_fputc(s[i], stream) != EOF);
    }
    return count;
  }
  if (stream == stdout || stream == stderr) {
    if (stream == stdout && s0il_process()->redir_stdout)
      return s0il_fwrite(ptr, size, nmemb, s0il_process()->redir_stdout);
    if (stream == stderr && s0il_process()->redir_stderr)
      return s0il_fwrite(ptr, size, nmemb, s0il_process()->redir_stderr);
    text_output = 1;
    uint8_t *s = (uint8_t *)ptr;
    for (size_t i = 0; i < size * nmemb; i++) {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL));
  }
  return fwrite(ptr, size, nmemb, stream);
}

ssize_t s0il_write(int fd, const void *buf, size_t count) {

  if (s0il_stream_is_internal((FILE *)(size_t)fd)) {
    return s0il_fwrite(buf, 1, count, (FILE *)(size_t)fd);
  }

  if (fd == 1 || fd == 2) {
    text_output = 1;
    uint8_t *s = (uint8_t *)buf;
    for (size_t i = 0; i < count; i++) {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL));
  }
  return write(fd, buf, count);
}

int s0il_puts(const char *s) {
  if (s0il_process()->redir_stdout)
    return fputs(s, s0il_process()->redir_stdout);
  int ret = s0il_fputs(s, stdout);
  s0il_fputc('\n', stdout);
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return ret;
}

int s0il_fprintf(FILE *stream, const char *restrict format, ...) {
  va_list ap;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_start(ap, format);
  va_copy(ap_copy, ap);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end(ap);
  ret = vsnprintf(buffer, needed, format, ap_copy);
  va_end(ap_copy);
  s0il_fputs(buffer, stream);
  free(buffer);
  return ret;
}

int s0il_vfprintf(FILE *stream, const char *format, va_list ap) {
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_copy(ap_copy, ap);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  ret = vsnprintf(buffer, needed, format, ap_copy);
  va_end(ap_copy);
  s0il_fputs(buffer, stream);
  free(buffer);
  return ret;
}

int s0il_printf(const char *restrict format, ...) {
  if (!format)
    return -1;
  va_list ap;
  va_list ap_copy;
  size_t needed;
  char *buffer;
  int ret;
  va_start(ap, format);
  va_copy(ap_copy, ap);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end(ap);
  ret = vsnprintf(buffer, needed, format, ap_copy);
  va_end(ap_copy);
  s0il_fputs(buffer, stdout);
  free(buffer);
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL)); // doing a ui iteration
  return ret;
}

int s0il_rename(const char *src, const char *dst) {
  int ret = 0;
  if (!src || !dst)
    return -1;
  char *src_p = s0il_resolve_path(src);
  char *dst_p = s0il_resolve_path(dst);

  ret = rename(src_p, dst_p);
  if (src_p != src)
    free(src_p);
  if (dst_p != dst)
    free(dst_p);
  return ret;
}

FILE *s0il_freopen(const char *pathname, const char *mode, FILE *stream) {
  if (stream == stdin) {
    s0il_process()->redir_stdin = s0il_fopen(pathname, mode);
    return stdin;
  } else if (stream == stdout) {
    s0il_process()->redir_stdout = s0il_fopen(pathname, mode);
    return stdout;
  } else if (stream == stderr) {
    s0il_process()->redir_stderr = s0il_fopen(pathname, mode);
    return stderr;
  }
  return freopen(pathname, mode, stream);
}

#define S0IL_FETCH_URLS

#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#if !defined(MAX)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MAX_HTTP_OUTPUT_BUFFER 4096

#if defined(CTX_ESP)
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
#define TAG "clib"
  static char *output_buffer; // Buffer to store response of http request from
                              // event handler
  static int output_len;      // Stores number of bytes read
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    fprintf(stderr, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    // fprintf(stderr, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    // fprintf(stderr, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    // fprintf(stderr, "HTTP_EVENT_ON_HEADER, key=%s, value=%s",
    // evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    // fprintf(stderr, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    //  Clean the buffer in case of a new request
    if (output_len == 0 && evt->user_data) {
      // we are just starting to copy the output data into the use
      // memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
    }
    /*
     *  Check for chunked encoding is added as the URL for chunked encoding used
     * in this example returns binary data. However, event handler can also be
     * used in case chunked encoding is used.
     */
    if (!esp_http_client_is_chunked_response(evt->client)) {
      // If user_data buffer is configured, copy the response into the buffer
      int copy_len = 0;
      if (evt->user_data) {
        // The last byte in evt->user_data is kept for the NULL character in
        // case of out-of-bound access.
        // copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
        if (evt->data_len) {
          s0il_fwrite(evt->data, 1, evt->data_len, evt->user_data);
        }
      } else {
        int content_len = esp_http_client_get_content_length(evt->client);
        if (output_buffer == NULL) {
          // We initialize output_buffer with 0 because it is used by strlen()
          // and similar functions therefore should be null terminated.
          output_buffer = (char *)calloc(content_len + 1, sizeof(char));
          output_len = 0;
          if (output_buffer == NULL) {
            fprintf(stderr, "Failed to allocate memory for output buffer");
            return ESP_FAIL;
          }
        }
        copy_len = MIN(evt->data_len, (content_len - output_len));
        if (copy_len) {
          memcpy(output_buffer + output_len, evt->data, copy_len);
        }
      }
      output_len += copy_len;
    }

    break;
  case HTTP_EVENT_ON_FINISH:
    // fprintf(stderr, "HTTP_EVENT_ON_FINISH");
    if (output_buffer != NULL) {
      // Response is accumulated in output_buffer. Uncomment the below line to
      // print the accumulated response ESP_LOG_BUFFER_HEX(TAG, output_buffer,
      // output_len);
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  case HTTP_EVENT_DISCONNECTED:
    // fprintf(stderr, "HTTP_EVENT_DISCONNECTED");
    int mbedtls_err = 0;
    esp_err_t err = esp_tls_get_and_clear_last_error(
        (esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
    if (err != 0) {
      fprintf(stderr, "Last esp error code: 0x%x", err);
      fprintf(stderr, "Last mbedtls failure: 0x%x", mbedtls_err);
    }
    if (output_buffer != NULL) {
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  case HTTP_EVENT_REDIRECT:
    // fprintf(stderr, "HTTP_EVENT_REDIRECT");
    esp_http_client_set_header(evt->client, "From", "user@example.com");
    esp_http_client_set_header(evt->client, "Accept", "text/html");
    esp_http_client_set_redirection(evt->client);
    break;
  }
  return ESP_OK;
}
#endif

typedef struct _Sha1 CtxSha1;
CtxSHA1 *ctx_sha1_new(void);
void ctx_sha1_free(CtxSHA1 *sha1);
int ctx_sha1_process(CtxSHA1 *sha1, const unsigned char *msg,
                     unsigned long len);
int ctx_sha1_done(CtxSHA1 *sha1, unsigned char *out);

FILE *s0il_fopen(const char *pathname, const char *mode) {
#ifdef S0IL_FETCH_URLS
  char cached_path[128] = "/tmp/..";
  if (strchr(pathname, ':') && strchr(pathname, ':') - pathname <= 5) {
    uint8_t digest[64];
    CtxSHA1 *sha1 = ctx_sha1_new();
    ctx_sha1_process(sha1, (uint8_t *)pathname, strlen(pathname));
    ctx_sha1_done(sha1, digest);

    sprintf(cached_path, "/tmp/%s", digest);
    for (int i = 0; i < 40; i++) {
      char hex[16] = "0123456789ABCDEF";
      if (i & 1)
        cached_path[5 + i] = hex[(digest[i / 2]) & 0xf];
      else
        cached_path[5 + i] = hex[(digest[i / 2]) >> 4];
    }

    if (access(cached_path, R_OK) == F_OK && !strchr(mode, '!')) {
      // printf("using cached %s\n", cached_path);
    } else {
#if defined(S0IL_LIBCURL)
      FILE *cached_file = s0il_fopen(cached_path, "wb");
      if (cached_file) {
        CURL *curl = curl_easy_init();
        // CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, pathname);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s0il_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, cached_file);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "s0il/0.0");
        // res =
        curl_easy_perform(curl);
        s0il_fclose(cached_file);
      }
#elif defined(CTX_ESP)
      FILE *cached_file = s0il_fopen(cached_path, "wb");
      esp_http_client_config_t config = {
          .url = pathname,
          .event_handler = _http_event_handler,
          .user_data = cached_file,
          //.crt_bundle_attach = esp_crt_bundle_attach,
          // XXX : it fails to authenticate lets encrypt..
      };
      esp_http_client_handle_t client = esp_http_client_init(&config);
      esp_err_t err = esp_http_client_perform(client);

      if (err == ESP_OK) {
        // fprintf(stderr, "HTTPS Status = %d, content_length = %"PRId64,
        //         esp_http_client_get_status_code(client),
        //         esp_http_client_get_content_length(client));
      } else {
        fprintf(stderr, "Error perform http request %s", esp_err_to_name(err));
      }
      esp_http_client_cleanup(client);

      s0il_fclose(cached_file);
#else
      s0il_printf("fetch to %s\n", cached_path);
#endif
    }

    pathname = cached_path;
  }
#endif

  char *path = s0il_resolve_path(pathname);
  file_t *file = s0il_find_file(path);
  if (file) {
    int fileno = 0;
    for (fileno = 0; fileno < S0IL_MAX_FILES; fileno++)
      if (_s0il_file[fileno] == NULL)
        break;
    _s0il_file[fileno] = file;
    file->pos = 0;
    if (strchr(mode, 'w')) {
      if (file->flags & S0IL_READONLY) {
        char *old_data = file->data;
        file->data = malloc(file->size);
        file->capacity = file->size;
        memcpy(file->data, old_data, file->size);
        file->flags &= ~S0IL_READONLY;
      }
      if (!strchr(mode, '+')) {
        file->pos = 0;
        file->size = 0;
      }
    }
    if (path != pathname)
      free(path);
    return (FILE *)(((char *)_s0il_internal_file) + fileno);
  }

  if (strchr(mode, 'w')) {
    char *parent = strdup(path);
    strrchr(parent, '/')[0] = 0;
    if (parent[0] == 0) {
      parent[0] = '/';
      parent[1] = 0;
    }
    folder_t *folder = NULL;

    for (CtxList *iter = folders; iter; iter = iter->next) {
      folder = iter->data;
      if (!strcmp(folder->path, parent)) {
        break;
      }
      folder = NULL;
    }
    free(parent);
    if (folder) {
      char *t = "";
      file_t *file = s0il_add_file(path, t, 0, 0);
      int fileno = 0;
      for (fileno = 0; fileno < S0IL_MAX_FILES; fileno++)
        if (_s0il_file[fileno] == NULL)
          break;
      _s0il_file[fileno] = file;
      file->pos = 0;
      if (path != pathname)
        free(path);
      return (FILE *)(((char *)_s0il_internal_file) + fileno);
    }
  }

  FILE *ret = fopen(path, mode);
  if (path != pathname)
    free(path);
  return ret;
}

int s0il_open(const char *pathname, int flags) {
  char *path = s0il_resolve_path(pathname);
  file_t *file = s0il_find_file(path);
  if (file) {
    int fileno = 0;
    for (fileno = 0; fileno < S0IL_MAX_FILES; fileno++)
      if (_s0il_file[fileno] == NULL)
        break;
    // XXX : handle too many open files
    _s0il_file[fileno] = file;
    file->pos = 0;
    return fileno;
  }
#if S0IL_HAVE_FS
  return open(pathname, flags);
#else
  return -1;
#endif
}

int s0il_close(int fd) {
  if (s0il_stream_is_internal((FILE *)(size_t)fd)) {
    _s0il_file[s0il_fileno((FILE *)(size_t)fd)] = NULL;
    return 0;
  }
#if S0IL_HAVE_FS
  return close(fd);
#else
  return -1;
#endif
}

FILE *s0il_fdopen(int fd, const char *mode) {
  if (fd == STDIN_FILENO)
    return stdin;
  if (fd == STDOUT_FILENO)
    return stdout;
  if (fd == STDERR_FILENO)
    return stderr;
  if (s0il_stream_is_internal((FILE *)(size_t)fd))
    return (FILE *)(size_t)fd;
  return fdopen(fd, mode);
}

int s0il_fclose(FILE *stream) {
  if (stream == stdin || stream == stdout || stream == stderr)
    return 0;
  if (s0il_stream_is_internal(stream)) {
    _s0il_file[s0il_fileno(stream)] = NULL;
    return 0;
  }
  return fclose(stream);
}

static int s0il_got_data(FILE *stream) {
  if (stream == NULL)
    stream = stdin;
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;

  if (stream == stdin) {
    int gotdata = ctx_vt_has_data(NULL);
    if (gotdata)
      return 1;
#ifndef EMSCRIPTEN
    int c = s0il_fgetc(stdin);
    if (c >= 0) {
      s0il_ungetc(c, stdin);
      return 1;
    }
#endif
    return 0;
  }

  {
    int c = s0il_fgetc(stream);
    if (c >= 0) {
      s0il_ungetc(c, stream);
      return 1;
    }
    return 0;
  }
}

static CtxList *commandline_history = NULL;

static char *s0il_gets(char *buf, size_t buflen) {
  FILE *stream = stdin;
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  size_t count = 0;

  size_t cursor_pos = 0;

  int in_esc = 0;
  char keybuf[8] = "";
  int keylen = 0;

  int history_pos = -1;

  if (keybuf[0]) {
  };

  while (count < buflen) {
    int c;
    if (s0il_got_data(stream))
      c = s0il_fgetc(stream);
    else {
      if (s0il_is_main_thread()) {
        ui_iteration(ui_host(NULL));
#ifndef EMSCRIPTEN
        usleep(1000); // XXX : seems more stable with it
#endif
        if (ctx_has_quit(ctx_host()))
          return NULL;
      }
      continue;
    }
    if (count == 0) {
      switch (c) {
      case -1:
      case 4: // ctrl-d - exit
        buf[count] = 0;
        return NULL;
      case 12: // ctrl-l - clear
        s0il_fputs("\033c\033[?30l> ", stdout);
        continue;
      }
    }
    if (c == '\t')
      continue;

    if (in_esc) {
      switch (c) {
      case 'A':
      case 'B':
        if (c == 'A')
          history_pos++;
        else
          history_pos--;
        if (history_pos < -1)
          history_pos = -1;
        else if (history_pos >= ctx_list_length(commandline_history))
          history_pos = -1;
        {
          const char *cmd = ctx_list_nth_data(commandline_history, history_pos);
          int len = ctx_utf8_strlen(buf);
          s0il_printf("\e[%iC", len - cursor_pos + 1);
          cursor_pos = len;

          int i = 0;
          for (i = 0; i <= len - 4; i += 4) {
            s0il_printf("\b\b\b\b    \b\b\b\b");
          }
          for (; i <= len; i++) {
            s0il_printf("\b \b");
          }
          count = 0;
          buf[count] = 0;

          if (cmd) {
            strncpy(buf, cmd, buflen - 1);
            s0il_printf("%s", buf);
            count = strlen(buf);
            cursor_pos = ctx_utf8_strlen(buf);
          }
        }

        in_esc = 0;
        break;
      case 'D': // left
        if (cursor_pos > 0) {
          s0il_fputs("\e[D", stdout);
          cursor_pos--;
        }
        in_esc = 0;
        break;
      case 'C': // right
        if ((int)cursor_pos < ctx_utf8_strlen(buf)) {
          s0il_fputs("\e[C", stdout);
          cursor_pos++;
        }
        in_esc = 0;
        break;
      default:
        if (keylen < 4) {
          // printf("{%i %c}", c, c>32?c:'_');
          keybuf[keylen++] = c;
          keybuf[keylen] = 0;
        } else
          in_esc = 0;
      }
    } else {
      if (c == 1) { // control-a
        if (cursor_pos > 0) {
          s0il_printf("\e[%iD", cursor_pos);
          cursor_pos = 0;
        }
        continue;
      } else if (c == 5) { // control-e
        s0il_printf("\e[%iC", ctx_utf8_strlen(buf) - cursor_pos);
        cursor_pos = ctx_utf8_strlen(buf);
        continue;
      } else if (c == '\n') {
        buf[count] = c;
        count++;
        break;
      } else
        switch (c) {
        case '\b':
        case '\177': // backspace
          if (count > 0 && cursor_pos > 0) {
            if (count == cursor_pos) {
              buf[count - 1] = 0;
              cursor_pos--;
              count--;
              s0il_fputs("\b \b", stdout);
            } else {
              memmove(&buf[cursor_pos] - 1, &buf[cursor_pos],
                      strlen(&buf[cursor_pos]) + 1);
              cursor_pos--;
              count--;
              s0il_fputs("\e[D", stdout);
              for (int i = cursor_pos; buf[i]; i++)
                s0il_fputc(buf[i], stdout);
              s0il_fputc(' ', stdout);

              s0il_printf("\e[%iD", count - cursor_pos + 1);
              continue;
            }
          }

          continue;
        case 27:
          in_esc = 1;
          keylen = 0;
          keybuf[keylen++] = c;
          keybuf[keylen] = 0;
          continue;
        default:
          if (count == cursor_pos) {
            buf[count] = c;
            cursor_pos++;
            count++;
          } else {
            memmove(&buf[cursor_pos] + 1, &buf[cursor_pos],
                    strlen(&buf[cursor_pos]) + 1);
            buf[cursor_pos] = c;
            cursor_pos++;
            count++;
            for (int i = cursor_pos - 1; buf[i]; i++)
              s0il_fputc(buf[i], stdout);
            s0il_printf("\e[%iD", count - cursor_pos);
            continue;
          }
        }
      s0il_fputc(c, stdout); /* echo */
    }
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL));
  }
  s0il_fputc('\n', stdout);
  buf[count] = 0;

  if (count &&
      (commandline_history == NULL || strcmp(commandline_history->data, buf))) {
    char *copy = strdup(buf);
    copy[count - 1] = 0;
    ctx_list_prepend(&commandline_history, copy);
  }
  history_pos = -1;

  return buf;
}

int s0il_fgetc(FILE *stream) {
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  if (stream == stdin) {

    if (ctx_vt_has_data(NULL)) {
      int c = ctx_vt_read(NULL);
      if (c == '\r')
        c = '\n';
      return c;
    }
  }
  if (s0il_stream_is_internal(stream)) {
    int ret = EOF;
    file_t *file = _s0il_file[s0il_fileno(stream)];
    if (file->pos < file->size) {
      ret = file->data[file->pos];
      file->pos++;
    }
    return ret;
  }
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return fgetc(stream);
}

char *s0il_fgets(char *s, int size, FILE *stream) {
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    int ret = 0;
    if (file->pos >= file->size)
      return NULL;
    for (; file->pos < file->size && ret < size; file->pos++) {
      char c = file->data[file->pos];
      s[ret++] = c;
      if (c == '\n')
        break;
    }
    s[ret] = 0;
    return s;
  }
  if (stream == stdin) {
    return s0il_gets(s, size);
  }
  return fgets(s, size, stream);
}

int s0il_ungetc(int c,
                FILE *stream) { // TODO : unget to ctx|term layer insteead
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  if (s0il_stream_is_internal(stream))
    return 0;
  return ungetc(c, stream);
}

size_t s0il_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  if (stream == stdin && ctx_vt_has_data(NULL)) {
    char *dst = ptr;
    int read = 0;
    for (unsigned i = 0; i < size * nmemb; i++) {
      if (ctx_vt_has_data(NULL)) {
        dst[i] = ctx_vt_read(NULL);
        read++;
      }
    }
    // XXX : this only works well when we can satisfy the reads..
    return read / size;
  }

  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    int request = size * nmemb;
    if (file->pos + request > file->size)
      request = file->size - file->pos;
    if (request <= 0)
      return 0;
    memcpy(ptr, file->data + file->pos, request);
    file->pos += request;
    return request;
  }
  // ui_iteration(ui_host(NULL));
  return fread(ptr, size, nmemb, stream);
}

int s0il_getc(FILE *stream) { return s0il_fgetc(stream); }

ssize_t s0il_read(int fildes, void *buf, size_t nbyte) {
  if (s0il_stream_is_internal((FILE *)(size_t)fildes)) {
    return s0il_fread(buf, 1, nbyte, (FILE *)(size_t)fildes);
  }
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  if (fildes == 0 && ctx_vt_has_data(NULL)) {
    return s0il_fread(buf, 1, nbyte, stdin);
  }
  return read(fildes, buf, nbyte);
}

int s0il_access(const char *pathname, int mode) {
  if (s0il_find_file(pathname))
    return F_OK;
  return access(pathname, mode);
}

int s0il_fflush(FILE *stream) {
  if (stream == stdin && s0il_process()->redir_stdin)
    stream = s0il_process()->redir_stdin;
  if (stream == stdout && s0il_process()->redir_stdout)
    stream = s0il_process()->redir_stdout;
  if (stream == stderr && s0il_process()->redir_stderr)
    stream = s0il_process()->redir_stderr;
  if (s0il_stream_is_internal(stream))
    return 0;
  if (s0il_is_main_thread())
    ui_iteration(ui_host(NULL));
  return fflush(stream);
}

// positions
int s0il_fsetpos(FILE *stream, fpos_t *pos) {
#if 0 // TODO
  if (s0il_stream_is_internal (stream))
  {
    _s0il_file->pos = *pos;
    return 0;
  }
#endif
  return fsetpos(stream, pos);
}

off_t s0il_lseek(int fd, off_t offset, int whence) {
  return lseek(fd, offset, whence);
}

int s0il_fseek(FILE *stream, long offset, int whence) {
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    switch (whence) {
    case SEEK_SET:
      file->pos = 0;
      return file->pos;
    case SEEK_CUR:
      return file->pos;
    case SEEK_END:
      file->pos = file->size;
      return file->pos;
    }
  }
  return fseek(stream, offset, whence);
}

void s0il_rewind(FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    file->pos = 0;
    return;
  }
  rewind(stream);
}

int s0il_fgetpos(FILE *s, fpos_t *pos) { // TODO
#if 0
  if (s0il_stream_is_internal (stream))
  {
    *pos = _s0il_file->pos;
    return 0;
  }
#endif
  return fgetpos(s, pos);
}

long s0il_telldir(DIR *dir) {
  if (dir == _s0il_internal_dir)
    return s0il_process()->dir->pos;
#if S0IL_HAVE_FS
  return telldir(dir);
#else
  return 0;
#endif
}

long s0il_ftell(FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    return file->pos;
  }
#if S0IL_HAVE_FS
  return ftell(stream);
#else
  return 0;
#endif
}

off_t s0il_ftello(FILE *stream) {
  if (s0il_stream_is_internal(stream)) {
    file_t *file = _s0il_file[s0il_fileno(stream)];
    return file->pos;
  }
  return ftello(stream);
}

pid_t s0il_getpid(void) { return s0il_process()->pid; }

pid_t s0il_getppid(void) { return s0il_process()->ppid; }

DIR *s0il_opendir(const char *name2) {
  char *name = (char *)name2;
  if (name[strlen(name) - 1] == '/') {
    name = strdup(name);
    name[strlen(name) - 1] = 0;
  }
  if (name[1] == 0) {
    name[0] = '/';
    name[1] = 0;
  }
  for (CtxList *iter = folders; iter; iter = iter->next) {
    folder_t *folder = iter->data;
    if (!strcmp(folder->path, name)) {
      s0il_process()->dir = folder;
      folder->pos = 0;
      if (name != name2)
        free(name);
      return _s0il_internal_dir;
    }
  }
#if S0IL_HAVE_FS
  DIR *ret = opendir(name);
  if (name != name2)
    free(name);
  return ret;
#else
  return NULL;
#endif
}

struct dirent *s0il_readdir(DIR *dirp) {
  if (dirp == _s0il_internal_dir) {
    static struct dirent ent;
    if (s0il_process()->dir->pos < s0il_process()->dir->count) {
      int i = 0;
      file_t *file = NULL;
      for (CtxList *iter = s0il_process()->dir->files; iter;
           iter = iter->next, i++) {
        if (i == s0il_process()->dir->pos)
          file = iter->data;
      }
      if (!file)
        return NULL;
      s0il_process()->dir->pos++;
      strncpy(ent.d_name, file->d_name, sizeof(ent.d_name) - 1);
      ent.d_type = file->d_type;
      return &ent;
    }
    return NULL;
  }
#if S0IL_HAVE_FS
  return readdir(dirp);
#else
  return NULL;
#endif
}

int s0il_closedir(DIR *dirp) {
  if (dirp == _s0il_internal_dir) {
    s0il_process()->dir = NULL;
    return 0;
  }
#if S0IL_HAVE_FS
  return closedir(dirp);
#else
  return 0;
#endif
}

int s0il_unlink(const char *pathname) {
  char *path = s0il_resolve_path(pathname);
  int ret = 0;
  file_t *file = NULL;
  if ((file = s0il_find_file(path))) {
    return s0il_unlink_internal(path);
  }
#if S0IL_HAVE_FS
  ret = unlink(path);
#endif
  if (path != pathname)
    free(path);
  return ret;
}

int s0il_remove(const char *pathname) {
  // TODO : handle dirs
  return s0il_unlink(pathname);
}

int s0il_stat(const char *pathname, struct stat *statbuf) {
  char *path = s0il_resolve_path(pathname);

  file_t *file = NULL;
  if ((file = s0il_find_file(path))) {
    statbuf->st_mode = file->d_type == DT_REG ? S_IFREG : S_IFDIR;
    statbuf->st_size = file->size;
    if (path != pathname)
      free(path);
    return 0;
  } else
    for (CtxList *iter = folders; iter; iter = iter->next) {
      folder_t *folder = iter->data;
      if (!strcmp(folder->path, path)) {
        statbuf->st_mode = S_IFDIR;
        if (path != pathname)
          free(path);
        return 0;
      }
    }
#if S0IL_HAVE_FS
  int ret = stat(path, statbuf);
  if (path != pathname)
    free(path);
  return ret;
#else
  return -1;
#endif
}

int s0il_fstat(int fd, struct stat *statbuf) {
#if S0IL_HAVE_FS
  return fstat(fd, statbuf);
#else
  return 0;
#endif
}

//// bits implemented in terms of some of the above

int s0il_getchar(void) { return s0il_fgetc(stdin); }

ssize_t s0il_getline(char **lineptr, size_t *n, FILE *stream) {
  *lineptr = realloc(*lineptr, 500);
  void *ret = s0il_fgets(*lineptr, 500, stream);
  if (!ret)
    return -1;
  return strlen(*lineptr);
}

int s0il_exit(int retval) {
  while ((s0il_process()->atexits)) {
    void (*function)(void) = s0il_process()->atexits->data;
    function();
    ctx_list_remove(&(s0il_process()->atexits), function);
  }
  if (s0il_thread_no() > 0) {
    // XXX : we should not do this for nested exec on these thread either,.
    //       we should keep track of stack depth
    // XXX : this gets called for nested mains as well!
#if CTX_ESP
    vTaskDelete(NULL);
    // store ret-val in pid_info?
#elif defined(PICO_BUILD)
#else
    pthread_exit((void *)(ssize_t)(retval));
#endif
  }
  return retval;
}

int s0il_select(int nfds, fd_set *read_fds, fd_set *write_fds,
                fd_set *except_fds, struct timeval *timeout) {
  if (nfds == 1 && read_fds && FD_ISSET(0, read_fds)) {
    if (s0il_is_main_thread())
      ui_iteration(ui_host(NULL)); // doing a ui iteration
    // workaround for missing select
    int has_data = s0il_got_data(stdin);
    if (write_fds)
      FD_ZERO(write_fds);
    if (except_fds)
      FD_ZERO(except_fds);
    if (has_data) {
      return 1;
    }
    if (read_fds)
      FD_ZERO(read_fds);
    return 0;
  }
#if S0IL_HAVE_SELECT
  // printf("select nfds: %i\n", nfds);
  return select(nfds, read_fds, write_fds, except_fds, timeout);
#else
  return 0;
#endif
}

int s0il_glob(const char *pattern, int flags, int (*errfunc)(char *, int),
              void *glob_buf) {
  // XXX : nneds to be implemented for wildcard expansion in cmdline parser
  return 0;
}

char *s0il_realpath(const char *path, char *resolved_path) {
  // TODO
#if defined(PICO_BUILD)
  if (!resolved_path)
    return strdup(path);
  strcpy(resolved_path, path);
  return resolved_path;
#else
  return realpath(path, resolved_path);
#endif
}

int s0il_rmdir(const char *path) {
  // TODO : internal
#if S0IL_HAVE_FS
  return rmdir(path);
#else
  return 0;
#endif
}

int s0il_fsync(int fd) {
  s0il_system("sync");
#if S0IL_HAVE_FS
  return fsync(fd);
#else
  return 0;
#endif
}

int s0il_ftruncate(int fd, int length) {
#if S0IL_HAVE_FS
  return ftruncate(fd, length);
#else
  return 0;
#endif
}

int s0il_mkdir(const char *pathname, int mode) {
// TODO: internal
#if S0IL_HAVE_FS
  return mkdir(pathname, mode);
#else
  return 0;
#endif
}

int s0il_truncate(const char *path, int length) {
// TODO : internal
#if S0IL_HAVE_FS
  return truncate(path, length);
#else
  return 0;
#endif
}

void *s0il_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void *s0il_malloc(size_t size) { return s0il_realloc(NULL, size); }

void s0il_free(void *ptr) {
  if (!ptr)
    return;
  s0il_realloc(ptr, 0);
}

void *s0il_calloc(size_t nmemb, size_t size) {
  char *ret = s0il_malloc(nmemb * size);
  if (ret)
    memset(ret, 0, nmemb * size);
  return ret;
}

char *s0il_strdup(const char *s) {
  size_t len = strlen(s);
  char *ret = s0il_malloc(len + 1);
  if (ret)
    memcpy(ret, s, len + 1);
  return ret;
}

char *s0il_strndup(const char *s, size_t n) {
  size_t len = strlen(s);
  if (len < n)
    n = len;
  char *ret = s0il_malloc(n + 1);
  if (ret) {
    memcpy(ret, s, n + 1);
    ret[n] = 0;
  }
  return ret;
}

int s0il_atexit(void (*function)(void)) {
  ctx_list_append(&(s0il_process()->atexits), function);
  return 0;
}

void s0il_signal(int sig, void (*func)(int)) {
#if S0IL_NATIVE
  signal(sig, func);
#else
#endif
}

int s0il_kill(int pid, int sig) {
  // ust tkill ?
  return kill(pid, sig);
}

int s0il_pause(void) { return pause(); }

int s0il_sigaction(int signum, void *act, void *oldact) { return 0; }

int s0il_gethostname(char *name, size_t len) {
  strncpy(name, "s0il", len);
  return 0;
}

int s0il_getuid(void) {
  printf("%s NYI", __FUNCTION__);
  return 1;
}

pid_t s0il_waitpid(pid_t pid, int *status, int options) {
  printf("%s NYI", __FUNCTION__);
  return 0;
}

pid_t s0il_wait(int *stat_loc) {
  printf("%s NYI", __FUNCTION__);
  return 0;
}

int s0il_ioctl(int fd, unsigned long request, void *arg) {
  return ioctl(fd, request, arg);
}

#if 0
int s0il_pipe(int pipefd[2]){
  printf ("%s NYI", __FUNCTION__);
  return 0;
}

int s0il_dup(int oldfd)
{
  printf ("%s NYI", __FUNCTION__);
  return 0;
}

int s0il_dup2(int oldfd, int newfd)
{
  printf ("%s NYI", __FUNCTION__);
  return 0;
}
#endif

#if S0IL_NATIVE
#include <sys/mman.h>
#endif

void *s0il_mmap(void *addr, size_t length, int prot, int flags, int fd,
                size_t offset) {
#if S0IL_NATIVE
  return mmap(addr, length, prot, flags, fd, offset);
#else
  return NULL;
#endif
}
int s0il_munmap(void *addr, size_t length) { return 0; }
