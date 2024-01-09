#include "port_config.h"
/* this file lists all the symbols available for dynamic resolution
 * by the elf-loader
 *
 */

#undef strstr
#undef strlen
#if S0IL_NATIVE
#include <poll.h>
#endif

#ifdef S0IL_DEFINES
#undef S0IL_DEFINES
#endif

#define EXPORT_UI 1

#include "s0il.h"
#include <fenv.h>

extern float __divsf3(float a, float b);
extern double __floatsidf(int a);
double __muldf3(double a, double b);
float __truncdfsf2(double a);
void __extendsfdf2(void);
void __floatundisf(void);
void __fixdfsi(void);
void __adddf3(void);
void __subdf3(void);
void __eqdf2(void);
void __nedf2(void);
void __ledf2(void);
void __gedf2(void);
void __lshrdi3(void);
void __udivdi3(void);
void __umoddi3(void);
void __moddi3(void);
void __gtdf2(void);
void __ltdf2(void);
void __unorddf2(void);
void __floatdidf(void);
void __fixdfdi(void);
void __divdi3(void);
void __floatundidf(void);
#if 0
void __atomic_fetch_add_8(void);
void __atomic_fetch_sub_8(void);
void __atomic_fetch_and_8(void);
void __atomic_fetch_or_8(void);
void __atomic_fetch_xor_8(void);
void __atomic_exchange_8(void);
#endif

int main_bundled(int argc, char **argv);
void ctx_set_pixels(Ctx *ctx, void *userdata, int x, int y, int w, int h,
                    void *buf);

extern int ctx_osk_mode;
int wifi_init_sta(const char *ssid_arg, const char *password_arg);
char **wifi_scan(void);

#define ELFSYM_EXPORT(_sym)                                                    \
  { #_sym, &_sym }
#define ELFSYM_END                                                             \
  { NULL, NULL }

struct esp_elfsym {
  const char *name;
  void *sym;
};

Ctx *ctx_host(void);
void __xpg_strerror_r(void);

void mbedtls_entropy_init(void);
void mbedtls_entropy_free(void);
void mbedtls_ctr_drbg_seed(void);
void mbedtls_ctr_drbg_free(void);
void mbedtls_ctr_drbg_init(void);
void mbedtls_ctr_drbg_random(void);
void mbedtls_ctr_drbg_reseed(void);
void mbedtls_ctr_drbg_set_prediction_resistance(void);
void mbedtls_entropy_func(void);
#if !defined(CTX_ESP)
void mbedtls_mpi_fill_random(void);
void mbedtls_ecp_curve_info_from_grp_id(void);
void mbedtls_ecp_curve_info_from_name(void);
void mbedtls_ecp_curve_list(void);
void mbedtls_ecp_gen_key(void);
void mbedtls_entropy_func(void);
void mbedtls_mpi_free(void);
void mbedtls_mpi_init(void);
void mbedtls_mpi_write_file(void);
void mbedtls_pk_free(void);
void mbedtls_pk_get_type(void);
void mbedtls_pk_info_from_type(void);
void mbedtls_pk_init(void);
void mbedtls_pk_setup(void);
void mbedtls_pk_write_key_der(void);
void mbedtls_pk_write_key_pem(void);
void mbedtls_rsa_export(void);
void mbedtls_rsa_export_crt(void);
void mbedtls_rsa_gen_key(void);
void mbedtls_strerror(void);
void mbedtls_cipher_free(void);
void mbedtls_aes_context(void);
// void mbedtls_chacha20_context(void);
void mbedtls_chacha20_free(void);
void mbedtls_chacha20_init(void);
void mbedtls_chacha20_setkey(void);
void mbedtls_chacha20_starts(void);
void mbedtls_chacha20_update(void);
void mbedtls_cipher_finish(void);
void mbedtls_cipher_free(void);
void mbedtls_cipher_info_from_type(void);
void mbedtls_cipher_init(void);
void mbedtls_cipher_reset(void);
void mbedtls_cipher_set_iv(void);
void mbedtls_cipher_setkey(void);
void mbedtls_cipher_set_padding_mode(void);
void mbedtls_cipher_setup(void);
void mbedtls_cipher_update(void);
void mbedtls_ecdh_compute_shared(void);
void mbedtls_ecdsa_free(void);
void mbedtls_ecdsa_from_keypair(void);
void mbedtls_ecdsa_init(void);
void mbedtls_ecdsa_sign(void);
void mbedtls_ecdsa_verify(void);
void mbedtls_ecp_copy(void);
void mbedtls_ecp_gen_keypair(void);
void mbedtls_ecp_group_copy(void);
void mbedtls_ecp_group_free(void);
void mbedtls_ecp_group_init(void);
void mbedtls_ecp_group_load(void);
void mbedtls_ecp_keypair_free(void);
void mbedtls_ecp_keypair_init(void);
void mbedtls_ecp_point_free(void);
void mbedtls_ecp_point_init(void);
void mbedtls_ecp_point_read_binary(void);
void mbedtls_ecp_point_write_binary(void);
void mbedtls_gcm_auth_decrypt(void);
void mbedtls_gcm_crypt_and_tag(void);
void mbedtls_gcm_free(void);
void mbedtls_gcm_init(void);
void mbedtls_gcm_setkey(void);
void mbedtls_md(void);
void mbedtls_md_finish(void);
void mbedtls_md_free(void);
void mbedtls_md_hmac_finish(void);
void mbedtls_md_hmac_starts(void);
void mbedtls_md_hmac_update(void);
void mbedtls_md_info_from_type(void);
void mbedtls_md_init(void);
void mbedtls_md_setup(void);
void mbedtls_md_starts(void);
void mbedtls_md_update(void);
void mbedtls_mpi_bitlen(void);
void mbedtls_mpi_cmp_mpi(void);
void mbedtls_mpi_copy(void);
void mbedtls_mpi_exp_mod(void);
void mbedtls_mpi_get_bit(void);
void mbedtls_mpi_lset(void);
void mbedtls_mpi_mod_mpi(void);
void mbedtls_mpi_read_binary(void);
void mbedtls_mpi_read_string(void);
void mbedtls_mpi_set_bit(void);
void mbedtls_mpi_size(void);
void mbedtls_mpi_sub_mpi(void);
void mbedtls_mpi_write_binary(void);
void mbedtls_pk_can_do(void);
void mbedtls_pk_get_bitlen(void);
void mbedtls_pk_get_type(void);
void mbedtls_pk_parse_key(void);
void mbedtls_pk_sign(void);
void mbedtls_pk_verify(void);
void mbedtls_poly1305_finish(void);
void mbedtls_poly1305_free(void);
void mbedtls_poly1305_init(void);
void mbedtls_poly1305_starts(void);
void mbedtls_poly1305_update(void);
void mbedtls_rsa_check_privkey(void);
void mbedtls_rsa_complete(void);
void mbedtls_rsa_export(void);
void mbedtls_rsa_gen_key(void);
void mbedtls_rsa_import(void);
void mbedtls_strerror(void);
void mbedtls_rsa_import_raw(void);
#endif

const struct esp_elfsym g_customer_elfsyms[] = {

// ELFSYM_EXPORT(dup),
// ELFSYM_EXPORT(dup2),
#ifndef S0IL_NATIVE
    ELFSYM_EXPORT(__adddf3),
#if !defined(EMSCRIPTEN) && !defined(PICO_BUILD)
    ELFSYM_EXPORT(__assert_func),
#endif
    ELFSYM_EXPORT(__divsf3),
    ELFSYM_EXPORT(__extendsfdf2),
    ELFSYM_EXPORT(__eqdf2),
    ELFSYM_EXPORT(__fixdfsi),
    ELFSYM_EXPORT(__floatsidf),
    ELFSYM_EXPORT(__floatundisf),
    ELFSYM_EXPORT(__floatdidf),
    ELFSYM_EXPORT(__gedf2),
    ELFSYM_EXPORT(__ledf2),
    ELFSYM_EXPORT(__lshrdi3),
    ELFSYM_EXPORT(__muldf3),
    ELFSYM_EXPORT(__nedf2),
    ELFSYM_EXPORT(__subdf3),
    ELFSYM_EXPORT(__truncdfsf2),
    ELFSYM_EXPORT(__udivdi3),
    ELFSYM_EXPORT(__umoddi3),
    ELFSYM_EXPORT(__moddi3),
    ELFSYM_EXPORT(__unorddf2),
    ELFSYM_EXPORT(__eqdf2),
    ELFSYM_EXPORT(__ltdf2),
    ELFSYM_EXPORT(__gtdf2),
    ELFSYM_EXPORT(__nedf2),
    ELFSYM_EXPORT(__divdi3),
    ELFSYM_EXPORT(__fixdfdi),
    ELFSYM_EXPORT(__floatundidf),
#if 0
    ELFSYM_EXPORT(__atomic_fetch_add_8),
    ELFSYM_EXPORT(__atomic_fetch_and_8),
    ELFSYM_EXPORT(__atomic_fetch_or_8),
    ELFSYM_EXPORT(__atomic_fetch_xor_8),
    ELFSYM_EXPORT(__atomic_exchange_8),
    ELFSYM_EXPORT(__atomic_fetch_sub_8),
#endif

#endif
    {"ctx_destroy", &s0il_ctx_destroy},
    {"ctx_new", &s0il_ctx_new},
    // ELFSYM_EXPORT(fesetround),
    ELFSYM_EXPORT(abort),
    ELFSYM_EXPORT(lrint),
    ELFSYM_EXPORT(fmax),
    ELFSYM_EXPORT(fmin),
    ELFSYM_EXPORT(abs),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(accept),
#endif
    {"access", &s0il_access},
    ELFSYM_EXPORT(acos),
    ELFSYM_EXPORT(acosf),
    ELFSYM_EXPORT(aligned_alloc),
    ELFSYM_EXPORT(asctime),
    ELFSYM_EXPORT(asctime_r),
    ELFSYM_EXPORT(asin),
    ELFSYM_EXPORT(round),
    ELFSYM_EXPORT(asinf),
    // ELFSYM_EXPORT(asprintf),
    ELFSYM_EXPORT(atan2),
    ELFSYM_EXPORT(atan2f),
    ELFSYM_EXPORT(atan),
    ELFSYM_EXPORT(atanf),
    ELFSYM_EXPORT(atanf),
    ELFSYM_EXPORT(atof),
    ELFSYM_EXPORT(atoi),
    ELFSYM_EXPORT(atol),
    ELFSYM_EXPORT(atoll),
    {"basename", &ui_basename},
    ELFSYM_EXPORT(bcmp),
    ELFSYM_EXPORT(bcopy),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(bind),
    ELFSYM_EXPORT(setsockopt),
#endif

#if CTX_FLOW3R
    ELFSYM_EXPORT(bsp_captouch_angle),
    ELFSYM_EXPORT(bsp_captouch_angular),
    ELFSYM_EXPORT(bsp_captouch_down),
    ELFSYM_EXPORT(bsp_captouch_radial),
#endif
    ELFSYM_EXPORT(main_bundled),
    ELFSYM_EXPORT(bzero),
    {"calloc", &s0il_calloc},
    ELFSYM_EXPORT(cbrt),
    ELFSYM_EXPORT(cbrtf),
    ELFSYM_EXPORT(ceil),
    ELFSYM_EXPORT(ceilf),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(cfgetispeed),
    ELFSYM_EXPORT(cfgetospeed),
    ELFSYM_EXPORT(cfsetispeed),
    ELFSYM_EXPORT(cfsetospeed),
#endif

    {"chdir", &s0il_chdir},
    ELFSYM_EXPORT(clearerr),
    {"clearenv", &s0il_clearenv},
    ELFSYM_EXPORT(clock),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(clock_gettime),
    ELFSYM_EXPORT(clock_settime),
#endif
    {"close", &s0il_close},
    {"closedir", s0il_closedir},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(connect),
#endif
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
    // ELFSYM_EXPORT(ctx_guess_media_type),
    ELFSYM_EXPORT(ctx_handle_events),
    ELFSYM_EXPORT(ctx_has_exited),
    ELFSYM_EXPORT(ctx_height),
    ELFSYM_EXPORT(ctx_host),
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
    // ELFSYM_EXPORT(ctx_path_get_media_type),
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
    ELFSYM_EXPORT(ctx_exit),
    ELFSYM_EXPORT(ctx_radial_gradient),
    ELFSYM_EXPORT(ctx_rectangle),
    ELFSYM_EXPORT(ctx_rel_arc_to),
    ELFSYM_EXPORT(ctx_rel_curve_to),
    ELFSYM_EXPORT(ctx_rel_line_to),
    ELFSYM_EXPORT(ctx_rel_move_to),
    ELFSYM_EXPORT(ctx_rel_quad_to),
    ELFSYM_EXPORT(ctx_reset_has_exited),
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
    {"exit", &s0il_exit},
    // ELFSYM_EXPORT(exp10),
    // ELFSYM_EXPORT(exp10f),
    ELFSYM_EXPORT(exp2),
    ELFSYM_EXPORT(exp2f),
    ELFSYM_EXPORT(exp),
    ELFSYM_EXPORT(expf),
    ELFSYM_EXPORT(expm1),
    ELFSYM_EXPORT(expm1f),
    ELFSYM_EXPORT(fabs),
    ELFSYM_EXPORT(fabsf),
    {"fclose", &s0il_fclose},
    {"pclose", &s0il_pclose},
    ELFSYM_EXPORT(fcntl),
    ELFSYM_EXPORT(fdim),
    ELFSYM_EXPORT(fdimf),
    {"fdopen", &s0il_fdopen},
    ELFSYM_EXPORT(feof),
    ELFSYM_EXPORT(ferror),
    {"fflush", &s0il_fflush},
    {"fgetc", &s0il_fgetc},
    {"fgetpos", &s0il_fgetpos},
    {"fgets", &s0il_fgets},
    ELFSYM_EXPORT(fileno),
    ELFSYM_EXPORT(finite),
    ELFSYM_EXPORT(finitef),
    ELFSYM_EXPORT(floor),
    ELFSYM_EXPORT(floorf),
    ELFSYM_EXPORT(fmod),
    ELFSYM_EXPORT(fmodf),
    {"popen", &s0il_popen},
    {"fopen", &s0il_fopen},
    {"fprintf", &s0il_fprintf},
    {"fputs", &s0il_fputs},
    {"fputc", &s0il_fputc},
    {"fread", &s0il_fread},
    {"free", &s0il_free},
    {"freopen", &s0il_freopen},
    ELFSYM_EXPORT(frexp),
    ELFSYM_EXPORT(frexpf),
    ELFSYM_EXPORT(fscanf),
    ELFSYM_EXPORT(fscanf),
    {"fseek", &s0il_fseek},
    ELFSYM_EXPORT(fseeko),
    {"fsetpos", &s0il_fsetpos},
    {"fstat", &s0il_fstat},
    {"fsync", &s0il_fsync},
    {"ftell", &s0il_ftell},
    {"ftello", &s0il_ftello},
    {"ftruncate", &s0il_ftruncate},
    {"fwrite", &s0il_fwrite},

#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(getaddrinfo),
#endif
    {"getc", &s0il_getc},
    {"getchar", &s0il_getchar},
    {"getcwd", &s0il_getcwd},
    {"getenv", &s0il_getenv},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(gethostbyname),
#endif
    {"getline", &s0il_getline},
    ELFSYM_EXPORT(getopt),
    {"getpid", &s0il_getpid},
    {"getppid", &s0il_getppid},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(getsockname),
#endif
    ELFSYM_EXPORT(gettimeofday),
    ELFSYM_EXPORT(gmtime),
    ELFSYM_EXPORT(gmtime_r),
    ELFSYM_EXPORT(hypot),
    ELFSYM_EXPORT(hypotf),
    ELFSYM_EXPORT(ilogbf),
    ELFSYM_EXPORT(index),
    {"ioctl", &s0il_ioctl},
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
    {"kill", &s0il_kill},
    ELFSYM_EXPORT(ldexp),
    ELFSYM_EXPORT(ldexpf),
    ELFSYM_EXPORT(ldiv),
    ELFSYM_EXPORT(lgamma),
    ELFSYM_EXPORT(lgammaf),
    ELFSYM_EXPORT(link),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(listen),
#endif
    ELFSYM_EXPORT(lldiv),
    ELFSYM_EXPORT(localeconv),
    ELFSYM_EXPORT(localtime),
    ELFSYM_EXPORT(localtime_r),
    ELFSYM_EXPORT(log10),
    ELFSYM_EXPORT(log10f),
    ELFSYM_EXPORT(log),
    ELFSYM_EXPORT(logf),
    ELFSYM_EXPORT(log2f),
    ELFSYM_EXPORT(longjmp),
    ELFSYM_EXPORT(lrintf),
    ELFSYM_EXPORT(lroundf),
    {"lseek", s0il_lseek},
    {"malloc", &s0il_malloc},
    ELFSYM_EXPORT(memccpy),
    ELFSYM_EXPORT(memchr),
    ELFSYM_EXPORT(memcmp),
    ELFSYM_EXPORT(memcpy),
    // ELFSYM_EXPORT(memmem), // not in emscripten?
    ELFSYM_EXPORT(memmove),
    // ELFSYM_EXPORT(mempcpy),
    ELFSYM_EXPORT(memset),
    {"mkdir", &s0il_mkdir},
    ELFSYM_EXPORT(mkdtemp),
    // ELFSYM_EXPORT(mkostemp),
    ELFSYM_EXPORT(mkstemp),
    // ELFSYM_EXPORT(mktemp),
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
#if defined(CTX_ESP)
    ELFSYM_EXPORT(environ),
#endif

    {"open", &s0il_open},
    {"opendir", &s0il_opendir},
#if CTX_ESP
    ELFSYM_EXPORT(pcTaskGetName),
#endif

    ELFSYM_EXPORT(perror),
#if !defined(PICO_BUILD)
#ifndef EMSCRIPTEN
    ELFSYM_EXPORT(poll),
#endif
#endif
    ELFSYM_EXPORT(posix_memalign),
    ELFSYM_EXPORT(pow),
    ELFSYM_EXPORT(powf),
    {"printf", &s0il_printf},

#if !defined(PICO_BUILD)
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
#endif
    {"putc", &s0il_fputc},
    {"putchar", &s0il_putchar},
    {"putenv", &s0il_putenv},
    {"puts", &s0il_puts},

    ELFSYM_EXPORT(qsort),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(raise),
#endif
    ELFSYM_EXPORT(rand),
    ELFSYM_EXPORT(random),
    {"read", &s0il_read},
    {"readdir", &s0il_readdir},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(readdir_r),
#endif
    {"realloc", &s0il_realloc},
    {"realpath", &s0il_realpath},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(recv),
    ELFSYM_EXPORT(recvfrom),
    ELFSYM_EXPORT(recvmsg),
#endif
    {"remove", &s0il_remove},
    {"rename", &s0il_rename},
    {"rewind", &s0il_rewind},
    ELFSYM_EXPORT(rindex),
    {"rmdir", &s0il_rmdir},
    ELFSYM_EXPORT(s0il_ctx_destroy),
    ELFSYM_EXPORT(s0il_ctx_new),
    ELFSYM_EXPORT(s0il_chdir),
    ELFSYM_EXPORT(s0il_fgets),
    ELFSYM_EXPORT(s0il_fprintf),
    ELFSYM_EXPORT(s0il_fputs),
    ELFSYM_EXPORT(s0il_fputc),
    ELFSYM_EXPORT(s0il_getcwd),
    ELFSYM_EXPORT(s0il_bundle_main),
    ELFSYM_EXPORT(s0il_printf),
    ELFSYM_EXPORT(s0il_putchar),
    ELFSYM_EXPORT(s0il_puts),
    ELFSYM_EXPORT(s0il_write),
    {"system", &s0il_system},
    ELFSYM_EXPORT(s0il_runv),
    ELFSYM_EXPORT(s0il_runvp),
    ELFSYM_EXPORT(scalbln),
    ELFSYM_EXPORT(scalblnf),
    ELFSYM_EXPORT(scalbn),
    ELFSYM_EXPORT(scalbnf),
    ELFSYM_EXPORT(scanf),
    ELFSYM_EXPORT(scanf),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(sched_yield),
    ELFSYM_EXPORT(seekdir),
    ELFSYM_EXPORT(fesetround),
    ELFSYM_EXPORT(fegetround),
#endif
    {"select", &s0il_select},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(sendmsg),
    ELFSYM_EXPORT(sendto),
#endif
    ELFSYM_EXPORT(setbuf),
    ELFSYM_EXPORT(setbuffer),
    {"setenv", &s0il_setenv},
    // ELFSYM_EXPORT(setjmp),
    ELFSYM_EXPORT(setlinebuf),
    ELFSYM_EXPORT(setlocale),
#if !defined(EMSCRIPTEN)
    ELFSYM_EXPORT(settimeofday),
#endif
    ELFSYM_EXPORT(setvbuf),
    {"signal", &s0il_signal},
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(shutdown),
#endif
    // ELFSYM_EXPORT(sincos),
    // ELFSYM_EXPORT(sincosf),
    ELFSYM_EXPORT(sin),
    ELFSYM_EXPORT(sinf),
    ELFSYM_EXPORT(sinh),
    ELFSYM_EXPORT(sinhf),
    ELFSYM_EXPORT(snprintf),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(socket),
#endif
    ELFSYM_EXPORT(sprintf),
    ELFSYM_EXPORT(sprintf),
    ELFSYM_EXPORT(sqrt),
    ELFSYM_EXPORT(sqrtf),
    ELFSYM_EXPORT(srand),
    ELFSYM_EXPORT(srandom),
    ELFSYM_EXPORT(sscanf), // TODO : handle stdin
    {"stat", s0il_stat},
    ELFSYM_EXPORT(stpcpy),
    ELFSYM_EXPORT(stpncpy),
    ELFSYM_EXPORT(strcasecmp),
    // ELFSYM_EXPORT(strcasestr),
    ELFSYM_EXPORT(strcat),
    ELFSYM_EXPORT(strchr),
    // ELFSYM_EXPORT(strchrnul),
    ELFSYM_EXPORT(strcmp),
    ELFSYM_EXPORT(strcoll),
    ELFSYM_EXPORT(strcpy),
    ELFSYM_EXPORT(strcspn),
    {"strdup", &s0il_strdup},
    ELFSYM_EXPORT(strerror),
    ELFSYM_EXPORT(strerror_r),
    ELFSYM_EXPORT(strftime),
    // ELFSYM_EXPORT(strlcat),
    // ELFSYM_EXPORT(strlcpy),
    ELFSYM_EXPORT(strlen),
    ELFSYM_EXPORT(strncasecmp),
    ELFSYM_EXPORT(strncat),
    ELFSYM_EXPORT(strncmp),
    ELFSYM_EXPORT(strncpy),
    {"strndup", &s0il_strndup},
    ELFSYM_EXPORT(strnlen),
    ELFSYM_EXPORT(strpbrk),
    // ELFSYM_EXPORT(strptime),
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
    // ELFSYM_EXPORT(strverscmp),
    ELFSYM_EXPORT(strxfrm),
    // ELFSYM_EXPORT(swab),
    ELFSYM_EXPORT(sysconf),
    ELFSYM_EXPORT(tan),
    ELFSYM_EXPORT(tanf),
    ELFSYM_EXPORT(tanh),
    ELFSYM_EXPORT(tanhf),
#if !defined(PICO_BUILD)
    ELFSYM_EXPORT(tcdrain),
    ELFSYM_EXPORT(tcflush),
    ELFSYM_EXPORT(tcflow),
    ELFSYM_EXPORT(tcgetattr),
    ELFSYM_EXPORT(tcgetsid),
    ELFSYM_EXPORT(tcsetattr),

    {"telldir", &s0il_telldir},
#endif
    ELFSYM_EXPORT(tgamma),
    ELFSYM_EXPORT(tgammaf),
    ELFSYM_EXPORT(time),
    ELFSYM_EXPORT(tmpfile),
    ELFSYM_EXPORT(tolower),
    ELFSYM_EXPORT(toupper),
    {"truncate", &s0il_truncate},
    ELFSYM_EXPORT(truncf),
    ELFSYM_EXPORT(tzset),
#if EXPORT_UI
    ELFSYM_EXPORT(ui_add_key_binding),
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
    ELFSYM_EXPORT(ui_entry_realloc),
    ELFSYM_EXPORT(ui_entry_coords),
    ELFSYM_EXPORT(s0il_path_lookup),
    ELFSYM_EXPORT(ui_focus_next),
    ELFSYM_EXPORT(ui_focus_prev),
    ELFSYM_EXPORT(ui_get_data),
    ELFSYM_EXPORT(ui_get_font_size),
    ELFSYM_EXPORT(s0il_detect_media_path),
    ELFSYM_EXPORT(s0il_detect_media_sector512),
    ELFSYM_EXPORT(s0il_has_mime),
    ELFSYM_EXPORT(ui_host),
    ELFSYM_EXPORT(ui_iteration),
    ELFSYM_EXPORT(ui_keyboard),
    ELFSYM_EXPORT(ui_keyboard_visible),
    ELFSYM_EXPORT(s0il_load_file),
    ELFSYM_EXPORT(s0il_main),
    ELFSYM_EXPORT(ui_move_to),
    ELFSYM_EXPORT(ui_new),
    ELFSYM_EXPORT(ui_pop_fun),
    ELFSYM_EXPORT(ui_push_fun),
    ELFSYM_EXPORT(s0il_add_magic),
    ELFSYM_EXPORT(ui_add_view),
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
    {"ungetc", &s0il_ungetc},
    {"unlink", &s0il_unlink},
    {"unsetenv", &s0il_unsetenv},
    ELFSYM_EXPORT(usleep),
#if CTX_ESP
    ELFSYM_EXPORT(uxTaskGetTaskNumber),
#endif
    // ELFSYM_EXPORT(vasprintf),
    ELFSYM_EXPORT(vdprintf),
    ELFSYM_EXPORT(vfscanf),
    ELFSYM_EXPORT(vprintf),
    ELFSYM_EXPORT(vscanf),
    ELFSYM_EXPORT(vsnprintf),
    ELFSYM_EXPORT(vsprintf),
    ELFSYM_EXPORT(vsscanf),
#if CTX_ESP
    ELFSYM_EXPORT(vTaskDelay),
    ELFSYM_EXPORT(wifi_init_sta),
    ELFSYM_EXPORT(wifi_scan),
    ELFSYM_EXPORT(lwip_getaddrinfo),
    ELFSYM_EXPORT(lwip_freeaddrinfo),
    ELFSYM_EXPORT(lwip_inet_pton),
    ELFSYM_EXPORT(lwip_getsockopt),
    ELFSYM_EXPORT(lwip_setsockopt),
    ELFSYM_EXPORT(__xpg_strerror_r),
#endif
    {"write", &s0il_write},
#if CTX_ESP
    ELFSYM_EXPORT(xTaskGetCurrentTaskHandle),
    ELFSYM_EXPORT(xTaskCreatePinnedToCore),
    ELFSYM_EXPORT(xTaskCreate),
#endif

    ELFSYM_EXPORT(y0),
    ELFSYM_EXPORT(y0f),
    ELFSYM_EXPORT(y1),
    ELFSYM_EXPORT(y1f),

#if !defined(EMSCRIPTEN)
    // ELFSYM_EXPORT(mbedtls_aes_context),
    ELFSYM_EXPORT(mbedtls_cipher_free),
#if 1
    // ELFSYM_EXPORT(mbedtls_chacha20_context),
    ELFSYM_EXPORT(mbedtls_chacha20_free),
    ELFSYM_EXPORT(mbedtls_chacha20_init),
    ELFSYM_EXPORT(mbedtls_chacha20_setkey),
    ELFSYM_EXPORT(mbedtls_chacha20_starts),
    ELFSYM_EXPORT(mbedtls_chacha20_update),
#endif
    ELFSYM_EXPORT(mbedtls_cipher_finish),
    ELFSYM_EXPORT(mbedtls_cipher_free),
    ELFSYM_EXPORT(mbedtls_cipher_info_from_type),
    ELFSYM_EXPORT(mbedtls_cipher_init),
    ELFSYM_EXPORT(mbedtls_cipher_reset),
    ELFSYM_EXPORT(mbedtls_cipher_set_iv),
    ELFSYM_EXPORT(mbedtls_cipher_setkey),
    ELFSYM_EXPORT(mbedtls_cipher_set_padding_mode),
    ELFSYM_EXPORT(mbedtls_cipher_setup),
    ELFSYM_EXPORT(mbedtls_cipher_update),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_set_prediction_resistance),
    ELFSYM_EXPORT(mbedtls_ecdh_compute_shared),
    ELFSYM_EXPORT(mbedtls_ecdsa_free),
    ELFSYM_EXPORT(mbedtls_ecdsa_from_keypair),
    ELFSYM_EXPORT(mbedtls_ecdsa_init),
    ELFSYM_EXPORT(mbedtls_ecdsa_sign),
    ELFSYM_EXPORT(mbedtls_ecdsa_verify),
    ELFSYM_EXPORT(mbedtls_ecp_copy),
    ELFSYM_EXPORT(mbedtls_ecp_gen_keypair),
    ELFSYM_EXPORT(mbedtls_ecp_group_copy),
    ELFSYM_EXPORT(mbedtls_ecp_group_free),
    ELFSYM_EXPORT(mbedtls_ecp_group_init),
    ELFSYM_EXPORT(mbedtls_ecp_group_load),
    ELFSYM_EXPORT(mbedtls_ecp_keypair_free),
    ELFSYM_EXPORT(mbedtls_ecp_keypair_init),
    ELFSYM_EXPORT(mbedtls_ecp_point_free),
    ELFSYM_EXPORT(mbedtls_ecp_point_init),
    ELFSYM_EXPORT(mbedtls_ecp_point_read_binary),
    ELFSYM_EXPORT(mbedtls_ecp_point_write_binary),

    ELFSYM_EXPORT(mbedtls_entropy_func),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_free),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_init),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_reseed),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_random),
    ELFSYM_EXPORT(mbedtls_ctr_drbg_seed),
    ELFSYM_EXPORT(mbedtls_ecp_curve_info_from_grp_id),
    ELFSYM_EXPORT(mbedtls_ecp_curve_info_from_name),
    ELFSYM_EXPORT(mbedtls_ecp_curve_list),
    ELFSYM_EXPORT(mbedtls_ecp_gen_key),
    ELFSYM_EXPORT(mbedtls_entropy_free),
    ELFSYM_EXPORT(mbedtls_entropy_func),
    ELFSYM_EXPORT(mbedtls_entropy_init),
    ELFSYM_EXPORT(mbedtls_mpi_free),
    ELFSYM_EXPORT(mbedtls_mpi_init),
    ELFSYM_EXPORT(mbedtls_mpi_write_file),
    ELFSYM_EXPORT(mbedtls_pk_free),
    ELFSYM_EXPORT(mbedtls_pk_get_type),
    ELFSYM_EXPORT(mbedtls_pk_info_from_type),
    ELFSYM_EXPORT(mbedtls_pk_init),
    ELFSYM_EXPORT(mbedtls_pk_setup),
    ELFSYM_EXPORT(mbedtls_pk_write_key_der),
    ELFSYM_EXPORT(mbedtls_pk_write_key_pem),
    ELFSYM_EXPORT(mbedtls_rsa_export),
    ELFSYM_EXPORT(mbedtls_rsa_export_crt),
    ELFSYM_EXPORT(mbedtls_rsa_gen_key),
    ELFSYM_EXPORT(mbedtls_strerror),
    ELFSYM_EXPORT(mbedtls_gcm_auth_decrypt),
    ELFSYM_EXPORT(mbedtls_gcm_crypt_and_tag),
    ELFSYM_EXPORT(mbedtls_gcm_free),
    ELFSYM_EXPORT(mbedtls_gcm_init),
    ELFSYM_EXPORT(mbedtls_gcm_setkey),
    ELFSYM_EXPORT(mbedtls_md),
    ELFSYM_EXPORT(mbedtls_md_finish),
    ELFSYM_EXPORT(mbedtls_md_free),
    ELFSYM_EXPORT(mbedtls_md_hmac_finish),
    ELFSYM_EXPORT(mbedtls_md_hmac_starts),
    ELFSYM_EXPORT(mbedtls_md_hmac_update),
    ELFSYM_EXPORT(mbedtls_md_info_from_type),
    ELFSYM_EXPORT(mbedtls_md_init),
    ELFSYM_EXPORT(mbedtls_md_setup),
    ELFSYM_EXPORT(mbedtls_md_starts),
    ELFSYM_EXPORT(mbedtls_md_update),
    ELFSYM_EXPORT(mbedtls_mpi_bitlen),
    ELFSYM_EXPORT(mbedtls_mpi_cmp_mpi),
    ELFSYM_EXPORT(mbedtls_mpi_copy),
    ELFSYM_EXPORT(mbedtls_mpi_exp_mod),
    ELFSYM_EXPORT(mbedtls_mpi_fill_random),
    ELFSYM_EXPORT(mbedtls_mpi_get_bit),
    ELFSYM_EXPORT(mbedtls_mpi_lset),
    ELFSYM_EXPORT(mbedtls_mpi_mod_mpi),
    ELFSYM_EXPORT(mbedtls_mpi_read_binary),
    ELFSYM_EXPORT(mbedtls_mpi_read_string),
    ELFSYM_EXPORT(mbedtls_mpi_set_bit),
    ELFSYM_EXPORT(mbedtls_mpi_size),
    ELFSYM_EXPORT(mbedtls_mpi_sub_mpi),
    ELFSYM_EXPORT(mbedtls_mpi_write_binary),
    ELFSYM_EXPORT(mbedtls_pk_can_do),
    ELFSYM_EXPORT(mbedtls_pk_get_bitlen),
    ELFSYM_EXPORT(mbedtls_pk_get_type),
    ELFSYM_EXPORT(mbedtls_pk_parse_key),
    ELFSYM_EXPORT(mbedtls_pk_sign),
    ELFSYM_EXPORT(mbedtls_pk_verify),
    ELFSYM_EXPORT(mbedtls_poly1305_finish),
    ELFSYM_EXPORT(mbedtls_poly1305_free),
    ELFSYM_EXPORT(mbedtls_poly1305_init),
    ELFSYM_EXPORT(mbedtls_poly1305_starts),
    ELFSYM_EXPORT(mbedtls_poly1305_update),
    ELFSYM_EXPORT(mbedtls_rsa_check_privkey),
    ELFSYM_EXPORT(mbedtls_rsa_complete),
    ELFSYM_EXPORT(mbedtls_rsa_export),
    ELFSYM_EXPORT(mbedtls_rsa_gen_key),
    ELFSYM_EXPORT(mbedtls_rsa_import),
    ELFSYM_EXPORT(mbedtls_rsa_import_raw),
    ELFSYM_EXPORT(mbedtls_strerror),

#endif

#if !defined(PICO_BUILD) && !defined(EMSCRIPTEN)
#if S0IL_NATIVE
    {"if_indextoname", &if_indextoname},
    {"if_nametoindex", &if_nametoindex},
#else
    {"if_indextoname", &netif_index_to_name},
    {"if_nametoindex", &netif_name_to_index},
    {"netif_index_to_name", &netif_index_to_name},
    {"netif_name_to_index", &netif_name_to_index},
#endif
#endif

    {"gethostname", &s0il_gethostname},
    {"getuid", &s0il_getuid},
    {"waitpid", &s0il_waitpid},
    {"wait", &s0il_wait},
    {"trunc", &trunc},
    {"acosh", &acosh},
    {"asinh", &asinh},
    {"atanh", &atanh},
    {"log1p", &log1p},
    {"log2", &log2},
    {"log10", &log10},
    {"cbrt", &cbrt},
#if 0
  {"dup", &s0il_dup},
  {"dup2", &s0il_dup2},
  {"pipe", &s0il_pipe},
#endif

    ELFSYM_END};

const void *s0il_sym(const char *name) {
  for (int i = 0; g_customer_elfsyms[i].name; i++)
    if (!strcmp(g_customer_elfsyms[i].name, name))
      return g_customer_elfsyms[i].sym;
  // printf ("symbol %s not found\n", name);
  return NULL;
}
