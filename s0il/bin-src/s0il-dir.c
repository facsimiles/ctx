#include "s0il.h"

typedef struct dir_entry_t {
  char *name;
  char *path;
  const char *mime_type;
} dir_entry_t;

typedef struct dir_info_t {
  dir_entry_t *entries;
  int count;
  int capacity;
} dir_info_t;

static void dir_info_finalize(void *d) {
  dir_info_t *di = d;
  if (di->entries) {
    for (int i = 0; i < di->count; i++) {
      free(di->entries[i].name);
      free(di->entries[i].path);
    }
  }
  free(di->entries);
  free(di);
}

static int cmp_dir_entry(const void *p1, const void *p2) {
  const dir_entry_t *e1 = p1;
  const dir_entry_t *e2 = p2;

  int e1_is_dir = !strcmp(e1->mime_type, "inode/directory");
  int e2_is_dir = !strcmp(e2->mime_type, "inode/directory");

  if (e2_is_dir - e1_is_dir)
    return (e2_is_dir - e1_is_dir);

  return strcmp(e1->name, e2->name);
}

static void view_dir(Ui *ui, const char *location) {
  if (!ui_get_data(ui)) {
    dir_info_t *di = calloc(sizeof(dir_info_t), 1);

    ui_set_data (ui, di, dir_info_finalize);

    DIR *dir = s0il_opendir(location);

    if (dir) {
      struct dirent *ent;

      while ((ent = s0il_readdir(dir))) {
        const char *base = ent->d_name;

        if (base[0] == '.')
          continue;

        if (di->count + 1 >= di->capacity) {
          di->capacity += 16;
          di->entries =
              realloc(di->entries, sizeof(dir_entry_t) * di->capacity);
        }
        dir_entry_t *de = &di->entries[di->count++];
        de->name = strdup(base);
        de->path = malloc(strlen(location) + 3 + strlen(base));
        if (location[strlen(location) - 1] == '/')
          sprintf(de->path, "%s%s", location, base);
        else
          sprintf(de->path, "%s/%s", location, base);
        de->mime_type = s0il_detect_media_path(de->path);
      }
      s0il_closedir(dir);
      qsort(di->entries, di->count, sizeof(dir_entry_t), cmp_dir_entry);
    }
  }

  ui_start_frame(ui);
  ui_title(ui, location);

  dir_info_t *di = ui_get_data(ui);
  float height = ctx_height(ui_ctx(ui));
  float font_size = ui_get_font_size(ui);

  if (di)
    for (int i = 0; i < di->count; i++) {
      dir_entry_t *de = &di->entries[i];

      if (ui_y(ui) > height + font_size * 4 && i < di->count - 2) {
      } else if (ui_y(ui) > -font_size * 4 || i == 0) {
        if (ui_button(ui, de->name)) {
          ui_do(ui, de->path);
        }
      } else {
        ui_text(ui, de->name);
      }
    }

  ui_end_frame(ui);
}


MAIN(s0il_dir) {
  //Ctx *ctx = ctx_new(512, 512, NULL);
  Ctx *ctx = ctx_host();
  Ui *ui = ui_host(ctx);
  ui_set_data (ui, NULL, NULL);
  if (argv[1]) {
    Ctx *ctx = ui_ctx(ui);
    do {
      ctx_start_frame(ctx);

      view_dir(ui, argv[1]);
      ui_keyboard(ui);

      ctx_end_frame(ctx);
    } while (!ctx_has_exited(ctx));
  } else {
    if (!ui) {
      printf("no-args would register mimetypes if run in env\n");
      return -1;
    }
    ui_add_view(ui, "inode/directory", NULL, argv[0]);
  }

  //ctx_destroy(ctx);
  return 42;
}
