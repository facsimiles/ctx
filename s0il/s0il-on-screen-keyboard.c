#include "s0il.h"
/////////////////////////// keyboard

extern int ctx_osk_mode;
void ui_overlay_button(Ui *ui, float x, float y, float w, float h,
                       const char *label, const char *action);

#if !defined(CTX_FLOW3R)
float ct_angle = 0.0f;
float ct_pos = 0.5;

float bsp_captouch_angle(float *radial_pos, int quantize, uint16_t petal_mask) {
  if (radial_pos)
    *radial_pos = ct_pos;
  return ((int)(ct_angle * quantize + 0.5f)) / (quantize * 1.0f);
}
#endif

static void ctx_on_ct_event(CtxEvent *event, void *data1, void *data2) {
#if !defined(CTX_FLOW3R)
  float w = ctx_width(event->ctx);
  float h = ctx_height(event->ctx);
  float m = w;
  if (h < m)
    m = h;
  float x = event->x - w / 2;
  float y = event->y - h / 2;
  ct_pos = sqrtf(x * x + y * y) / m - 0.15f;
  ct_angle = atan2f(x, y) / (2 * 3.1415f);
  ct_angle -= 0.5f;
  while (ct_angle < 0.0f)
    ct_angle += 1.0f;
  ct_angle = 1.0f - ct_angle;
  if (event->type == CTX_DRAG_RELEASE)
    ct_angle = -1000.0f;
#endif
  event->stop_propagate = 1;
}

// static char kb_preview[8] = ""; // TODO : set this, and show it in middle

void kb_cursor_drag(CtxEvent *event, void *data1, void *data2) {
  event->stop_propagate = 1;
  const char *key = NULL;

  static uint32_t prev_event = 0;
  // TODO : key-repeat
  if (event->type == CTX_DRAG_PRESS) {
    prev_event = event->time;
  }

  float h_delta = event->start_x - event->x;
  float v_delta = event->start_y - event->y;
  float dist = hypotf(h_delta, v_delta);
  if (dist < ctx_height(event->ctx) * 0.05) {
    if (event->type == CTX_DRAG_RELEASE) {
      key = "space";
      ctx_key_press(event->ctx, 0, key, 0);
      return;
    }
  } else if (fabsf(h_delta) > fabsf(v_delta)) {
    if (h_delta > 0)
      key = "left";
    else
      key = "right";
  } else {
    if (v_delta > 0)
      key = "up";
    else
      key = "down";
  }
  if (key && (event->time - prev_event) > 400) {
    prev_event = event->time;
    ctx_key_press(event->ctx, 0, key, 0);
  }
}

void captouch_keyboard(Ctx *ctx) {
  float width = ctx_width(ctx);
  float height = ctx_height(ctx);
  ctx_save(ctx);
  ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
  float _em = height / 10;
  ctx_font_size(ctx, _em);

  float rad_pos;
  // #if CTX_FLOW3R
  float raw_angle = bsp_captouch_angle(&rad_pos, 20, 0);
  // #else
  //   float raw_angle = -1.0f;
  //   rad_pos = 0.5f;
  // #endif
  static float angle = -1.0f;

  if (raw_angle >= 0) {
    if (angle <= 0)
      angle = raw_angle;
    else
      // the values are already snapped, thus this acts as a debouncing
      // for position making the hovered position be the registered as
      // release position.
      angle = angle * 0.8f + raw_angle * 0.2f;
  } else
    angle = -1.0;

  static bool shift_down = false;
  static bool fn_down = false;
  static int last_col = 0;
  static int last_row = 0;
  static bool last_active = false;
  static bool last_down = false;
  bool down = false;
  int cursor_col = -1;

  int cursor_row = -1;
  static float smoothed_row = 0.0f;

  if (angle >= 0) {
    cursor_col = (int)(angle * 20 + 0.5f);
    if (cursor_col >= 20)
      cursor_col = 0;

    if (rad_pos > 0.05) {
      if (rad_pos < 0.15)
        cursor_row = 0;
      else if (rad_pos < 0.25)
        cursor_row = 1;
      else
        cursor_row = 2;
    }

    down = true;
  }

  smoothed_row = smoothed_row * 0.5f + 0.5 * (cursor_row + 0.5f);

  if (cursor_row != -1)
    cursor_row = smoothed_row;

  const char *neutral_rows[][12] = {
      {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "0", 0},
      {"a", "s", "d", "f", "g", "h", "j", "k", "l", ";", 0},
      {"z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "A", 0}};

  const char *shift_rows[][12] = {
      {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "~"},
      {"A", "S", "D", "F", "G", "H", "J", "K", "L", ":", 0},
      {"Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "a", 0}};

  const char *fn_rows[][12] = {
      {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "a", 0},
      {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", 0},
      {"-", "_", "+", "=", "[", "]", "|", "'", " ", "\"", "~", 0}};

  const char *fn_shift_rows[][12] = {
      {"{", "}", "`", "~", "\\", "€", "≈", "¡", "¿", "π", "0", 0},
      {"æ", "Æ", "Ø", "ø", "å", "Å", "è", "é", "É", "•", 0},
      {"ß", "…", "ü", "Ü", "ö", "Ö", "♥", "ð", "ñ", "Ñ", "0", 0}};

  ctx_save(ctx);
  ctx_rgba(ctx, 0, 0, 0, 0.6);
  ctx_arc(ctx, width / 2, height / 2, height * 0.6, M_PI * 1.05f, -M_PI * 0.05f,
          1);
  ctx_arc(ctx, width / 2, height / 2, height * 0.20, -M_PI * 0.05f,
          M_PI * 1.05f, 0);
  ctx_listen(ctx, CTX_DRAG, ctx_on_ct_event, NULL, NULL);

  ctx_fill(ctx);
  ctx_restore(ctx);

  if ((down != last_down) && (down == false)) {
    if (last_active) {
      if (last_row == 0 && last_col == 10) {
        fn_down = !fn_down;
      } else if (last_row == 2 && last_col == 10) {
        shift_down = !shift_down;
      } else {
        if (fn_down) {
          if (shift_down)
            ctx_key_press(ctx, 0, fn_shift_rows[last_row][last_col], 0);
          else
            ctx_key_press(ctx, 0, fn_rows[last_row][last_col], 0);
        } else {
          if (shift_down)
            ctx_key_press(ctx, 0, shift_rows[last_row][last_col], 0);
          else
            ctx_key_press(ctx, 0, neutral_rows[last_row][last_col], 0);
        }
      }
    }
  }

  last_down = false;
  last_active = false;
  for (int row = 0; row < 3; row++) {

    for (int col = 0; neutral_rows[row][col]; col++) {
      int i = 15 - col;

      ctx_save(ctx);
      ctx_translate(ctx, width / 2, height / 2);
      ctx_rotate(ctx, (i) / (1.0f * 20) * M_PI * 2 + M_PI);
      if (i == cursor_col && row == cursor_row) {
        ctx_rgba(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
        ctx_rectangle(ctx, -_em / 2, height * 0.5f - (3 - row) * _em, _em,
                      _em * 1.2);
        ctx_fill(ctx);
        last_col = col;
        last_row = row;
        last_active = true;
        last_down = true;
        ctx_rgba(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
      }
      ctx_move_to(ctx, 0, (height * 0.5 - (2.2 - row) * _em));
      if (fn_down) {
        if (shift_down)
          ctx_text(ctx, fn_shift_rows[row][col]);
        else
          ctx_text(ctx, fn_rows[row][col]);
      } else {
        if (shift_down)
          ctx_text(ctx, shift_rows[row][col]);
        else
          ctx_text(ctx, neutral_rows[row][col]);
      }
      ctx_restore(ctx);
    }
  }

#if 0
   if (angle >= 0)
   {
     ctx_save (ctx);
     ctx_translate (ctx, width/2, height/2);
     ctx_rotate (ctx, angle * M_PI * 2 + M_PI);
     ctx_move_to (ctx, 0, 0.5 * height/2);
     ctx_line_to (ctx, 0, height/2);
     ctx_line_width (ctx, 2.0);
     ctx_gray (ctx, 1.0f);
     ctx_stroke (ctx);
     ctx_restore (ctx);
   }
#endif

#if !defined(CTX_FLOW3R)
  ctx_save(ctx);
  ctx_rectangle(ctx, width * 0.7, height * 0.25, width * 0.3, height * 0.2);
  ctx_listen(ctx, CTX_PRESS, ui_cb_do, ui_host(ctx), (void *)"return");
  ctx_reset_path(ctx);

  ctx_rectangle(ctx, width * 0.0, height * 0.25, width * 0.15, height * 0.2);
  ctx_listen(ctx, CTX_PRESS, ui_cb_do, ui_host(ctx), (void *)"backspace");
  ctx_reset_path(ctx);
  ctx_rectangle(ctx, width * 0.15, height * 0.25, width * 0.15, height * 0.2);
  ctx_listen(ctx, CTX_PRESS, ui_cb_do, ui_host(ctx), (void *)"space");
  ctx_reset_path(ctx);

  ctx_rectangle(ctx, width * 0.4, height * 0.45, width * 0.2, height * 0.2);
  ctx_listen(ctx, CTX_DRAG, kb_cursor_drag, NULL, NULL);
  ctx_reset_path(ctx);

  ctx_restore(ctx);
#endif
}

void ui_keyboard(Ui *ui) {
  Ctx *ctx = ui_ctx(ui);

  switch (ctx_osk_mode) {
  case 2:
    // if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
    //    fade = 0.9;
    captouch_keyboard(ctx);
    return;
  case 1: {
    float h = ctx_height(ctx);
    float w = ctx_width(ctx);
    ui_overlay_button(ui, 0, h - h * 0.14, w, h * 0.14, "kb", "kb-show");
  } break;
  }
}

bool ui_keyboard_visible(Ui *ui) { return ctx_osk_mode != 0; }
