
#include "esp_elf.h"
#include "private/elf_symbol.h"

#ifdef EXPORT_UI
#include "ui.h"
#endif

static int text_output = 0;
static int gfx_output  = 0;

int  elf_output_state (void)
{
  return text_output * 1 + gfx_output * 2;
}
void reset_elf_output_state (void)
{
  text_output = 0;
  gfx_output = 0;
}

int wrap_fputs (const char *s, FILE *stream)
{
  if (stream == stdout || stream == stderr)
  {
    text_output = 1;
    for (int i = 0; s[i]; i++)
        ctx_vt_write(NULL, s[i]);
  }
  int ret = fputs(s, stream);
  return ret;
}

int wrap_fputc (int c, FILE *stream)
{
  if (stream == stdout || stream == stderr)
  {
     text_output = 1;
     ctx_vt_write(NULL, c);
  }
  return fputc(c, stream);
}

int wrap_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if (stream == stdout || stream == stderr)
  {
    text_output = 1;
    uint8_t *s=(uint8_t*)ptr;
    for (int i = 0; i< size*nmemb; i++)
      ctx_vt_write(NULL, s[i]);
  }

  return fwrite (ptr, size, nmemb, stream);
}

int wrap_puts (const char *s)
{
  int ret = wrap_fputs(s, stdout);
  wrap_fputc ('\n', stdout);
  return ret;
}

int wrap_putchar (int c)
{
  return wrap_fputc (c, stdout);
}



int wrap_fprintf (FILE *stream, const char *restrict format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  int ret;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  va_start (ap, format);
  ret = vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  wrap_fputs (buffer, stream);
  free (buffer);
  return ret;
}

int wrap_vfprintf (FILE *stream, const char *format, va_list ap)
{
  // XXX : not intercepted
  return vfprintf (stream, format, ap);
}

int wrap_printf (const char *restrict format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  int ret;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  va_start (ap, format);
  ret = vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  wrap_fputs (buffer, stdout);
  free (buffer);
  return ret;
}


Ctx *wrap_ctx_new(int width, int height, const char *backend)
{
  gfx_output  = 1;
  return ctx_new(width, height, backend);
}


extern float __divsf3(float a, float b);
extern double __floatsidf(int a);
double __muldf3 (double a, double b);
float __truncdfsf2 (double a);

const struct esp_elfsym g_customer_elfsyms[] = {
    ESP_ELFSYM_EXPORT(__muldf3),
    ESP_ELFSYM_EXPORT(__divsf3),
    ESP_ELFSYM_EXPORT(__floatsidf),
    ESP_ELFSYM_EXPORT(__truncdfsf2),
    { "puts", &wrap_puts},
    { "putchar", &wrap_putchar},
    { "printf", &wrap_printf},
    { "fputs", &wrap_fputs},
    { "fputc", &wrap_fputc},
    { "fprintf", &wrap_fprintf},
    { "fwrite", &wrap_fwrite},
#ifdef EXPORT_UI
    ESP_ELFSYM_EXPORT(_default_ui),
    ESP_ELFSYM_EXPORT(ui_start),
    ESP_ELFSYM_EXPORT(ui_text),
    ESP_ELFSYM_EXPORT(ui_button),
    ESP_ELFSYM_EXPORT(ui_toggle),
    ESP_ELFSYM_EXPORT(ui_entry),
    ESP_ELFSYM_EXPORT(ui_slider),
    ESP_ELFSYM_EXPORT(ui_do),
    ESP_ELFSYM_EXPORT(ui_new),
    ESP_ELFSYM_EXPORT(ui_destroy),
    ESP_ELFSYM_EXPORT(ui_scroll_to),
    ESP_ELFSYM_EXPORT(ui_set_scroll_offset),
    ESP_ELFSYM_EXPORT(ui_end),
    ESP_ELFSYM_EXPORT(ui_main),
#endif
    {"ctx_new", &wrap_ctx_new},
    ESP_ELFSYM_EXPORT(ctx_font_size),
    ESP_ELFSYM_EXPORT(ctx_line_width),
    ESP_ELFSYM_EXPORT(ctx_fill_rule),
    ESP_ELFSYM_EXPORT(ctx_line_join),
    ESP_ELFSYM_EXPORT(ctx_line_cap),
    ESP_ELFSYM_EXPORT(ctx_global_alpha),
    ESP_ELFSYM_EXPORT(ctx_linear_gradient),
    ESP_ELFSYM_EXPORT(ctx_radial_gradient),
    ESP_ELFSYM_EXPORT(ctx_text),
    ESP_ELFSYM_EXPORT(ctx_text_width),
    ESP_ELFSYM_EXPORT(ctx_glyph_width),
    ESP_ELFSYM_EXPORT(ctx_dirty_rect),
    ESP_ELFSYM_EXPORT(ctx_text_stroke),
    ESP_ELFSYM_EXPORT(ctx_text_align),
    ESP_ELFSYM_EXPORT(ctx_text_baseline),
    ESP_ELFSYM_EXPORT(ctx_text_direction),
    ESP_ELFSYM_EXPORT(ctx_compositing_mode),
    ESP_ELFSYM_EXPORT(ctx_blend_mode),
    ESP_ELFSYM_EXPORT(ctx_extend),
    ESP_ELFSYM_EXPORT(ctx_move_to),
    ESP_ELFSYM_EXPORT(ctx_line_to),
    ESP_ELFSYM_EXPORT(ctx_curve_to),
    ESP_ELFSYM_EXPORT(ctx_quad_to),
    ESP_ELFSYM_EXPORT(ctx_rel_move_to),
    ESP_ELFSYM_EXPORT(ctx_rel_line_to),
    ESP_ELFSYM_EXPORT(ctx_rel_curve_to),
    ESP_ELFSYM_EXPORT(ctx_rel_quad_to),
    ESP_ELFSYM_EXPORT(ctx_rgba),
    ESP_ELFSYM_EXPORT(ctx_rgb),
    ESP_ELFSYM_EXPORT(ctx_gray),
    ESP_ELFSYM_EXPORT(ctx_fill),
    ESP_ELFSYM_EXPORT(ctx_stroke),
    ESP_ELFSYM_EXPORT(ctx_save),
    ESP_ELFSYM_EXPORT(ctx_restore),
    ESP_ELFSYM_EXPORT(ctx_scale),
    ESP_ELFSYM_EXPORT(ctx_rotate),
    ESP_ELFSYM_EXPORT(ctx_translate),
    ESP_ELFSYM_EXPORT(ctx_apply_transform),
    ESP_ELFSYM_EXPORT(ctx_gradient_add_stop),
    ESP_ELFSYM_EXPORT(ctx_listen),
    ESP_ELFSYM_EXPORT(ctx_get_event),
    ESP_ELFSYM_EXPORT(ctx_pointer_is_down),
    ESP_ELFSYM_EXPORT(ctx_pointer_x),
    ESP_ELFSYM_EXPORT(ctx_pointer_y),
    ESP_ELFSYM_EXPORT(ctx_freeze),
    ESP_ELFSYM_EXPORT(ctx_thaw),
    ESP_ELFSYM_EXPORT(ctx_events_frozen),
    ESP_ELFSYM_EXPORT(ctx_events_clear_items),
    ESP_ELFSYM_EXPORT(ctx_key_down),
    ESP_ELFSYM_EXPORT(ctx_key_up),
    ESP_ELFSYM_EXPORT(ctx_key_press),
    ESP_ELFSYM_EXPORT(ctx_scrolled),
    ESP_ELFSYM_EXPORT(ctx_incoming_message),
    ESP_ELFSYM_EXPORT(ctx_pointer_motion),
    ESP_ELFSYM_EXPORT(ctx_pointer_press),
    ESP_ELFSYM_EXPORT(ctx_pointer_release),
    ESP_ELFSYM_EXPORT(ctx_pointer_drop),
    ESP_ELFSYM_EXPORT(ctx_get_event_fds),
    ESP_ELFSYM_EXPORT(ctx_listen_with_finalize),
    ESP_ELFSYM_EXPORT(ctx_listen_full),
    ESP_ELFSYM_EXPORT(ctx_event_stop_propagate),
    ESP_ELFSYM_EXPORT(ctx_add_key_binding),
    ESP_ELFSYM_EXPORT(ctx_add_key_binding_full),
    ESP_ELFSYM_EXPORT(ctx_get_bindings),
    ESP_ELFSYM_EXPORT(ctx_clear_bindings),
    ESP_ELFSYM_EXPORT(ctx_remove_idle),
    ESP_ELFSYM_EXPORT(ctx_add_timeout_full),
    ESP_ELFSYM_EXPORT(ctx_add_idle),
    ESP_ELFSYM_EXPORT(ctx_add_idle_full),
    ESP_ELFSYM_EXPORT(ctx_add_timeout),
    ESP_ELFSYM_EXPORT(ctx_get_font_name),
    ESP_ELFSYM_EXPORT(ctx_wrap_left),
    ESP_ELFSYM_EXPORT(ctx_wrap_right),
    ESP_ELFSYM_EXPORT(ctx_line_height),
    ESP_ELFSYM_EXPORT(ctx_destroy),
    ESP_ELFSYM_EXPORT(ctx_start_frame),
    ESP_ELFSYM_EXPORT(ctx_end_frame),
    ESP_ELFSYM_EXPORT(ctx_begin_path),
    ESP_ELFSYM_EXPORT(ctx_start_group),
    ESP_ELFSYM_EXPORT(ctx_end_group),
    ESP_ELFSYM_EXPORT(ctx_clip),
    ESP_ELFSYM_EXPORT(ctx_image_smoothing),
    ESP_ELFSYM_EXPORT(ctx_arc),
    ESP_ELFSYM_EXPORT(ctx_arc_to),
    ESP_ELFSYM_EXPORT(ctx_rel_arc_to),
    ESP_ELFSYM_EXPORT(ctx_rectangle),
    ESP_ELFSYM_EXPORT(ctx_round_rectangle),
    ESP_ELFSYM_EXPORT(ctx_close_path),
    ESP_ELFSYM_EXPORT(ctx_miter_limit),
    ESP_ELFSYM_EXPORT(ctx_line_dash_offset),
    ESP_ELFSYM_EXPORT(ctx_line_dash),
    ESP_ELFSYM_EXPORT(ctx_font),
    ESP_ELFSYM_EXPORT(ctx_font_family),
    ESP_ELFSYM_EXPORT(ctx_font_extents),
    //ESP_ELFSYM_EXPORT(ctx_parse),
    ESP_ELFSYM_EXPORT(ctx_new_page),
    ESP_ELFSYM_EXPORT(ctx_view_box),
    ESP_ELFSYM_EXPORT(ctx_define_texture),
    ESP_ELFSYM_EXPORT(ctx_drop_eid),
    ESP_ELFSYM_EXPORT(ctx_width),
    ESP_ELFSYM_EXPORT(ctx_height),
    ESP_ELFSYM_EXPORT(ctx_x),
    ESP_ELFSYM_EXPORT(ctx_y),
    ESP_ELFSYM_EXPORT(ctx_get_global_alpha),
    ESP_ELFSYM_EXPORT(ctx_get_font_size),
    ESP_ELFSYM_EXPORT(ctx_get_miter_limit),
    ESP_ELFSYM_EXPORT(ctx_get_image_smoothing),
    ESP_ELFSYM_EXPORT(ctx_get_line_dash_offset),
    ESP_ELFSYM_EXPORT(ctx_get_wrap_left),
    ESP_ELFSYM_EXPORT(ctx_get_wrap_right),
    ESP_ELFSYM_EXPORT(ctx_get_line_height),
    ESP_ELFSYM_EXPORT(ctx_get_line_join),
    ESP_ELFSYM_EXPORT(ctx_get_line_cap),
    ESP_ELFSYM_EXPORT(ctx_get_line_width),
    ESP_ELFSYM_EXPORT(ctx_get_font),
    ESP_ELFSYM_EXPORT(ctx_get_transform),
    ESP_ELFSYM_EXPORT(ctx_clip_extents),
    ESP_ELFSYM_EXPORT(ctx_texture_load),
    ESP_ELFSYM_EXPORT(ctx_texture),
    ESP_ELFSYM_EXPORT(ctx_draw_texture),
    ESP_ELFSYM_EXPORT(ctx_draw_texture_clipped),
    ESP_ELFSYM_EXPORT(ctx_draw_image),
    ESP_ELFSYM_EXPORT(ctx_set_texture_source),
    ESP_ELFSYM_EXPORT(ctx_set_texture_cache),
    ESP_ELFSYM_EXPORT(ctx_deferred_scale),
    ESP_ELFSYM_EXPORT(ctx_deferred_translate),
    ESP_ELFSYM_EXPORT(ctx_deferred_move_to),
    ESP_ELFSYM_EXPORT(ctx_deferred_rel_line_to),
    ESP_ELFSYM_EXPORT(ctx_deferred_rel_move_to),
    ESP_ELFSYM_EXPORT(ctx_deferred_rectangle),
    ESP_ELFSYM_EXPORT(ctx_resolve),
    ESP_ELFSYM_EXPORT(ctx_render_ctx),
    ESP_ELFSYM_EXPORT(ctx_render_ctx_textures),
    ESP_ELFSYM_EXPORT(ctx_get_text_align),
    ESP_ELFSYM_EXPORT(ctx_get_text_baseline),
    ESP_ELFSYM_EXPORT(ctx_get_compositing_mode),
    ESP_ELFSYM_EXPORT(ctx_get_blend_mode),
    ESP_ELFSYM_EXPORT(ctx_get_extend),
    ESP_ELFSYM_EXPORT(ctx_need_redraw),
    ESP_ELFSYM_EXPORT(ctx_queue_draw),
    ESP_ELFSYM_EXPORT(ctx_ticks),
    ESP_ELFSYM_EXPORT(ctx_set_clipboard),
    ESP_ELFSYM_EXPORT(ctx_get_clipboard),
    ESP_ELFSYM_EXPORT(ctx_quit),
    ESP_ELFSYM_EXPORT(ctx_has_quit),
    //ESP_ELFSYM_EXPORT(ctx_guess_media_type),
    //ESP_ELFSYM_EXPORT(ctx_path_get_media_type),
    ESP_ELFSYM_EXPORT(ctx_gstate_protect),
    ESP_ELFSYM_EXPORT(ctx_gstate_unprotect),


    ESP_ELFSYM_EXPORT(sprintf),
    ESP_ELFSYM_END
};

