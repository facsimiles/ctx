/* this file lists all the symbols available for dynamic resolution
 * by the elf-loader
 *
 */

//#if CTX_FLOW3R
#undef strstr
#undef strlen
//#endif
#define _XOPEN_SOURCE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#if NATIVE
#include <poll.h>
#endif

#define EXPORT_UI 1

#include "ctx.h"
#ifdef EXPORT_UI
#include "ui.h"
#endif
#include "run.h"

#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <setjmp.h>

#include "run-stdio.c"

extern float __divsf3     (float a, float b);
extern double __floatsidf (int a);
double __muldf3    (double a, double b);
float __truncdfsf2 (double a);
void __extendsfdf2 (void);
void __floatundisf (void);
void __fixdfsi (void);
void __adddf3  (void);
void __subdf3  (void);
void __eqdf2   (void);
void __nedf2   (void);
void __ledf2   (void);
void __gedf2   (void);
void __lshrdi3 (void);

void busywarp(void);
void ctx_set_pixels(Ctx *ctx, void *userdata, int x, int y, int w, int h, void *buf);

extern int ctx_osk_mode;
int wifi_init_sta(const char *ssid_arg, const char *password_arg);
char **wifi_scan(void);

#define ELFSYM_EXPORT(_sym)  { #_sym, &_sym }
#define ELFSYM_END           { NULL, NULL }

struct esp_elfsym {const char *name; void *sym;};

void run_signal(int sig, void(*func)(int))
{
#if NATIVE
  signal(sig, func);
#else
#endif
}

const struct esp_elfsym g_customer_elfsyms[] =
{

#ifndef NATIVE
    ELFSYM_EXPORT(__adddf3),
    ELFSYM_EXPORT(__assert_func),
    ELFSYM_EXPORT(__divsf3),
    ELFSYM_EXPORT(__extendsfdf2),
    ELFSYM_EXPORT(__eqdf2),
    ELFSYM_EXPORT(__fixdfsi),
    ELFSYM_EXPORT(__floatsidf),
    ELFSYM_EXPORT(__floatundisf),
    ELFSYM_EXPORT(__gedf2),
    ELFSYM_EXPORT(__ledf2),
    ELFSYM_EXPORT(__lshrdi3),
    ELFSYM_EXPORT(__muldf3),
    ELFSYM_EXPORT(__nedf2),
    ELFSYM_EXPORT(__subdf3),
    ELFSYM_EXPORT(__truncdfsf2),
#endif
    {"ctx_destroy", &run_ctx_destroy},
    {"ctx_new",     &run_ctx_new},
    ELFSYM_EXPORT(abort),
    ELFSYM_EXPORT(abs),
    ELFSYM_EXPORT(accept),
    {"access", &run_access},
    ELFSYM_EXPORT(acos),
    ELFSYM_EXPORT(acosf),
    ELFSYM_EXPORT(aligned_alloc),
    ELFSYM_EXPORT(asctime),
    ELFSYM_EXPORT(asctime_r),
    ELFSYM_EXPORT(asin),
    ELFSYM_EXPORT(asinf),
    ELFSYM_EXPORT(asprintf),
    ELFSYM_EXPORT(atan2),
    ELFSYM_EXPORT(atan2f),
    ELFSYM_EXPORT(atan),
    ELFSYM_EXPORT(atanf),
    ELFSYM_EXPORT(atanf),
    ELFSYM_EXPORT(atof),
    ELFSYM_EXPORT(atoi),
    ELFSYM_EXPORT(atol),
    ELFSYM_EXPORT(atoll),
    ELFSYM_EXPORT(basename),
    ELFSYM_EXPORT(bcmp),
    ELFSYM_EXPORT(bcopy),
    ELFSYM_EXPORT(bind),
#if CTX_FLOW3R
    ELFSYM_EXPORT(bsp_captouch_angle),
    ELFSYM_EXPORT(bsp_captouch_angular),
    ELFSYM_EXPORT(bsp_captouch_down),
    ELFSYM_EXPORT(bsp_captouch_radial),
#endif
    ELFSYM_EXPORT(busywarp),
    ELFSYM_EXPORT(bzero),
    ELFSYM_EXPORT(calloc),
    ELFSYM_EXPORT(cbrt),
    ELFSYM_EXPORT(cbrtf),
    ELFSYM_EXPORT(ceil),
    ELFSYM_EXPORT(ceilf),
    ELFSYM_EXPORT(chdir),
    ELFSYM_EXPORT(clearerr),
    ELFSYM_EXPORT(clearerr),
    ELFSYM_EXPORT(clock),
    ELFSYM_EXPORT(clock_gettime),
    ELFSYM_EXPORT(clock_settime),
    ELFSYM_EXPORT(close),
    ELFSYM_EXPORT(close),
    {"closedir", run_closedir},
    ELFSYM_EXPORT(connect),
    ELFSYM_EXPORT(cos),
    ELFSYM_EXPORT(cosf),
    ELFSYM_EXPORT(cosh),
    ELFSYM_EXPORT(coshf),
    ELFSYM_EXPORT(creat),
    ELFSYM_EXPORT(ctime),
    ELFSYM_EXPORT(ctime),
    ELFSYM_EXPORT(ctime_r),
    ELFSYM_EXPORT(ctime_r),
    ELFSYM_EXPORT(ctx_add_idle),
    ELFSYM_EXPORT(ctx_add_idle_full),
    ELFSYM_EXPORT(ctx_add_key_binding),
    ELFSYM_EXPORT(ctx_add_key_binding_full),
    ELFSYM_EXPORT(ctx_add_timeout),
    ELFSYM_EXPORT(ctx_add_timeout_full),
    ELFSYM_EXPORT(ctx_append_drawlist),
    ELFSYM_EXPORT(ctx_apply_transform),
    ELFSYM_EXPORT(ctx_arc),
    ELFSYM_EXPORT(ctx_arc_to),
    ELFSYM_EXPORT(ctx_begin_path),
    ELFSYM_EXPORT(ctx_blend_mode),
    ELFSYM_EXPORT(ctx_clear_bindings),
    ELFSYM_EXPORT(ctx_clip),
    ELFSYM_EXPORT(ctx_clip_extents),
    ELFSYM_EXPORT(ctx_close_path),
    ELFSYM_EXPORT(ctx_compositing_mode),
    ELFSYM_EXPORT(ctx_current_path),
    ELFSYM_EXPORT(ctx_curve_to),
    ELFSYM_EXPORT(ctx_deferred_move_to),
    ELFSYM_EXPORT(ctx_deferred_rectangle),
    ELFSYM_EXPORT(ctx_deferred_rel_line_to),
    ELFSYM_EXPORT(ctx_deferred_rel_move_to),
    ELFSYM_EXPORT(ctx_deferred_scale),
    ELFSYM_EXPORT(ctx_deferred_translate),
    ELFSYM_EXPORT(ctx_define_texture),
    ELFSYM_EXPORT(ctx_dirty_rect),
    ELFSYM_EXPORT(ctx_draw_image),
    ELFSYM_EXPORT(ctx_drawlist_clear),
    ELFSYM_EXPORT(ctx_draw_texture),
    ELFSYM_EXPORT(ctx_draw_texture_clipped),
    ELFSYM_EXPORT(ctx_drop_eid),
    ELFSYM_EXPORT(ctx_end_frame),
    ELFSYM_EXPORT(ctx_end_group),
    ELFSYM_EXPORT(ctx_events_clear_items),
    ELFSYM_EXPORT(ctx_events_frozen),
    ELFSYM_EXPORT(ctx_event_stop_propagate),
    ELFSYM_EXPORT(ctx_event_stop_propagate),
    ELFSYM_EXPORT(ctx_extend),
    ELFSYM_EXPORT(ctx_fill),
    ELFSYM_EXPORT(ctx_fill_rule),
    ELFSYM_EXPORT(ctx_font),
    ELFSYM_EXPORT(ctx_font_extents),
    ELFSYM_EXPORT(ctx_font_family),
    ELFSYM_EXPORT(ctx_font_size),
    ELFSYM_EXPORT(ctx_freeze),
    ELFSYM_EXPORT(ctx_get_bindings),
    ELFSYM_EXPORT(ctx_get_bindings),
    ELFSYM_EXPORT(ctx_get_blend_mode),
    ELFSYM_EXPORT(ctx_get_clipboard),
    ELFSYM_EXPORT(ctx_get_compositing_mode),
    ELFSYM_EXPORT(ctx_get_event),
    ELFSYM_EXPORT(ctx_get_event),
    ELFSYM_EXPORT(ctx_get_event_fds),
    ELFSYM_EXPORT(ctx_get_extend),
    ELFSYM_EXPORT(ctx_get_fill_rule),
    ELFSYM_EXPORT(ctx_get_font),
    ELFSYM_EXPORT(ctx_get_font_name),
    ELFSYM_EXPORT(ctx_get_font_size),
    ELFSYM_EXPORT(ctx_get_fullscreen),
    ELFSYM_EXPORT(ctx_get_global_alpha),
    ELFSYM_EXPORT(ctx_get_image_smoothing),
    ELFSYM_EXPORT(ctx_get_line_cap),
    ELFSYM_EXPORT(ctx_get_line_dash_offset),
    ELFSYM_EXPORT(ctx_get_line_height),
    ELFSYM_EXPORT(ctx_get_line_join),
    ELFSYM_EXPORT(ctx_get_line_width),
    ELFSYM_EXPORT(ctx_get_miter_limit),
    ELFSYM_EXPORT(ctx_get_text_align),
    ELFSYM_EXPORT(ctx_get_text_baseline),
    ELFSYM_EXPORT(ctx_get_transform),
    ELFSYM_EXPORT(ctx_get_wrap_left),
    ELFSYM_EXPORT(ctx_get_wrap_right),
    ELFSYM_EXPORT(ctx_global_alpha),
    ELFSYM_EXPORT(ctx_glyph_width),
    ELFSYM_EXPORT(ctx_gradient_add_stop),
    ELFSYM_EXPORT(ctx_gray),
    ELFSYM_EXPORT(ctx_gstate_protect),
    ELFSYM_EXPORT(ctx_gstate_unprotect),
    ELFSYM_EXPORT(ctx_guess_media_type),
    ELFSYM_EXPORT(ctx_handle_events),
    ELFSYM_EXPORT(ctx_has_quit),
    ELFSYM_EXPORT(ctx_height),
    ELFSYM_EXPORT(ctx_image_smoothing),
    ELFSYM_EXPORT(ctx_incoming_message),
    ELFSYM_EXPORT(ctx_in_fill),
    ELFSYM_EXPORT(ctx_in_stroke),
    ELFSYM_EXPORT(ctx_iterator_next),
    ELFSYM_EXPORT(ctx_key_down),
    ELFSYM_EXPORT(ctx_key_press),
    ELFSYM_EXPORT(ctx_key_up),
    ELFSYM_EXPORT(ctx_linear_gradient),
    ELFSYM_EXPORT(ctx_line_cap),
    ELFSYM_EXPORT(ctx_line_dash),
    ELFSYM_EXPORT(ctx_line_dash_offset),
    ELFSYM_EXPORT(ctx_line_height),
    ELFSYM_EXPORT(ctx_line_join),
    ELFSYM_EXPORT(ctx_line_to),
    ELFSYM_EXPORT(ctx_line_width),
    ELFSYM_EXPORT(ctx_listen),
    ELFSYM_EXPORT(ctx_listen_full),
    ELFSYM_EXPORT(ctx_listen_with_finalize),
    ELFSYM_EXPORT(ctx_logo),
    ELFSYM_EXPORT(ctx_matrix_identity),
    ELFSYM_EXPORT(ctx_matrix_invert),
    ELFSYM_EXPORT(ctx_matrix_rotate),
    ELFSYM_EXPORT(ctx_matrix_translate),
    ELFSYM_EXPORT(ctx_miter_limit),
    ELFSYM_EXPORT(ctx_move_to),
    ELFSYM_EXPORT(ctx_need_redraw),
    ELFSYM_EXPORT(ctx_new_drawlist),
    ELFSYM_EXPORT(ctx_new_for_framebuffer),
    ELFSYM_EXPORT(ctx_new_page),
    ELFSYM_EXPORT(ctx_osk_mode),
    ELFSYM_EXPORT(ctx_paint),
    ELFSYM_EXPORT(ctx_parse),
    ELFSYM_EXPORT(ctx_path_extents),
    ELFSYM_EXPORT(ctx_path_get_media_type),
    ELFSYM_EXPORT(ctx_pcm_get_format),
    ELFSYM_EXPORT(ctx_pcm_get_queued_length),
    ELFSYM_EXPORT(ctx_pcm_get_sample_rate),
    ELFSYM_EXPORT(ctx_pcm_queue),
    ELFSYM_EXPORT(ctx_pcm_set_format),
    ELFSYM_EXPORT(ctx_pcm_set_sample_rate),
    ELFSYM_EXPORT(ctx_pixel_format_get_stride),
    ELFSYM_EXPORT(ctx_pointer_drop),
    ELFSYM_EXPORT(ctx_pointer_is_down),
    ELFSYM_EXPORT(ctx_pointer_motion),
    ELFSYM_EXPORT(ctx_pointer_press),
    ELFSYM_EXPORT(ctx_pointer_release),
    ELFSYM_EXPORT(ctx_pointer_x),
    ELFSYM_EXPORT(ctx_pointer_y),
    ELFSYM_EXPORT(ctx_preserve),
    ELFSYM_EXPORT(ctx_quad_to),
    ELFSYM_EXPORT(ctx_queue_draw),
    ELFSYM_EXPORT(ctx_quit),
    ELFSYM_EXPORT(ctx_radial_gradient),
    ELFSYM_EXPORT(ctx_rectangle),
    ELFSYM_EXPORT(ctx_rel_arc_to),
    ELFSYM_EXPORT(ctx_rel_curve_to),
    ELFSYM_EXPORT(ctx_rel_line_to),
    ELFSYM_EXPORT(ctx_rel_move_to),
    ELFSYM_EXPORT(ctx_rel_quad_to),
    ELFSYM_EXPORT(ctx_remove_idle),
    ELFSYM_EXPORT(ctx_render_ctx),
    ELFSYM_EXPORT(ctx_render_ctx),
    ELFSYM_EXPORT(ctx_render_ctx_textures),
    ELFSYM_EXPORT(ctx_resolve),
    ELFSYM_EXPORT(ctx_restore),
    ELFSYM_EXPORT(ctx_rgb),
    ELFSYM_EXPORT(ctx_rgba),
    ELFSYM_EXPORT(ctx_rgba8),
    ELFSYM_EXPORT(ctx_rgba_stroke),
    ELFSYM_EXPORT(ctx_rotate),
    ELFSYM_EXPORT(ctx_round_rectangle),
    ELFSYM_EXPORT(ctx_save),
    ELFSYM_EXPORT(ctx_scale),
    ELFSYM_EXPORT(ctx_scrolled),
    ELFSYM_EXPORT(ctx_set_clipboard),
    ELFSYM_EXPORT(ctx_set_drawlist),
    ELFSYM_EXPORT(ctx_set_fullscreen),
    ELFSYM_EXPORT(ctx_set_size),
    ELFSYM_EXPORT(ctx_set_texture_cache),
    ELFSYM_EXPORT(ctx_set_texture_source),
    ELFSYM_EXPORT(ctx_set_transform),
    ELFSYM_EXPORT(ctx_start_frame),
    ELFSYM_EXPORT(ctx_start_group),
    ELFSYM_EXPORT(ctx_strhash),
    ELFSYM_EXPORT(ctx_string_append_byte),
    ELFSYM_EXPORT(ctx_string_append_data),
    ELFSYM_EXPORT(ctx_string_append_printf),
    ELFSYM_EXPORT(ctx_string_append_str),
    ELFSYM_EXPORT(ctx_string_dissolve),
    ELFSYM_EXPORT(ctx_string_free),
    ELFSYM_EXPORT(ctx_string_new),
    ELFSYM_EXPORT(ctx_string_set),
    ELFSYM_EXPORT(ctx_stroke),
    ELFSYM_EXPORT(ctx_stroke_source),
    ELFSYM_EXPORT(ctx_text),
    ELFSYM_EXPORT(ctx_text_align),
    ELFSYM_EXPORT(ctx_text_baseline),
    ELFSYM_EXPORT(ctx_text_direction),
    ELFSYM_EXPORT(ctx_text_stroke),
    ELFSYM_EXPORT(ctx_texture),
    ELFSYM_EXPORT(ctx_texture_load),
    ELFSYM_EXPORT(ctx_text_width),
    ELFSYM_EXPORT(ctx_thaw),
    ELFSYM_EXPORT(ctx_ticks),
    ELFSYM_EXPORT(ctx_ticks),
    ELFSYM_EXPORT(ctx_translate),
    ELFSYM_EXPORT(ctx_utf8_len),
    ELFSYM_EXPORT(ctx_utf8_strlen),
    ELFSYM_EXPORT(ctx_view_box),
    ELFSYM_EXPORT(ctx_width),
    ELFSYM_EXPORT(ctx_wrap_left),
    ELFSYM_EXPORT(ctx_wrap_right),
    ELFSYM_EXPORT(ctx_x),
    ELFSYM_EXPORT(ctx_y),
    ELFSYM_EXPORT(difftime),
    ELFSYM_EXPORT(difftime),
    ELFSYM_EXPORT(ctx_set_pixels),
    ELFSYM_EXPORT(div),
    ELFSYM_EXPORT(div),
    ELFSYM_EXPORT(exit),
    ELFSYM_EXPORT(exp10),
    ELFSYM_EXPORT(exp10f),
    ELFSYM_EXPORT(exp2),
    ELFSYM_EXPORT(exp2f),
    ELFSYM_EXPORT(exp),
    ELFSYM_EXPORT(expf),
    ELFSYM_EXPORT(expm1),
    ELFSYM_EXPORT(expm1f),
    ELFSYM_EXPORT(fabs),
    ELFSYM_EXPORT(fabsf),
    {"fclose", &run_fclose},
    ELFSYM_EXPORT(fcntl),
    ELFSYM_EXPORT(fdim),
    ELFSYM_EXPORT(fdimf),
    {"fdopen", &run_fdopen},
    ELFSYM_EXPORT(feof),
    ELFSYM_EXPORT(ferror),
    {"fflush", &run_fflush},
    {"fgetc", &run_fgetc},
    {"fgetpos", &run_fgetpos},
    {"fgets", &run_fgets},
    ELFSYM_EXPORT(fileno),
    ELFSYM_EXPORT(finite),
    ELFSYM_EXPORT(finitef),
    ELFSYM_EXPORT(floor),
    ELFSYM_EXPORT(floorf),
    ELFSYM_EXPORT(fmod),
    ELFSYM_EXPORT(fmodf),
    { "fopen",   &run_fopen},
    { "fprintf", &run_fprintf},
    { "fputs",   &run_fputs},
    { "fputc",   &run_fputc},
    { "fread",   &run_fread},
    ELFSYM_EXPORT(freopen),
    ELFSYM_EXPORT(frexp),
    ELFSYM_EXPORT(frexpf),
    ELFSYM_EXPORT(fscanf),
    ELFSYM_EXPORT(fscanf),
    {"fseek", &run_fseek},
    ELFSYM_EXPORT(fseeko),
    {"fsetpos", &run_fsetpos},
    {"fstat", &run_fstat},
    ELFSYM_EXPORT(fsync),
    {"ftell", &run_ftell},
    {"ftello", &run_ftello},
    ELFSYM_EXPORT(ftruncate),

    { "fwrite",  &run_fwrite},

    ELFSYM_EXPORT(getaddrinfo),
    {"getc", &run_getc},
    {"getchar", &run_getchar},
    ELFSYM_EXPORT(getcwd),
    ELFSYM_EXPORT(getenv),
    ELFSYM_EXPORT(gethostbyname),
    //ELFSYM_EXPORT(getline),
    { "getline",  &run_getline},
    ELFSYM_EXPORT(getopt),
    { "getpid", &run_getpid},
    { "getppid", &run_getppid},
    ELFSYM_EXPORT(getsockname),
    ELFSYM_EXPORT(gettimeofday),
    ELFSYM_EXPORT(gmtime),
    ELFSYM_EXPORT(gmtime_r),
    ELFSYM_EXPORT(hypot),
    ELFSYM_EXPORT(hypotf),
    ELFSYM_EXPORT(ilogbf),
    ELFSYM_EXPORT(index),
    ELFSYM_EXPORT(ioctl),
    ELFSYM_EXPORT(isalnum),
    ELFSYM_EXPORT(isalpha),
    ELFSYM_EXPORT(isascii),
    ELFSYM_EXPORT(isatty),
    ELFSYM_EXPORT(isblank),
    ELFSYM_EXPORT(iscntrl),
    ELFSYM_EXPORT(isdigit),
    ELFSYM_EXPORT(isgraph),
    ELFSYM_EXPORT(islower),
    ELFSYM_EXPORT(isprint),
    ELFSYM_EXPORT(ispunct),
    ELFSYM_EXPORT(isspace),
    ELFSYM_EXPORT(isupper),
    ELFSYM_EXPORT(j0),
    ELFSYM_EXPORT(j0f),
    ELFSYM_EXPORT(j1),
    ELFSYM_EXPORT(j1f),
    ELFSYM_EXPORT(kill),
    ELFSYM_EXPORT(ldexp),
    ELFSYM_EXPORT(ldexpf),
    ELFSYM_EXPORT(ldiv),
    ELFSYM_EXPORT(lgamma),
    ELFSYM_EXPORT(lgammaf),
    ELFSYM_EXPORT(link),
    ELFSYM_EXPORT(listen),
    ELFSYM_EXPORT(lldiv),
    ELFSYM_EXPORT(localeconv),
    ELFSYM_EXPORT(localtime),
    ELFSYM_EXPORT(localtime_r),
    ELFSYM_EXPORT(log10),
    ELFSYM_EXPORT(log10f),
    ELFSYM_EXPORT(log),
    ELFSYM_EXPORT(logf),
    ELFSYM_EXPORT(longjmp),
    ELFSYM_EXPORT(lrintf),
    ELFSYM_EXPORT(lroundf),
    {"lseek", run_lseek},
    ELFSYM_EXPORT(memccpy),
    ELFSYM_EXPORT(memchr),
    ELFSYM_EXPORT(memcmp),
    ELFSYM_EXPORT(memcpy),
    ELFSYM_EXPORT(memmem),
    ELFSYM_EXPORT(memmove),
    ELFSYM_EXPORT(mempcpy),
    ELFSYM_EXPORT(memset),
    ELFSYM_EXPORT(mkdir),
    ELFSYM_EXPORT(mkdtemp),
    ELFSYM_EXPORT(mkostemp),
    ELFSYM_EXPORT(mkstemp),
  //ELFSYM_EXPORT(mktemp),
    ELFSYM_EXPORT(mktime),
    ELFSYM_EXPORT(fmod),
    ELFSYM_EXPORT(fmodf),
    ELFSYM_EXPORT(modf),
    ELFSYM_EXPORT(modff),
    ELFSYM_EXPORT(nan),
    ELFSYM_EXPORT(nanf),
    ELFSYM_EXPORT(nearbyint),
    ELFSYM_EXPORT(nearbyintf),
    ELFSYM_EXPORT(nextafter),
    ELFSYM_EXPORT(nextafterf),
    ELFSYM_EXPORT(nexttoward),
    ELFSYM_EXPORT(nexttowardf),
    ELFSYM_EXPORT(open),
    {"opendir", &run_opendir},
#if CTX_FLOW3R
    ELFSYM_EXPORT(pcTaskGetName),
#endif

    ELFSYM_EXPORT(perror),
    ELFSYM_EXPORT(poll),
    ELFSYM_EXPORT(posix_memalign),
    ELFSYM_EXPORT(pow),
    ELFSYM_EXPORT(powf),
    { "printf",  &run_printf},


    ELFSYM_EXPORT(pthread_attr_destroy),
    ELFSYM_EXPORT(pthread_attr_init),
    ELFSYM_EXPORT(pthread_attr_setstacksize),
    ELFSYM_EXPORT(pthread_cancel),
    ELFSYM_EXPORT(pthread_cond_broadcast),
    ELFSYM_EXPORT(pthread_cond_destroy),
    ELFSYM_EXPORT(pthread_cond_init),
    ELFSYM_EXPORT(pthread_cond_signal),
    ELFSYM_EXPORT(pthread_cond_timedwait),
    ELFSYM_EXPORT(pthread_cond_wait),
    ELFSYM_EXPORT(pthread_create),
    ELFSYM_EXPORT(pthread_detach),
    ELFSYM_EXPORT(pthread_equal),
    ELFSYM_EXPORT(pthread_exit),
    ELFSYM_EXPORT(pthread_getspecific),
    ELFSYM_EXPORT(pthread_join),
    ELFSYM_EXPORT(pthread_key_create),
    ELFSYM_EXPORT(pthread_key_delete),
    ELFSYM_EXPORT(pthread_mutexattr_destroy),
    ELFSYM_EXPORT(pthread_mutexattr_gettype),
    ELFSYM_EXPORT(pthread_mutexattr_init),
    ELFSYM_EXPORT(pthread_mutexattr_settype),
    ELFSYM_EXPORT(pthread_mutex_destroy),
    ELFSYM_EXPORT(pthread_mutex_init),
    ELFSYM_EXPORT(pthread_mutex_lock),
    ELFSYM_EXPORT(pthread_mutex_timedlock),
    ELFSYM_EXPORT(pthread_mutex_trylock),
    ELFSYM_EXPORT(pthread_mutex_unlock),
    ELFSYM_EXPORT(pthread_once),
    ELFSYM_EXPORT(pthread_self),
    ELFSYM_EXPORT(pthread_setcancelstate),
    ELFSYM_EXPORT(pthread_setspecific),
    { "putc",    &run_fputc},
    { "putchar", &run_putchar},
    ELFSYM_EXPORT(putenv),
    { "puts",    &run_puts},


    ELFSYM_EXPORT(qsort),
    ELFSYM_EXPORT(raise),
    ELFSYM_EXPORT(rand),
    ELFSYM_EXPORT(random),
    {"read", &run_read},
    {"readdir", &run_readdir},
    ELFSYM_EXPORT(readdir_r),
    ELFSYM_EXPORT(realloc),
    ELFSYM_EXPORT(realpath),
    ELFSYM_EXPORT(recv),
    ELFSYM_EXPORT(recvfrom),
    ELFSYM_EXPORT(recvmsg),
    ELFSYM_EXPORT(remove),
    ELFSYM_EXPORT(rename),
    {"rewind", &run_rewind},
    ELFSYM_EXPORT(rindex),
    ELFSYM_EXPORT(rmdir),
    ELFSYM_EXPORT(run_ctx_destroy),
    ELFSYM_EXPORT(run_ctx_new),
    ELFSYM_EXPORT(run_chdir),
    ELFSYM_EXPORT(run_fgets),
    ELFSYM_EXPORT(run_fprintf),
    ELFSYM_EXPORT(run_fputs),
    ELFSYM_EXPORT(run_fputc),
    ELFSYM_EXPORT(run_getcwd),
    ELFSYM_EXPORT(run_inline_main),
    ELFSYM_EXPORT(run_printf),
    ELFSYM_EXPORT(run_putchar),
    ELFSYM_EXPORT(run_puts),
    ELFSYM_EXPORT(run_write),
    ELFSYM_EXPORT(runs),
    ELFSYM_EXPORT(runv),
    ELFSYM_EXPORT(runvp),
    ELFSYM_EXPORT(scalbln),
    ELFSYM_EXPORT(scalblnf),
    ELFSYM_EXPORT(scalbn),
    ELFSYM_EXPORT(scalbnf),
    ELFSYM_EXPORT(scanf),
    ELFSYM_EXPORT(scanf),
    ELFSYM_EXPORT(sched_yield),
    ELFSYM_EXPORT(seekdir),
    ELFSYM_EXPORT(select),
    ELFSYM_EXPORT(sendmsg),
    ELFSYM_EXPORT(sendto),
    ELFSYM_EXPORT(setbuf),
    ELFSYM_EXPORT(setbuffer),
    ELFSYM_EXPORT(setenv),
    ELFSYM_EXPORT(setjmp),
    ELFSYM_EXPORT(setlinebuf),
    ELFSYM_EXPORT(setlocale),
    ELFSYM_EXPORT(settimeofday),
    ELFSYM_EXPORT(setvbuf),
    { "signal",   &run_signal},
    ELFSYM_EXPORT(shutdown),
    ELFSYM_EXPORT(sincos),
    ELFSYM_EXPORT(sincosf),
    ELFSYM_EXPORT(sin),
    ELFSYM_EXPORT(sinf),
    ELFSYM_EXPORT(sinh),
    ELFSYM_EXPORT(sinhf),
    ELFSYM_EXPORT(snprintf),
    ELFSYM_EXPORT(socket),
    ELFSYM_EXPORT(sprintf),
    ELFSYM_EXPORT(sprintf),
    ELFSYM_EXPORT(sqrt),
    ELFSYM_EXPORT(sqrtf),
    ELFSYM_EXPORT(srand),
    ELFSYM_EXPORT(srandom),
    ELFSYM_EXPORT(sscanf),
    {"stat", run_stat},
    ELFSYM_EXPORT(stpcpy),
    ELFSYM_EXPORT(stpncpy),
    ELFSYM_EXPORT(strcasecmp),
    ELFSYM_EXPORT(strcasestr),
    ELFSYM_EXPORT(strcat),
    ELFSYM_EXPORT(strchr),
    ELFSYM_EXPORT(strchrnul),
    ELFSYM_EXPORT(strcmp),
    ELFSYM_EXPORT(strcoll),
    ELFSYM_EXPORT(strcpy),
    ELFSYM_EXPORT(strcspn),
    ELFSYM_EXPORT(strdup),
    ELFSYM_EXPORT(strerror),
    ELFSYM_EXPORT(strerror_r),
    ELFSYM_EXPORT(strftime),
    //ELFSYM_EXPORT(strlcat),
    //ELFSYM_EXPORT(strlcpy),
    ELFSYM_EXPORT(strlen),
    ELFSYM_EXPORT(strncasecmp),
    ELFSYM_EXPORT(strncat),
    ELFSYM_EXPORT(strncmp),
    ELFSYM_EXPORT(strncpy),
    ELFSYM_EXPORT(strndup),
    ELFSYM_EXPORT(strnlen),
    ELFSYM_EXPORT(strpbrk),
    ELFSYM_EXPORT(strptime),
    ELFSYM_EXPORT(strrchr),
    ELFSYM_EXPORT(strsep),
    ELFSYM_EXPORT(strsignal),
    ELFSYM_EXPORT(strsignal),
    ELFSYM_EXPORT(strspn),
    ELFSYM_EXPORT(strstr),
    ELFSYM_EXPORT(strtod),
    ELFSYM_EXPORT(strtof),
    ELFSYM_EXPORT(strtok),
    ELFSYM_EXPORT(strtok_r),
    ELFSYM_EXPORT(strtol),
    ELFSYM_EXPORT(strtoll),
    ELFSYM_EXPORT(strtoul),
    ELFSYM_EXPORT(strtoull),
    ELFSYM_EXPORT(strverscmp),
    ELFSYM_EXPORT(strxfrm),
    ELFSYM_EXPORT(swab),
    ELFSYM_EXPORT(sysconf),
    ELFSYM_EXPORT(tan),
    ELFSYM_EXPORT(tanf),
    ELFSYM_EXPORT(tanh),
    ELFSYM_EXPORT(tanhf),
    ELFSYM_EXPORT(tcdrain),
    ELFSYM_EXPORT(tcflush),
    ELFSYM_EXPORT(tcgetattr),
    ELFSYM_EXPORT(tcsetattr),
    ELFSYM_EXPORT(telldir),
    ELFSYM_EXPORT(tgamma),
    ELFSYM_EXPORT(tgammaf),
    ELFSYM_EXPORT(time),
    ELFSYM_EXPORT(tmpfile),
    ELFSYM_EXPORT(tolower),
    ELFSYM_EXPORT(toupper),
    ELFSYM_EXPORT(truncate),
    ELFSYM_EXPORT(truncf),
    ELFSYM_EXPORT(tzset),
#if EXPORT_UI
    ELFSYM_EXPORT(ui_basename),
    ELFSYM_EXPORT(ui_button),
    ELFSYM_EXPORT(ui_button_coords),
    ELFSYM_EXPORT(ui_cb_do),
    ELFSYM_EXPORT(ui_ctx),
    ELFSYM_EXPORT(ui_destroy),
    ELFSYM_EXPORT(ui_do),
    ELFSYM_EXPORT(ui_draw_bg),
    ELFSYM_EXPORT(ui_end_frame),
    ELFSYM_EXPORT(ui_entry),
    ELFSYM_EXPORT(ui_entry_coords),
    ELFSYM_EXPORT(ui_find_executable),
    ELFSYM_EXPORT(ui_focus_next),
    ELFSYM_EXPORT(ui_focus_prev),
    ELFSYM_EXPORT(ui_get_data),
    ELFSYM_EXPORT(ui_get_font_size),
    ELFSYM_EXPORT(ui_get_mime_type),
    ELFSYM_EXPORT(ui_has_magic),
    ELFSYM_EXPORT(ui_host),
    ELFSYM_EXPORT(ui_iteration),
    ELFSYM_EXPORT(ui_keyboard),
    ELFSYM_EXPORT(ui_keyboard_visible),
    ELFSYM_EXPORT(ui_load_file),
    ELFSYM_EXPORT(ui_main),
    ELFSYM_EXPORT(ui_move_to),
    ELFSYM_EXPORT(ui_new),
    ELFSYM_EXPORT(ui_pop_fun),
    ELFSYM_EXPORT(ui_push_fun),
    ELFSYM_EXPORT(ui_register_magic),
    ELFSYM_EXPORT(ui_register_view),
    ELFSYM_EXPORT(ui_scroll_to),
    ELFSYM_EXPORT(ui_set_data),
    ELFSYM_EXPORT(ui_set_scroll_offset),
    ELFSYM_EXPORT(ui_slider),
    ELFSYM_EXPORT(ui_slider_coords),
    ELFSYM_EXPORT(ui_seperator),
    ELFSYM_EXPORT(ui_start_frame),
    ELFSYM_EXPORT(ui_text),
    ELFSYM_EXPORT(ui_title),
    ELFSYM_EXPORT(ui_toggle),
    ELFSYM_EXPORT(ui_x),
    ELFSYM_EXPORT(ui_y),
#endif
    {"ungetc", &run_ungetc},
    ELFSYM_EXPORT(unlink),
    ELFSYM_EXPORT(unsetenv),
    ELFSYM_EXPORT(usleep),
#if CTX_FLOW3R
    ELFSYM_EXPORT(uxTaskGetTaskNumber),
#endif
    ELFSYM_EXPORT(vasprintf),
    ELFSYM_EXPORT(vdprintf),
    ELFSYM_EXPORT(vfscanf),
    ELFSYM_EXPORT(vprintf),
    ELFSYM_EXPORT(vscanf),
    ELFSYM_EXPORT(vsnprintf),
    ELFSYM_EXPORT(vsprintf),
    ELFSYM_EXPORT(vsscanf),
#if CTX_FLOW3R
    ELFSYM_EXPORT(vTaskDelay),
#endif
#if CTX_FLOW3R
    ELFSYM_EXPORT(wifi_init_sta),
    ELFSYM_EXPORT(wifi_scan),
#endif
    { "write",   &run_write},
#if CTX_FLOW3R
    ELFSYM_EXPORT(xTaskGetCurrentTaskHandle),
    ELFSYM_EXPORT(xTaskCreatePinnedToCore),
    ELFSYM_EXPORT(xTaskCreate),
#endif

    ELFSYM_EXPORT(y0),
    ELFSYM_EXPORT(y0f),
    ELFSYM_EXPORT(y1),
    ELFSYM_EXPORT(y1f),

    ELFSYM_END
};

const void *run_sym(const char *name)
{
  for (int i = 0; g_customer_elfsyms[i].name; i++)
   if (!strcmp (g_customer_elfsyms[i].name, name))
    return g_customer_elfsyms[i].sym;
//printf ("symbol %s not found\n", name);
  return NULL;
}
