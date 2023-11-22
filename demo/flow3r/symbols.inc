/* this file lists all the symbols available for dynamic resolution
 * by the elf-loader
 *
 */


#include "esp_elf.h"
#include "private/elf_symbol.h"
//#include <semaphore.h>

#define EXPORT_UI
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
    {
       if (s[i] == '\n')
         ctx_vt_write(NULL, '\r');
       ctx_vt_write(NULL, s[i]);
    }
  }
  int ret = fputs(s, stream);
  return ret;
}

int wrap_fputc (int c, FILE *stream)
{
  if (stream == stdout || stream == stderr)
  {
     text_output = 1;
     if (c == '\n')
       ctx_vt_write(NULL, '\r');
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
    {
      if (s[i] == '\n')
        ctx_vt_write(NULL, '\r');
      ctx_vt_write(NULL, s[i]);
    }
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

void wrap_ctx_destroy(Ctx *ctx)
{
  gfx_output  = 0;
  //return ctx_new(width, height, backend);
}


extern float __divsf3(float a, float b);
extern double __floatsidf(int a);
double __muldf3 (double a, double b);
float __truncdfsf2 (double a);
void __extendsfdf2 (void);
void __floatundisf (void);

#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <setjmp.h>

const struct esp_elfsym g_customer_elfsyms[] = {


    //ESP_ELFSYM_EXPORT(htonl),
    

    ESP_ELFSYM_EXPORT(__muldf3),
    ESP_ELFSYM_EXPORT(__divsf3),
    ESP_ELFSYM_EXPORT(__floatsidf),
    ESP_ELFSYM_EXPORT(__floatundisf),
    ESP_ELFSYM_EXPORT(__truncdfsf2),
    ESP_ELFSYM_EXPORT(__extendsfdf2),


    { "puts",    &wrap_puts},
    { "putchar", &wrap_putchar},
    { "printf",  &wrap_printf},
    { "fputs",   &wrap_fputs},
    { "fputc",   &wrap_fputc},
    { "fprintf", &wrap_fprintf},
    { "fwrite",  &wrap_fwrite},

    ESP_ELFSYM_EXPORT(atof),
    ESP_ELFSYM_EXPORT(atoi),
    ESP_ELFSYM_EXPORT(atol),
    ESP_ELFSYM_EXPORT(atoll),
    ESP_ELFSYM_EXPORT(div),
    ESP_ELFSYM_EXPORT(qsort),
    ESP_ELFSYM_EXPORT(strtod),
    ESP_ELFSYM_EXPORT(strtol),
    ESP_ELFSYM_EXPORT(lldiv),
    ESP_ELFSYM_EXPORT(ldiv),

    ESP_ELFSYM_EXPORT(open),
    ESP_ELFSYM_EXPORT(link),
    ESP_ELFSYM_EXPORT(unlink),
    ESP_ELFSYM_EXPORT(fcntl),
    ESP_ELFSYM_EXPORT(creat),
    ESP_ELFSYM_EXPORT(close),
    ESP_ELFSYM_EXPORT(write),
    ESP_ELFSYM_EXPORT(read),

    ESP_ELFSYM_EXPORT(sysconf),
    ESP_ELFSYM_EXPORT(isalnum),
    ESP_ELFSYM_EXPORT(isalpha),
    ESP_ELFSYM_EXPORT(isascii),
    ESP_ELFSYM_EXPORT(isblank),
    ESP_ELFSYM_EXPORT(iscntrl),
    ESP_ELFSYM_EXPORT(isdigit),
    ESP_ELFSYM_EXPORT(isgraph),
    ESP_ELFSYM_EXPORT(islower),
    ESP_ELFSYM_EXPORT(isprint),
    ESP_ELFSYM_EXPORT(ispunct),
    ESP_ELFSYM_EXPORT(isspace),
    ESP_ELFSYM_EXPORT(isupper),
    ESP_ELFSYM_EXPORT(tolower),
    ESP_ELFSYM_EXPORT(toupper),
    ESP_ELFSYM_EXPORT(putenv),
    ESP_ELFSYM_EXPORT(setenv),
    ESP_ELFSYM_EXPORT(getenv),
    ESP_ELFSYM_EXPORT(unsetenv),
    ESP_ELFSYM_EXPORT(strerror),
    ESP_ELFSYM_EXPORT(abort),
    ESP_ELFSYM_EXPORT(seekdir),
    ESP_ELFSYM_EXPORT(telldir),
    ESP_ELFSYM_EXPORT(readdir_r),
    ESP_ELFSYM_EXPORT(connect),
    ESP_ELFSYM_EXPORT(listen),
    ESP_ELFSYM_EXPORT(bind),
    ESP_ELFSYM_EXPORT(socket),
    ESP_ELFSYM_EXPORT(accept),
    ESP_ELFSYM_EXPORT(getaddrinfo),
    ESP_ELFSYM_EXPORT(getsockname),
    ESP_ELFSYM_EXPORT(gethostbyname),
    ESP_ELFSYM_EXPORT(sinf),
    ESP_ELFSYM_EXPORT(logf),
    ESP_ELFSYM_EXPORT(atanf),
    ESP_ELFSYM_EXPORT(atan2f),
    ESP_ELFSYM_EXPORT(asinf),
    ESP_ELFSYM_EXPORT(hypotf),
    ESP_ELFSYM_EXPORT(cbrtf),
    ESP_ELFSYM_EXPORT(acosf),
    ESP_ELFSYM_EXPORT(tanf),
    ESP_ELFSYM_EXPORT(atanf),

    // we only include single precision variants
    ESP_ELFSYM_EXPORT(cosf),
    ESP_ELFSYM_EXPORT(powf),
    ESP_ELFSYM_EXPORT(sqrtf),
    ESP_ELFSYM_EXPORT(floorf),
    ESP_ELFSYM_EXPORT(exp10f),
    ESP_ELFSYM_EXPORT(exp2f),
    ESP_ELFSYM_EXPORT(expf),
    ESP_ELFSYM_EXPORT(expm1f),
    ESP_ELFSYM_EXPORT(fabsf),
    ESP_ELFSYM_EXPORT(fdimf),
    ESP_ELFSYM_EXPORT(finitef),
    ESP_ELFSYM_EXPORT(floorf),
    ESP_ELFSYM_EXPORT(ilogbf),
    ESP_ELFSYM_EXPORT(j0f),
    ESP_ELFSYM_EXPORT(j1f),
    ESP_ELFSYM_EXPORT(ldexpf),
    ESP_ELFSYM_EXPORT(lgammaf),
    ESP_ELFSYM_EXPORT(lrintf),
    ESP_ELFSYM_EXPORT(lroundf),
    ESP_ELFSYM_EXPORT(nanf),
    ESP_ELFSYM_EXPORT(nearbyintf),
    ESP_ELFSYM_EXPORT(nextafterf),
    ESP_ELFSYM_EXPORT(nexttowardf),
    ESP_ELFSYM_EXPORT(scalblnf),
    ESP_ELFSYM_EXPORT(scalbnf),
    ESP_ELFSYM_EXPORT(sincosf),
    ESP_ELFSYM_EXPORT(sinhf),
    ESP_ELFSYM_EXPORT(tgammaf),
    ESP_ELFSYM_EXPORT(truncf),

    ESP_ELFSYM_EXPORT(basename),
    ESP_ELFSYM_EXPORT(getopt),
    ESP_ELFSYM_EXPORT(ioctl),
    ESP_ELFSYM_EXPORT(realpath),
    ESP_ELFSYM_EXPORT(recv),
    ESP_ELFSYM_EXPORT(recvmsg),
    ESP_ELFSYM_EXPORT(recvfrom),
    ESP_ELFSYM_EXPORT(sendmsg),
    ESP_ELFSYM_EXPORT(sendto),
    ESP_ELFSYM_EXPORT(shutdown),
    ESP_ELFSYM_EXPORT(select),
    ESP_ELFSYM_EXPORT(poll),
    ESP_ELFSYM_EXPORT(setjmp),
    ESP_ELFSYM_EXPORT(longjmp),
    ESP_ELFSYM_EXPORT(mkdir),

    ESP_ELFSYM_EXPORT(asprintf),
    ESP_ELFSYM_EXPORT(clearerr),

    ESP_ELFSYM_EXPORT(calloc),
    ESP_ELFSYM_EXPORT(realloc),
    ESP_ELFSYM_EXPORT(aligned_alloc),
    ESP_ELFSYM_EXPORT(posix_memalign),
    ESP_ELFSYM_EXPORT(opendir),
    ESP_ELFSYM_EXPORT(closedir),
    ESP_ELFSYM_EXPORT(stat),
    ESP_ELFSYM_EXPORT(readdir),
    ESP_ELFSYM_EXPORT(sprintf),
    ESP_ELFSYM_EXPORT(srandom),
    ESP_ELFSYM_EXPORT(random),
    ESP_ELFSYM_EXPORT(fread),
    ESP_ELFSYM_EXPORT(fflush),
    ESP_ELFSYM_EXPORT(fgetc),
    ESP_ELFSYM_EXPORT(fgets),
    ESP_ELFSYM_EXPORT(fileno),
    ESP_ELFSYM_EXPORT(fscanf),
    ESP_ELFSYM_EXPORT(fseek),
    ESP_ELFSYM_EXPORT(fopen),
    ESP_ELFSYM_EXPORT(perror),
    ESP_ELFSYM_EXPORT(getc),
    ESP_ELFSYM_EXPORT(putc),
    ESP_ELFSYM_EXPORT(remove),
    ESP_ELFSYM_EXPORT(rename),
    ESP_ELFSYM_EXPORT(rewind),
    ESP_ELFSYM_EXPORT(scanf),
    ESP_ELFSYM_EXPORT(setbuf),
    ESP_ELFSYM_EXPORT(setbuffer),
    ESP_ELFSYM_EXPORT(setlinebuf),
    ESP_ELFSYM_EXPORT(setvbuf),
    ESP_ELFSYM_EXPORT(ungetc),
    ESP_ELFSYM_EXPORT(vasprintf),
    ESP_ELFSYM_EXPORT(vdprintf),
    ESP_ELFSYM_EXPORT(vfscanf),
    ESP_ELFSYM_EXPORT(vprintf),
    ESP_ELFSYM_EXPORT(vsnprintf),
    ESP_ELFSYM_EXPORT(vsprintf),
    ESP_ELFSYM_EXPORT(vscanf),
    ESP_ELFSYM_EXPORT(vsscanf),

    ESP_ELFSYM_EXPORT(abs),

    ESP_ELFSYM_EXPORT(bcmp),
    ESP_ELFSYM_EXPORT(bcopy),
    ESP_ELFSYM_EXPORT(bzero),
    ESP_ELFSYM_EXPORT(index),
    ESP_ELFSYM_EXPORT(memccpy),
    ESP_ELFSYM_EXPORT(memchr),
    ESP_ELFSYM_EXPORT(memcmp),
    ESP_ELFSYM_EXPORT(memcpy),
    ESP_ELFSYM_EXPORT(memmem),
    ESP_ELFSYM_EXPORT(memmove),
    ESP_ELFSYM_EXPORT(mempcpy),
    ESP_ELFSYM_EXPORT(memset),
    ESP_ELFSYM_EXPORT(rindex),
    ESP_ELFSYM_EXPORT(stpcpy),
    ESP_ELFSYM_EXPORT(stpncpy),
    ESP_ELFSYM_EXPORT(strcasecmp),
    ESP_ELFSYM_EXPORT(strcasestr),
    ESP_ELFSYM_EXPORT(strcat),
    ESP_ELFSYM_EXPORT(strchr),
    ESP_ELFSYM_EXPORT(strchrnul),
    ESP_ELFSYM_EXPORT(strcmp),
    ESP_ELFSYM_EXPORT(strcpy),
    ESP_ELFSYM_EXPORT(strcspn),
    ESP_ELFSYM_EXPORT(strdup),
    ESP_ELFSYM_EXPORT(strerror_r),
    ESP_ELFSYM_EXPORT(strlcat),
    ESP_ELFSYM_EXPORT(strlcpy),
    ESP_ELFSYM_EXPORT(strlen),
    ESP_ELFSYM_EXPORT(strncasecmp),
    ESP_ELFSYM_EXPORT(strncat),
    ESP_ELFSYM_EXPORT(strncmp),
    ESP_ELFSYM_EXPORT(strncpy),
    ESP_ELFSYM_EXPORT(strndup),
    ESP_ELFSYM_EXPORT(strnlen),
    ESP_ELFSYM_EXPORT(strpbrk),
    ESP_ELFSYM_EXPORT(strrchr),
    ESP_ELFSYM_EXPORT(strsep),
    ESP_ELFSYM_EXPORT(strsignal),
    ESP_ELFSYM_EXPORT(strspn),
    ESP_ELFSYM_EXPORT(strtok),
    ESP_ELFSYM_EXPORT(strtok_r),
    ESP_ELFSYM_EXPORT(strverscmp),
    ESP_ELFSYM_EXPORT(swab),

    ESP_ELFSYM_EXPORT(mktemp),
    ESP_ELFSYM_EXPORT(mkdtemp),
    ESP_ELFSYM_EXPORT(mkstemp),
    ESP_ELFSYM_EXPORT(mkostemp),

    ESP_ELFSYM_EXPORT(asctime),
    ESP_ELFSYM_EXPORT(clock),
    ESP_ELFSYM_EXPORT(clock_settime),
    ESP_ELFSYM_EXPORT(ctime),
    ESP_ELFSYM_EXPORT(ctime_r),
    ESP_ELFSYM_EXPORT(difftime),
    ESP_ELFSYM_EXPORT(gettimeofday),
    ESP_ELFSYM_EXPORT(gmtime),
    ESP_ELFSYM_EXPORT(localtime),
    ESP_ELFSYM_EXPORT(localtime_r),
    ESP_ELFSYM_EXPORT(mktime),
    ESP_ELFSYM_EXPORT(strftime),
    ESP_ELFSYM_EXPORT(time),

    ESP_ELFSYM_EXPORT(access),
    ESP_ELFSYM_EXPORT(chdir),
    ESP_ELFSYM_EXPORT(close),
    ESP_ELFSYM_EXPORT(fsync),
    ESP_ELFSYM_EXPORT(getcwd),
    ESP_ELFSYM_EXPORT(getpid),
    ESP_ELFSYM_EXPORT(isatty),


#ifdef EXPORT_UI
//  ESP_ELFSYM_EXPORT(_default_ui),
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
    ESP_ELFSYM_EXPORT(ctx_pcm_set_format),
    ESP_ELFSYM_EXPORT(ctx_pcm_set_sample_rate),
    ESP_ELFSYM_EXPORT(ctx_pcm_get_format),
    ESP_ELFSYM_EXPORT(ctx_pcm_get_sample_rate),
    ESP_ELFSYM_EXPORT(ctx_pcm_get_queued_length),
    ESP_ELFSYM_EXPORT(ctx_pcm_queue),
    ESP_ELFSYM_EXPORT(ctx_ticks),
    ESP_ELFSYM_EXPORT(ctx_logo),
    ESP_ELFSYM_EXPORT(ctx_quit),
    ESP_ELFSYM_EXPORT(ctx_get_event),
    ESP_ELFSYM_EXPORT(ctx_handle_events),

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
    ESP_ELFSYM_EXPORT(ctx_rgba_stroke),
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
    {"ctx_destroy", &wrap_ctx_destroy},
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
    ESP_ELFSYM_EXPORT(ctx_get_fullscreen),
    ESP_ELFSYM_EXPORT(ctx_set_fullscreen),
    ESP_ELFSYM_EXPORT(ctx_get_extend),
    ESP_ELFSYM_EXPORT(ctx_need_redraw),
    ESP_ELFSYM_EXPORT(ctx_queue_draw),
    ESP_ELFSYM_EXPORT(ctx_ticks),
    ESP_ELFSYM_EXPORT(ctx_set_clipboard),
    ESP_ELFSYM_EXPORT(ctx_get_clipboard),
    ESP_ELFSYM_EXPORT(ctx_quit),
    ESP_ELFSYM_EXPORT(ctx_rgba8),
    ESP_ELFSYM_EXPORT(ctx_has_quit),
    //ESP_ELFSYM_EXPORT(ctx_guess_media_type),
    //ESP_ELFSYM_EXPORT(ctx_path_get_media_type),
    ESP_ELFSYM_EXPORT(ctx_gstate_protect),
    ESP_ELFSYM_EXPORT(ctx_gstate_unprotect),



    ESP_ELFSYM_EXPORT(pthread_create),
    ESP_ELFSYM_EXPORT(pthread_exit),
    ESP_ELFSYM_EXPORT(pthread_join),
    ESP_ELFSYM_EXPORT(pthread_detach),
    ESP_ELFSYM_EXPORT(pthread_cancel),
    ESP_ELFSYM_EXPORT(pthread_once),
    ESP_ELFSYM_EXPORT(pthread_attr_init),
    ESP_ELFSYM_EXPORT(pthread_attr_destroy),
    ESP_ELFSYM_EXPORT(pthread_equal),
    ESP_ELFSYM_EXPORT(pthread_self),
    ESP_ELFSYM_EXPORT(sched_yield),
    ESP_ELFSYM_EXPORT(pthread_cond_init),
    ESP_ELFSYM_EXPORT(pthread_cond_destroy),
    ESP_ELFSYM_EXPORT(pthread_cond_signal),
    ESP_ELFSYM_EXPORT(pthread_cond_broadcast),
    ESP_ELFSYM_EXPORT(pthread_cond_wait),
    ESP_ELFSYM_EXPORT(pthread_cond_timedwait),


    ESP_ELFSYM_EXPORT(pthread_mutex_init),
    ESP_ELFSYM_EXPORT(pthread_mutex_destroy),
    ESP_ELFSYM_EXPORT(pthread_mutex_lock),
    ESP_ELFSYM_EXPORT(pthread_mutex_unlock),
    ESP_ELFSYM_EXPORT(pthread_mutex_trylock),
    ESP_ELFSYM_EXPORT(pthread_mutex_timedlock),


    ESP_ELFSYM_EXPORT(pthread_mutexattr_init),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_gettype),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_settype),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_destroy),
#if 0
    ESP_ELFSYM_EXPORT(pthread_mutexattr_getpshared),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_setpshared),
#endif

#if 0
    ESP_ELFSYM_EXPORT(sem_init),
    ESP_ELFSYM_EXPORT(sem_destroy),
    ESP_ELFSYM_EXPORT(sem_post),
    ESP_ELFSYM_EXPORT(sem_wait),
    ESP_ELFSYM_EXPORT(sem_trywait),
    ESP_ELFSYM_EXPORT(sem_timedwait),
    ESP_ELFSYM_EXPORT(sem_getvalue),
#endif

#if 0
    ESP_ELFSYM_EXPORT(pthread_rwlock_init),
    ESP_ELFSYM_EXPORT(pthread_rwlock_destroy),
    ESP_ELFSYM_EXPORT(pthread_rwlock_rdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_tryrdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_wrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_trywrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_unlock),
#endif

    ESP_ELFSYM_EXPORT(display_set_pixels),

    ESP_ELFSYM_END
};

