#include "ctx.h"
/* ctx needs an allocator - make it use the one from MicroPython */
//#include "ctx-alloc.h"

#include "epicardium.h"

#include "py/obj.h"
#include "py/runtime.h"

/* CTX allocator wrappers {{{ */
void *ctx_alloc_malloc(size_t size)
{
	return m_malloc(size);
}

void *ctx_alloc_calloc(size_t nmemb, size_t size)
{
	size_t byte_size = nmemb * size;
	char *ret        = (char *)m_malloc(byte_size);
	for (size_t i = 0; i < byte_size; i++)
		ret[i] = 0;
	return ret;
}

void *ctx_alloc_realloc(void *ptr, size_t size)
{
	return m_realloc(ptr, size);
}

void ctx_alloc_free(void *ptr)
{
	return m_free(ptr);
}
/* CTX allocator wrappers }}} */

typedef struct _mp_ctx_obj_t {
	mp_obj_base_t base;
	Ctx *ctx;
} mp_ctx_obj_t;

#ifdef EMSCRIPTEN
extern int _mp_quit;
void mp_idle (int ms);
#endif

/* CTX API functions {{{ */
#define MP_CTX_COMMON_FUN_0(name)                                              \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in)                        \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx);                                         \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_0_idle(name)                                         \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in)                        \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx);                                         \
		mp_idle (0);                                                   \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1F(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, mp_obj_get_float(arg1));                 \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1I(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, mp_obj_get_int(arg1));                   \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_2F(name)                                             \
	static mp_obj_t mp_ctx_##name(                                         \
		mp_obj_t self_in, mp_obj_t arg1, mp_obj_t arg2)                \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(arg1),                                \
			mp_obj_get_float(arg2));                               \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_3F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(args[1]),                             \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]));                            \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 4, 4, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_4F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 5);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(args[1]),                             \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]),                             \
			mp_obj_get_float(args[4]));                            \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 5, 5, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_5F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 6);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(args[1]),                             \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]),                             \
			mp_obj_get_float(args[4]),                             \
			mp_obj_get_float(args[5]));                            \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 6, 6, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_6F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 7);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(args[1]),                             \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]),                             \
			mp_obj_get_float(args[4]),                             \
			mp_obj_get_float(args[5]),                             \
			mp_obj_get_float(args[6]));                            \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 7, 7, mp_ctx_##name);

MP_CTX_COMMON_FUN_0(begin_path);
MP_CTX_COMMON_FUN_0(save);
MP_CTX_COMMON_FUN_0(restore);

MP_CTX_COMMON_FUN_0_idle(flush);
MP_CTX_COMMON_FUN_0(reset);

MP_CTX_COMMON_FUN_0(start_group);
MP_CTX_COMMON_FUN_0(end_group);
MP_CTX_COMMON_FUN_0(clip);
MP_CTX_COMMON_FUN_0(identity);
MP_CTX_COMMON_FUN_1F(rotate);
MP_CTX_COMMON_FUN_1F(miter_limit);
MP_CTX_COMMON_FUN_1F(line_width);
MP_CTX_COMMON_FUN_1F(global_alpha);
MP_CTX_COMMON_FUN_1F(line_dash_offset);
MP_CTX_COMMON_FUN_1F(font_size);
MP_CTX_COMMON_FUN_2F(scale);
MP_CTX_COMMON_FUN_2F(translate);
MP_CTX_COMMON_FUN_2F(line_to);
MP_CTX_COMMON_FUN_2F(move_to);
MP_CTX_COMMON_FUN_6F(curve_to);
MP_CTX_COMMON_FUN_4F(quad_to);
MP_CTX_COMMON_FUN_5F(arc_to);
MP_CTX_COMMON_FUN_5F(rel_arc_to);
MP_CTX_COMMON_FUN_4F(rectangle);
MP_CTX_COMMON_FUN_5F(round_rectangle);
MP_CTX_COMMON_FUN_2F(rel_line_to);
MP_CTX_COMMON_FUN_2F(rel_move_to);
MP_CTX_COMMON_FUN_6F(rel_curve_to);
MP_CTX_COMMON_FUN_4F(rel_quad_to);
MP_CTX_COMMON_FUN_0(close_path);

MP_CTX_COMMON_FUN_4F(linear_gradient);
MP_CTX_COMMON_FUN_6F(radial_gradient);

MP_CTX_COMMON_FUN_0(preserve);
MP_CTX_COMMON_FUN_0(fill);
MP_CTX_COMMON_FUN_0(stroke);
MP_CTX_COMMON_FUN_0(paint);
MP_CTX_COMMON_FUN_3F(logo);

MP_CTX_COMMON_FUN_1I(blend_mode);
MP_CTX_COMMON_FUN_1I(text_align);
MP_CTX_COMMON_FUN_1I(text_baseline);
MP_CTX_COMMON_FUN_1I(fill_rule);
MP_CTX_COMMON_FUN_1I(line_cap);
MP_CTX_COMMON_FUN_1I(line_join);
MP_CTX_COMMON_FUN_1I(compositing_mode);
MP_CTX_COMMON_FUN_1I(image_smoothing);

static mp_obj_t mp_ctx_line_dash(mp_obj_t self_in, mp_obj_t dashes_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);

	size_t count  = mp_obj_get_int(mp_obj_len(dashes_in));
	float *dashes = m_malloc(sizeof(float) * count);
	for (size_t i = 0; i < count; i++) {
		dashes[i] = mp_obj_get_float(mp_obj_subscr(
			dashes_in, mp_obj_new_int(i), MP_OBJ_SENTINEL)
		);
	}

	ctx_line_dash(self->ctx, dashes, count);

	m_free(dashes);
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_line_dash_obj, mp_ctx_line_dash);


static mp_obj_t mp_ctx_texture (size_t n_args, const mp_obj_t *args)
{
        mp_buffer_info_t buffer_info;
	assert(n_args == 7);
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);

        if (!mp_get_buffer(args[1], &buffer_info, MP_BUFFER_READ))
        {
           mp_raise_TypeError("not a buffer");
        }
        int format = mp_obj_get_int (args[2]);
        int width  = mp_obj_get_int (args[3]);
        int height = mp_obj_get_int (args[4]);
        int stride = mp_obj_get_int (args[5]);
        static int eid_no = 0;
        char ieid[10]; // this is a workaround for the rasterizer
                       // not keeping the cache validity expected
                       // as we drive it
                       // it also means we cannot properly use
                       // eid based APIs
        sprintf (ieid, "%i", eid_no++);
	ctx_define_texture (self->ctx,
                        ieid, width, height, stride, format,
                        buffer_info.buf, NULL);
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_texture_obj, 6, 6, mp_ctx_texture);

static mp_obj_t mp_ctx_arc(size_t n_args, const mp_obj_t *args)
{
	assert(n_args == 7);
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	ctx_arc(self->ctx,
		mp_obj_get_float(args[1]),
		mp_obj_get_float(args[2]),
		mp_obj_get_float(args[3]),
		mp_obj_get_float(args[4]),
		mp_obj_get_float(args[5]),
		mp_obj_get_int(args[6]));
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_arc_obj, 7, 7, mp_ctx_arc);

static mp_obj_t mp_ctx_font(mp_obj_t self_in, mp_obj_t font_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const char *font   = mp_obj_str_get_str(font_in);
	ctx_font(self->ctx, font);
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_font_obj, mp_ctx_font);

#define MP_CTX_TEXT_FUN(name)                                                  \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_str_get_str(args[1]),                           \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]));                            \
		return MP_OBJ_FROM_PTR(self);                                  \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 4, 4, mp_ctx_##name);

MP_CTX_TEXT_FUN(fill_text);
MP_CTX_TEXT_FUN(stroke_text);

static mp_obj_t mp_ctx_text_width(mp_obj_t self_in, mp_obj_t string_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const char *string = mp_obj_str_get_str(string_in);
	return mp_obj_new_float(ctx_text_width(self->ctx, string));
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_text_width_obj, mp_ctx_text_width);

static mp_obj_t
mp_ctx_color_common(size_t n_args, const mp_obj_t *args, bool stroke)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	mp_obj_t color_in  = args[1];

	float alpha_f = 1.0;
	if (n_args == 3) {
		alpha_f = mp_obj_get_float(args[2]);
	}
	if (alpha_f < 0.0 || alpha_f > 1.0) {
		mp_raise_ValueError("alpha must be between 0.0 or 1.0");
	}

	mp_obj_t red_in, green_in, blue_in;
	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}
	red_in   = mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL);
	green_in = mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL);
	blue_in  = mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL);

	/*
	 * The color can be either floats between 0 and 1 or integers between 0
	 * and 255.  Make this decision based on the first element we find.
	 */
	if (mp_obj_is_type(red_in, &mp_type_float)) {
		float red, green, blue;
		red   = mp_obj_get_float(red_in);
		green = mp_obj_get_float(green_in);
		blue  = mp_obj_get_float(blue_in);

		if (stroke) {
			ctx_rgba_stroke(self->ctx, red, green, blue, alpha_f);
		} else {
			ctx_rgba(self->ctx, red, green, blue, alpha_f);
		}
	} else {
		uint8_t red, green, blue, alpha;
		red   = mp_obj_get_int(red_in);
		green = mp_obj_get_int(green_in);
		blue  = mp_obj_get_int(blue_in);

		alpha = (int)(alpha_f * 255.0);
		if (stroke) {
			ctx_rgba8_stroke(self->ctx, red, green, blue, alpha);
		} else {
			ctx_rgba8(self->ctx, red, green, blue, alpha);
		}
	}

	return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_ctx_color(size_t n_args, const mp_obj_t *args)
{
	return mp_ctx_color_common(n_args, args, false);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_color_obj, 2, 3, mp_ctx_color);

static mp_obj_t mp_ctx_stroke_color(size_t n_args, const mp_obj_t *args)
{
	return mp_ctx_color_common(n_args, args, true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
	mp_ctx_stroke_color_obj, 2, 3, mp_ctx_stroke_color
);

static mp_obj_t mp_ctx_add_stop(size_t n_args, const mp_obj_t *args)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	mp_obj_t color_in  = args[2];

	float pos = mp_obj_get_float(args[1]);

	float alpha_f = 1.0;
	if (n_args == 4) {
		alpha_f = mp_obj_get_float(args[3]);
	}
	if (alpha_f < 0.0 || alpha_f > 1.0) {
		mp_raise_ValueError("alpha must be between 0.0 or 1.0");
	}

	mp_obj_t red_in, green_in, blue_in;
	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}
	red_in   = mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL);
	green_in = mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL);
	blue_in  = mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL);

	/*
	 * The color can be either floats between 0 and 1 or integers between 0
	 * and 255.  Make this decision based on the first element we find.
	 */
	if (mp_obj_is_type(red_in, &mp_type_float)) {
		float red, green, blue;
		red   = mp_obj_get_float(red_in);
		green = mp_obj_get_float(green_in);
		blue  = mp_obj_get_float(blue_in);

		ctx_gradient_add_stop(
			self->ctx, pos, red, green, blue, alpha_f
		);
	} else {
		uint8_t red, green, blue, alpha;
		red   = mp_obj_get_int(red_in);
		green = mp_obj_get_int(green_in);
		blue  = mp_obj_get_int(blue_in);

		alpha = (int)(alpha_f * 255.0);
		ctx_gradient_add_stop_u8(
			self->ctx, pos, red, green, blue, alpha
		);
	}

	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_add_stop_obj, 3, 4, mp_ctx_add_stop);


static mp_obj_t mp_ctx_update(mp_obj_t self_in, mp_obj_t display_in)
{
#ifdef EMSCRIPTEN
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);

        int res = 0;
        Ctx *ctx = ctx_wasm_get_context(CTX_CB_KEEP_DATA);
        ctx_reset (ctx);
        ctx_render_ctx (self->ctx, ctx);
        ctx_flush (ctx);
	/*
	 * The drawlist still contains the draw commands which were just
	 * executed.  Flush them now.
	 */
	ctx_drawlist_clear(self->ctx);

	/* report errors from epic_disp_ctx() */
	if (res < 0) {
		mp_raise_OSError(-res);
	}
#else
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);

	/* TODO: Don't ignore the passed in display */
	if (display_in == mp_const_none) {
		mp_raise_ValueError("must pass in the display object");
	}

	int res = epic_disp_ctx(self->ctx);
#endif
	/*
	 * The drawlist still contains the draw commands which were just
	 * executed.  Flush them now.
	 */
	ctx_drawlist_clear(self->ctx);

	/* report errors from epic_disp_ctx() */
	if (res < 0) {
		mp_raise_OSError(-res);
	}
        mp_idle (0);
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_update_obj, mp_ctx_update);
/* CTX API functions }}} */

#ifdef EMSCRIPTEN
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static mp_obj_t mp_ctx_tinyvg_get_size (mp_obj_t self_in, mp_obj_t path_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
#ifdef EMSCRIPTEN
	const char *path = mp_obj_str_get_str(path_in);
        int width = 0; int height = 0;
        int fd = open (path, O_RDONLY);
        ctx_tinyvg_fd_get_size (fd, &width, &height);
        close (fd);

        mp_obj_t mp_w   = MP_OBJ_NEW_SMALL_INT(width);
        mp_obj_t mp_h   = MP_OBJ_NEW_SMALL_INT(height);
        mp_obj_t tup[] = { mp_w, mp_h };
        return mp_obj_new_tuple(2, tup);
#endif
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_tinyvg_get_size_obj, mp_ctx_tinyvg_get_size);
/* CTX API functions }}} */

static mp_obj_t mp_ctx_tinyvg_draw (mp_obj_t self_in, mp_obj_t path_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
#ifdef EMSCRIPTEN
	const char *path = mp_obj_str_get_str(path_in);
        int fd = open (path, O_RDONLY);
        ctx_tinyvg_fd_draw (self->ctx, fd, CTX_TVG_FLAG_DEFAULTS);
        close (fd);
#endif
	return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_tinyvg_draw_obj, mp_ctx_tinyvg_draw);
/* CTX API functions }}} */

static mp_obj_t mp_ctx_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *args
) {
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = type;
	o->ctx          = ctx_new_drawlist(160,80);

	return MP_OBJ_FROM_PTR(o);
}

extern const mp_obj_type_t mp_ctx_type;

static mp_obj_t mp_ctx_new_drawlist  (mp_obj_t width_in, mp_obj_t height_in)
{
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = &mp_ctx_type;
	o->ctx          = ctx_new_drawlist(mp_obj_get_float(width_in),
                                           mp_obj_get_float(height_in));

	return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_new_drawlist_obj, mp_ctx_new_drawlist);


static mp_obj_t mp_ctx_get_context (mp_obj_t name)
{
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = &mp_ctx_type;
	o->ctx          = ctx_wasm_get_context(CTX_CB_KEEP_DATA);
	return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_get_context_obj, mp_ctx_get_context);




/* CTX class/type */
#define MP_CTX_INT_CONSTANT(prefix, ident)                                     \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##ident), MP_ROM_INT((int)CTX_##prefix##_##ident)     \
	}
#define MP_CTX_METHOD(name)                                                    \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&mp_ctx_##name##_obj)  \
	}

static const mp_rom_map_elem_t mp_ctx_locals_dict_table[] = {
	MP_CTX_INT_CONSTANT(FILL_RULE,WINDING),
	MP_CTX_INT_CONSTANT(FILL_RULE,EVEN_ODD),
	MP_CTX_INT_CONSTANT(JOIN,BEVEL),
	MP_CTX_INT_CONSTANT(JOIN,ROUND),
	MP_CTX_INT_CONSTANT(JOIN,MITER),
	MP_CTX_INT_CONSTANT(CAP,NONE),
	MP_CTX_INT_CONSTANT(CAP,ROUND),
	MP_CTX_INT_CONSTANT(CAP,SQUARE),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_OVER),
	MP_CTX_INT_CONSTANT(COMPOSITE,COPY),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_IN),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_OUT),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_ATOP),
	MP_CTX_INT_CONSTANT(COMPOSITE,CLEAR),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_OVER),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_IN),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_OUT),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_ATOP),
	MP_CTX_INT_CONSTANT(COMPOSITE,XOR),
	MP_CTX_INT_CONSTANT(BLEND,NORMAL),
	MP_CTX_INT_CONSTANT(BLEND,MULTIPLY),
	MP_CTX_INT_CONSTANT(BLEND,SCREEN),
	MP_CTX_INT_CONSTANT(BLEND,OVERLAY),
	MP_CTX_INT_CONSTANT(BLEND,DARKEN),
	MP_CTX_INT_CONSTANT(BLEND,LIGHTEN),
	MP_CTX_INT_CONSTANT(BLEND,COLOR_DODGE),
	MP_CTX_INT_CONSTANT(BLEND,COLOR_BURN),
	MP_CTX_INT_CONSTANT(BLEND,HARD_LIGHT),
	MP_CTX_INT_CONSTANT(BLEND,SOFT_LIGHT),
	MP_CTX_INT_CONSTANT(BLEND,DIFFERENCE),
	MP_CTX_INT_CONSTANT(BLEND,EXCLUSION),
	MP_CTX_INT_CONSTANT(BLEND,HUE),
	MP_CTX_INT_CONSTANT(BLEND,SATURATION),
	MP_CTX_INT_CONSTANT(BLEND,COLOR),
	MP_CTX_INT_CONSTANT(BLEND,LUMINOSITY),
	MP_CTX_INT_CONSTANT(BLEND,DIVIDE),
	MP_CTX_INT_CONSTANT(BLEND,ADDITION),
	MP_CTX_INT_CONSTANT(BLEND,SUBTRACT),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,ALPHABETIC),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,TOP),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,HANGING),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,MIDDLE),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,IDEOGRAPHIC),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,BOTTOM),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,START),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,END),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,CENTER),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,LEFT),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,RIGHT),

	MP_CTX_INT_CONSTANT(FORMAT,GRAY8),
	MP_CTX_INT_CONSTANT(FORMAT,GRAYA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB8),
	MP_CTX_INT_CONSTANT(FORMAT,RGBA8),
	MP_CTX_INT_CONSTANT(FORMAT,BGRA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565_BYTESWAPPED),
	MP_CTX_INT_CONSTANT(FORMAT,RGB332),
	//MP_CTX_INT_CONSTANT(FORMAT,RGBAF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYAF),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY1),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY2),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY4),
	MP_CTX_INT_CONSTANT(FORMAT,YUV420),

	MP_CTX_METHOD(begin_path),
	MP_CTX_METHOD(save),
	MP_CTX_METHOD(restore),
	MP_CTX_METHOD(start_group),
	MP_CTX_METHOD(end_group),
	MP_CTX_METHOD(clip),
	MP_CTX_METHOD(identity),
	MP_CTX_METHOD(rotate),
	MP_CTX_METHOD(miter_limit),
	MP_CTX_METHOD(line_width),
	MP_CTX_METHOD(line_dash_offset),
	MP_CTX_METHOD(global_alpha),
	MP_CTX_METHOD(font),
	MP_CTX_METHOD(font_size),
	MP_CTX_METHOD(scale),
	MP_CTX_METHOD(translate),
	MP_CTX_METHOD(line_to),
	MP_CTX_METHOD(move_to),
	MP_CTX_METHOD(curve_to),
	MP_CTX_METHOD(quad_to),
	MP_CTX_METHOD(arc),
	MP_CTX_METHOD(arc_to),
	MP_CTX_METHOD(rel_arc_to),
	MP_CTX_METHOD(rectangle),
	MP_CTX_METHOD(round_rectangle),
	MP_CTX_METHOD(rel_line_to),
	MP_CTX_METHOD(rel_move_to),
	MP_CTX_METHOD(rel_curve_to),
	MP_CTX_METHOD(rel_quad_to),
	MP_CTX_METHOD(close_path),
	MP_CTX_METHOD(preserve),
	MP_CTX_METHOD(fill),
	MP_CTX_METHOD(stroke),
	MP_CTX_METHOD(paint),
	MP_CTX_METHOD(logo),
	MP_CTX_METHOD(blend_mode),
	MP_CTX_METHOD(text_align),
	MP_CTX_METHOD(text_baseline),
	MP_CTX_METHOD(fill_rule),
	MP_CTX_METHOD(line_cap),
	MP_CTX_METHOD(line_join),
	MP_CTX_METHOD(compositing_mode),
	MP_CTX_METHOD(fill_text),
	MP_CTX_METHOD(stroke_text),
	MP_CTX_METHOD(text_width),
	MP_CTX_METHOD(linear_gradient),
	MP_CTX_METHOD(radial_gradient),
	MP_CTX_METHOD(line_dash),
	MP_CTX_METHOD(add_stop),
	MP_CTX_METHOD(texture),
	MP_CTX_METHOD(image_smoothing),
	MP_CTX_METHOD(color),
	MP_CTX_METHOD(stroke_color),
	MP_CTX_METHOD(update),
	MP_CTX_METHOD(flush),
	MP_CTX_METHOD(reset),
	MP_CTX_METHOD(tinyvg_draw),
	MP_CTX_METHOD(tinyvg_get_size),
};
static MP_DEFINE_CONST_DICT(mp_ctx_locals_dict, mp_ctx_locals_dict_table);

const mp_obj_type_t mp_ctx_type = {
	.base        = { &mp_type_type },
	.name        = MP_QSTR_Ctx,
	.make_new    = mp_ctx_make_new,
	.locals_dict = (mp_obj_t)&mp_ctx_locals_dict,
};

/* The globals table for this module */
static const mp_rom_map_elem_t mp_ctx_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ctx_graphics) },
	{ MP_ROM_QSTR(MP_QSTR_Ctx), MP_ROM_PTR(&mp_ctx_type) },
//	{ MP_ROM_QSTR(MP_QSTR_new_for_buffer), MP_ROM_PTR(&mp_ctx_new_for_buffer_obj) },
	{ MP_ROM_QSTR(MP_QSTR_new_drawlist), MP_ROM_PTR(&mp_ctx_new_drawlist_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_context), MP_ROM_PTR(&mp_ctx_get_context_obj) }
};
static MP_DEFINE_CONST_DICT(mp_ctx_module_globals, mp_ctx_module_globals_table);

const mp_obj_module_t mp_ctx_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&mp_ctx_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_ctx_graphics, mp_ctx_module, MODULE_CTX_ENABLED);
