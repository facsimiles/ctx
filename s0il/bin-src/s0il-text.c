#include "s0il.h"

typedef struct TextContext {
  char *path;
  uint8_t *contents;

  int length;
  int capacity;

  float scroll;
  float scroll_amount;

  int pos;
  int line;
  int virtual_line;
  int col;

  float cursor_x;

  int up_pos;
  int down_pos;

  int dirty;
} TextContext;

static TextContext _text_data;
static TextContext *text = &_text_data;

static void reset_cursor_info(void) {
  text->up_pos = -1;
  text->down_pos = -1;
  text->line = -1;
  text->virtual_line = -1;
}

void text_left(CtxEvent *event, void *data1, void *data2) {
  if (text->pos == 0)
    return;
  do {
    text->pos--;
  } while (text->pos && (text->contents[text->pos] & 192) == 128);
  ctx_event_stop_propagate(event);
  reset_cursor_info();
  text->cursor_x = -1.0f;
}

void text_right(CtxEvent *event, void *data1, void *data2) {
  text->pos += ctx_utf8_len(text->contents[text->pos]);
  ctx_event_stop_propagate(event);
  reset_cursor_info();
  text->cursor_x = -1.0f;
}

void text_down(CtxEvent *event, void *data1, void *data2) {
  if (text->down_pos >= 0)
    text->pos = text->down_pos;
  else {
    text->pos++;
    while (text->contents[text->pos] && (text->contents[text->pos] != '\n'))
      text->pos++;
    if (text->contents[text->pos] == '\n')
      text->pos++;
  }
  reset_cursor_info();
  ctx_event_stop_propagate(event);
}

void text_up(CtxEvent *event, void *data1, void *data2) {
  if (text->up_pos >= 0)
    text->pos = text->up_pos;
  else {
    if (text->pos > 0)
      text->pos--;
    while (text->pos > 0 && (text->contents[text->pos] != '\n'))
      text->pos--;
  }
  reset_cursor_info();
  ctx_event_stop_propagate(event);
}

void text_scroll_down(CtxEvent *event, void *data1, void *data2) {
  text->scroll -= text->scroll_amount;
  ctx_event_stop_propagate(event);
}

void text_scroll_up(CtxEvent *event, void *data1, void *data2) {
  text->scroll += text->scroll_amount;
  ctx_event_stop_propagate(event);
}

static void text_key_press(CtxEvent *event, void *data1, void *data2) {
  Ui *ui = data1;
  const char *string = event->string;

  if (!strcmp(string, "space"))
    string = " ";
  else if (!strcmp(string, "return"))
    string = "\n";

  if (ui_keyboard_visible(ui)) {
    if (!strcmp(string, "shift-space")) {
      s0il_do(ui, "kb-hide");
      return;
    }
    if (!strcmp(string, "escape")) {
      s0il_do(ui, "kb-hide");
      return;
    }
  } else {
    if (!strcmp(string, "shift-space")) {
      s0il_do(ui, "kb-show");
      return;
    }
    if (!strcmp(string, "return")) {
      s0il_do(ui, "kb-show");
      return;
    }
  }

  if (!strcmp(string, "escape"))
    s0il_do(ui, "exit");
  else if (!strcmp(string, "down"))
    text_down(event, data1, data2);
  else if (!strcmp(string, "up"))
    text_up(event, data1, data2);
  else if (!strcmp(string, "left"))
    text_left(event, data1, data2);
  else if (!strcmp(string, "right"))
    text_right(event, data1, data2);
  else if (!strcmp(string, "backspace")) {
    int old_cursor_pos = text->pos;
    text->pos--;
    while ((text->contents[text->pos] & 192) == 128)
      text->pos--;
    if (text->pos < 0)
      text->pos = 0;
    memmove(&text->contents[text->pos], &text->contents[old_cursor_pos],
            strlen((char *)&text->contents[old_cursor_pos]) + 1);
    text->dirty++;
  } else if (!strcmp(string, "delete")) {
    if (text->pos < (int)strlen((char *)text->contents)) {
      memmove(&text->contents[text->pos], &text->contents[text->pos + 1],
              strlen((char *)&text->contents[text->pos + 1]) + 1);
      while ((text->contents[text->pos] & 192) == 128)
        memmove(&text->contents[text->pos], &text->contents[text->pos + 1],
                strlen((char *)&text->contents[text->pos + 1]) + 1);
    }
    text->dirty++;
  } else if (ctx_utf8_strlen(string) == 1) {
    int insert_len = strlen(string);
    if ((int)strlen((char *)text->contents) + insert_len + 1 >=
        text->capacity) {
      text->capacity += 512;
      text->contents = realloc(text->contents, text->capacity);
    }

    {
      memmove(&text->contents[text->pos + insert_len],
              &text->contents[text->pos],
              strlen((char *)&text->contents[text->pos]) + 1);
      memcpy(&text->contents[text->pos], string, insert_len);
      text->pos += insert_len;
    }
    text->dirty++;
  } else
    printf("unhandled key:[%s]\n", event->string);
}

void view_text(Ui *ui, const char *path) {
  Ctx *ctx = ui_ctx(ui);
  // usleep (1000 * 130);
  ui_start_frame(ui);

#define UPDATE_NEIGHBOR()                                                      \
  do {                                                                         \
    if (((text->cursor_x >= 0.0f) && (text->virtual_line >= 0) &&              \
         (text->up_pos == -1)) &&                                              \
        (virtual_line_no == (text->virtual_line - 1)) &&                       \
        ((x >= text->cursor_x))) {                                             \
      text->up_pos = byte_pos;                                                 \
    }                                                                          \
    if (((text->virtual_line >= 0) && (text->cursor_x >= 0.0f) &&              \
         text->down_pos == -1) &&                                              \
        (virtual_line_no == (text->virtual_line + 1)) &&                       \
        (x >= text->cursor_x)) {                                               \
      text->down_pos = byte_pos;                                               \
    }                                                                          \
  } while (0)

#define MAYBE_DRAW_CURSOR()                                                    \
  do {                                                                         \
    if (byte_pos >= text->pos && !cursor_drawn) {                              \
      cursor_drawn = true;                                                     \
      const char *end =                                                        \
          &prev_word[strlen(prev_word) - (byte_pos - text->pos)];              \
      float part_width = ctx_text_width(ctx, end);                             \
      ctx_save(ctx);                                                           \
      ctx_rgba(ctx, 1, 0, 0, 0.5f);                                            \
      float cursor_x = x - part_width;                                         \
      float cursor_width = cw;                                                 \
      ctx_rectangle(ctx, cursor_x - cursor_width / 2, y - font_size * 0.8,     \
                    cursor_width, font_size);                                  \
      if (text->cursor_x < 0.0f)                                               \
        text->cursor_x = cursor_x;                                             \
      ctx_fill(ctx);                                                           \
      ctx_restore(ctx);                                                        \
    };                                                                         \
  } while (0)

#define ADD_WORD()                                                             \
  do {                                                                         \
    word[wordlen] = 0;                                                         \
    float word_width = ctx_text_width(ctx, word);                              \
    /*float word_width = (ctx_utf8_strlen (word)) * cw;*/                      \
    if (x + word_width >= x1) {                                                \
      y += line_height;                                                        \
      virtual_line_no++;                                                       \
      x = x0;                                                                  \
    }                                                                          \
    if (y > -cw && y < height + cw) {                                          \
      ctx_move_to(ctx, x, y);                                                  \
      ctx_text(ctx, word);                                                     \
    }                                                                          \
    x += word_width;                                                           \
    UPDATE_NEIGHBOR();                                                         \
    wordlen = 0;                                                               \
    strcpy(prev_word, word);                                                   \
    MAYBE_DRAW_CURSOR();                                                       \
    if (byte_pos >= text->pos && text->virtual_line <= -1) {                   \
      text->virtual_line = virtual_line_no;                                    \
      if (text->pos > 0 && contents[text->pos - 1] == '\n')                    \
        text->virtual_line--;                                                  \
    }                                                                          \
  } while (0)

  char *contents = (char *)text->contents;
  float width = ctx_width(ctx);
  float height = ctx_height(ctx);
  if (contents) {
    ctx_save(ctx);
    // ctx_font(ctx, "Mono");
    ctx_text_align(ctx, CTX_TEXT_ALIGN_START);
    float font_size = height / 13.0f;
    ctx_font_size(ctx, font_size);
    ctx_gray(ctx, 1.0f);
    float cw = ctx_text_width(ctx, " ");

    if (text->virtual_line >= 0) {
      float text_scroll_min =
          0.0f - ctx_height(ctx) / 6 - text->virtual_line * font_size;
      float text_scroll_max =
          0.0f + ctx_height(ctx) / 4 - text->virtual_line * font_size;
      if (ui_keyboard_visible(ui)) {
        text->scroll = text_scroll_min;
      } else {
        if (text->scroll < text_scroll_min)
          text->scroll = text_scroll_min;
        else if (text->scroll > text_scroll_max)
          text->scroll = text_scroll_max;
      }
    }

    char word[128] = "";
    char prev_word[128] = "";
    int wordlen = 0;

    float y = text->scroll + height * 0.5f;
    float x0 = cw * 4;
    float x = x0;
    float x1 = width - x0;
    float line_height = font_size;

    char *p = contents;
    int byte_pos = 0;
    int line_no = 1;
    int virtual_line_no = 0;
    // int col_no = 0;

    bool cursor_drawn = false;

#if 0
     char tmp[50];
     sprintf (tmp, "vl:%i:%i col:%i\n pos:%i  up:%i dn:%i %.1f", text_virtual_line, text_line, text_col,
     text_pos, text_up_pos, text_down_pos, (double)text_cursor_x);
     ctx_move_to (ctx, font_size * 2, y-font_size * 4);ctx_text(ctx, tmp);
#endif

    while (*p) {
      switch (*p) {
      case ' ':
        if (wordlen)
          ADD_WORD();
        x += cw;

        break;
      // TODO : tab
      case '\n': {
        if (wordlen)
          ADD_WORD();
        MAYBE_DRAW_CURSOR();
        UPDATE_NEIGHBOR();
        line_no++;
        virtual_line_no++;
        y += line_height;
        x = x0;
      } break;
      default:
        if (wordlen + 1 < (int)sizeof(word)) {
          word[wordlen++] = *p;
        }
        break;
      }
      p++;
      byte_pos++;
      if (byte_pos >= text->pos && text->line <= -1)
        text->line = line_no;
      UPDATE_NEIGHBOR();
    }
    if (wordlen)
      ADD_WORD();
    ctx_restore(ctx);
  }
  ui_end_frame(ui);

#undef ADD_WORD
  ctx_listen(ctx, CTX_KEY_PRESS, text_key_press, ui, NULL);
}

MAIN(s0il_text) {
  Ctx *ctx = ctx_new(512, 512, NULL);
  Ui *ui = ui_host(ctx);
  text->scroll = 0.0f - ctx_height(ctx) / 8;
  text->pos = 0;
  reset_cursor_info();

  if (argv[1] && !strcmp(argv[1], "--register"))
  {
    if (!ui) {
      printf("no-args would register mimetypes if run in env\n");
      return -1;
    }
    const char *mime_types[] = {"text/plain",
                                ".txt",
                                "text/markdown",
                                ".md",
                                "text/html",
                                ".html",
                                "application/javascript",
                                ".js",
                                "text/css",
                                ".css",
                                "text/x-csrc",
                                ".c",
                                "text/x-chdr",
                                ".h",
                                NULL,
                                NULL};
    for (int i = 0; mime_types[i]; i += 2) {
      if (mime_types[i + 1])
        s0il_add_magic(mime_types[i], mime_types[i + 1], NULL, 0, 1);
      s0il_add_view(ui, mime_types[i], NULL, argv[0]);
    }
  }
  else if (argv[1]) {
    Ctx *ctx = ui_ctx(ui);

    {
      text->contents = (uint8_t*)s0il_load_file(ui, argv[1], NULL);
      if (!text->contents) {
        ctx_destroy(ctx);
        return -1;
      }
      text->path = strdup(argv[1]);
      text->length = strlen((char *)text->contents);
      text->capacity = text->length + 1;
      text->dirty = 0;
    }

    do {
      ctx_start_frame(ctx);

      view_text(ui, argv[1]);
      ui_keyboard(ui);

      ctx_end_frame(ctx);
    } while (!ctx_has_exited(ctx));

    if (text->dirty) {
      FILE *f = fopen(argv[1], "w");
      fwrite(text->contents, strlen((char *)text->contents), 1, f);
      fclose(f);
    }
    free(text->path);
    text->path = NULL;
    free(text->contents);
    text->contents = NULL;
  } 

  ctx_destroy(ctx);
  return 0;
}
