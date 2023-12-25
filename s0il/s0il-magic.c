#include "port_config.h"
#include "s0il.h"
#include <unistd.h>

typedef struct ctx_magic_t {
  bool is_text;
  char *mime_type;
  char *ext;
  uint8_t magic[16];
  int magic_len;
} ctx_magic_t;

static CtxList *ui_magic = NULL;

void s0il_add_magic(const char *mime_type, const char *ext, const char *magic_data,
               int magic_len, int is_text) {
  // TODO : skip duplicates
  if (!mime_type)
    return;
  if (magic_len < 0) {
    if (magic_data)
      magic_len = strlen((char *)magic_data);
  }

  if (magic_len > 16)
    magic_len = 16;
  ctx_magic_t *magic = calloc(sizeof(ctx_magic_t), 1);
  magic->mime_type = strdup(mime_type);
  if (ext)
    magic->ext = strdup(ext);
  if (magic_data)
    memcpy((char *)magic->magic, (char *)magic_data, magic_len);
  magic->magic_len = magic_len;
  magic->is_text = is_text;

#if 0
   for (CtxList *iter = ui_magic; iter; iter = iter->next)
   {
     ctx_magic_t *magic_b = iter->data;
     if (!strcmp (magic_b->mime_type, mime_type) &&
        (!strcmp (magic_b->mime_type, mime_type) &&
       bail
   }
#endif

  ctx_list_append(&ui_magic, magic);
}

bool s0il_has_mime(const char *mime_type) {
  for (CtxList *iter = ui_magic; iter; iter = iter->next) {
    ctx_magic_t *magic = iter->data;
    if (!strcmp(magic->mime_type, mime_type))
      return true;
  }
  return false;
}

int s0il_media_is_text(const char *media_type) {
  for (CtxList *iter = ui_magic; iter; iter = iter->next) {
    ctx_magic_t *magic = iter->data;
    if (!strcmp(magic->mime_type, media_type))
      return magic->is_text;
  }
  return 0;
}

static const char *s0il_detect_dir_type(const char *path) {
  char temp[512];
  for (CtxList *iter = ui_magic; iter; iter = iter->next) {
    ctx_magic_t *magic = iter->data;
    if (magic->ext && !strcmp(magic->ext, "inode/directory")) {
      snprintf(temp, sizeof(temp), "%s/%s", path, magic->magic);
      if (s0il_access(temp, R_OK) == F_OK)
        return magic->mime_type;
    }
  }
  return "inode/directory";
}

const char *s0il_detect_media_sector512(const char *path, const char *sector) {
  const char *suffix_match = NULL;
  const char *magic_match = NULL;

  for (CtxList *iter = ui_magic; iter; iter = iter->next) {
    ctx_magic_t *magic = iter->data;
    if (magic->ext && strstr(path, magic->ext) &&
        (strstr(path, magic->ext)[strlen(magic->ext)] == 0))
      suffix_match = magic->mime_type;
    if (magic->magic_len && !memcmp(sector, magic->magic, magic->magic_len))
      magic_match = magic->mime_type;
  }

  if (magic_match)
    return magic_match;
  if (suffix_match)
    return suffix_match;

  for (unsigned int i = 0; i < 512 - 4;) {
    uint8_t first_byte = ((uint8_t *)sector)[i];
    if ((first_byte & 0x80) == 0) {
      i++;
    } else {
      /// UTF-8 detector
      int trail_bytes = 0;
      if ((first_byte & (128 + 64 + 32)) == 128 + 64)
        trail_bytes = 1;
      else if ((first_byte & (128 + 64 + 32 + 16)) == 128 + 64 + 32)
        trail_bytes = 2;
      else if ((first_byte & (128 + 64 + 32 + 16 + 8)) == 128 + 64 + 32 + 16)
        trail_bytes = 3;
      else
        return "application/octet-stream";
      for (int j = 0; j < trail_bytes; j++) {
        uint8_t byte = ((uint8_t *)sector)[i + j + 1];
        if ((byte & (128 + 64)) != 128)
          return "application/octet-stream";
      }
      i++;
      i += trail_bytes;
    }
  }
  return "text/plain";
}

const char *s0il_detect_media_path(const char *location) {
  const char *path = location;
  if (strchr(path, ':') && strchr(path, ':') < path + 5) {
    path = strchr(path, ':') + 1;
    if (path[0] == '/')
      path++;
    if (path[0] == '/')
      path++;
  }

  struct stat stat_buf;
  if (!path || path[0] == 0)
    return 0;
  s0il_stat(path, &stat_buf);
  if (S_ISDIR(stat_buf.st_mode)) {
    return s0il_detect_dir_type(location);
  }

  char sector[512] = {
      0,
  };
  FILE *f = s0il_fopen(path, "r");
  if (!f)
    return NULL;

  memset(sector, 0, 512);
  s0il_fread(sector, 512, 1, f);
  s0il_fclose(f);
  return s0il_detect_media_sector512(location, sector);
}

int magic_main(int argc, char **argv) {
  for (CtxList *iter = ui_magic; iter; iter = iter->next) {
    ctx_magic_t *magic = iter->data;
    s0il_printf("%s ext:%s magic-len:%i %s\n", magic->mime_type, magic->ext,
                magic->magic_len, magic->is_text ? "text" : "");
  }
  return 0;
}

int file_main(int argc, char **argv) {
  for (int i = 1; argv[i]; i++)
    if (argv[i][0] != '-')
      s0il_printf("%s : %s\n", argv[i], s0il_detect_media_path(argv[i]));
  return 0;
}
