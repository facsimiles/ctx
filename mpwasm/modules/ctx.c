#include "ctx.h"
/* ctx needs an allocator - make it use the one from MicroPython */
//#include "ctx-alloc.h"

#include "epicardium.h"

#include "py/obj.h"
#include "py/runtime.h"

typedef struct _mp_ctx_event_obj_t mp_ctx_event_obj_t;
typedef struct _mp_ctx_obj_t {
	mp_obj_base_t base;
	Ctx *ctx;
	mp_ctx_event_obj_t *ctx_event;
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
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

void gc_collect(void);

#define MP_CTX_COMMON_FUN_0_idle(name)                                         \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in)                        \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx);                                         \
                mp_idle(0);                                                    \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1F(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, mp_obj_get_float(arg1));                 \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1I(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, mp_obj_get_int(arg1));                   \
		return self_in;                                                \
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
		return self_in;                                                \
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
		return args[0];                                                \
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
		return args[0];                                                \
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
		return args[0];                                                \
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
		return self;                                                   \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 7, 7, mp_ctx_##name);


#define MP_CTX_COMMON_FUN_9F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 10);                                          \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_get_float(args[1]),                             \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]),                             \
			mp_obj_get_float(args[4]),                             \
			mp_obj_get_float(args[5]),                             \
			mp_obj_get_float(args[6]),                             \
			mp_obj_get_float(args[7]),                             \
			mp_obj_get_float(args[8]),                             \
			mp_obj_get_float(args[9]));                            \
		return self;                                                   \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 10, 10, mp_ctx_##name);

MP_CTX_COMMON_FUN_0(begin_path);
MP_CTX_COMMON_FUN_0(save);
MP_CTX_COMMON_FUN_0(restore);

MP_CTX_COMMON_FUN_0(start_frame);
MP_CTX_COMMON_FUN_0_idle(end_frame);

MP_CTX_COMMON_FUN_0(start_group);
MP_CTX_COMMON_FUN_0(end_group);
MP_CTX_COMMON_FUN_0(clip);
MP_CTX_COMMON_FUN_0(identity);
MP_CTX_COMMON_FUN_1F(rotate);
MP_CTX_COMMON_FUN_2F(scale);
MP_CTX_COMMON_FUN_2F(translate);
MP_CTX_COMMON_FUN_9F(apply_transform);
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

//MP_CTX_COMMON_FUN_3F(key_down);
//MP_CTX_COMMON_FUN_3F(key_up);
//MP_CTX_COMMON_FUN_3F(key_press);
MP_CTX_COMMON_FUN_4F(scrolled);
MP_CTX_COMMON_FUN_4F(pointer_motion);
MP_CTX_COMMON_FUN_4F(pointer_release);
MP_CTX_COMMON_FUN_4F(pointer_press);
        // missing: incoming_message
        //          pointer_drop


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
        return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_line_dash_obj, mp_ctx_line_dash);


static mp_obj_t mp_ctx_in_fill(mp_obj_t self_in, mp_obj_t arg1, mp_obj_t arg2)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  return mp_obj_new_bool (ctx_in_fill (self->ctx,
                                         mp_obj_get_float(arg1),
                                         mp_obj_get_float(arg2)));
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_in_fill_obj, mp_ctx_in_fill);

#if 0
static mp_obj_t mp_ctx_in_stroke(mp_obj_t self_in, mp_obj_t arg1, mp_obj_t arg2)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  return mp_obj_new_bool (ctx_in_stroke (self->ctx,
                                           mp_obj_get_float(arg1),
                                           mp_obj_get_float(arg2)));
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_in_stroke_obj, mp_ctx_in_stroke);
#endif

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
	return args[0];
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
        return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_arc_obj, 7, 7, mp_ctx_arc);

static mp_obj_t mp_ctx_font(mp_obj_t self_in, mp_obj_t font_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const char *font   = mp_obj_str_get_str(font_in);
	ctx_font(self->ctx, font);
        return self_in;                                                \
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_font_obj, mp_ctx_font);

#define MP_CTX_TEXT_FUNB(name)                                                  \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_str_get_str(args[1]),                           \
			mp_obj_get_float(args[2]),                             \
			mp_obj_get_float(args[3]));                            \
                return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 4, 4, mp_ctx_##name);

MP_CTX_TEXT_FUNB(fill_text);
MP_CTX_TEXT_FUNB(stroke_text);


#define MP_CTX_TEXT_FUN(name)                                                  \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_str_get_str(args[1]));                          \
                return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 2, 2, mp_ctx_##name);

MP_CTX_TEXT_FUN(text);
MP_CTX_TEXT_FUN(parse);
MP_CTX_TEXT_FUN(text_stroke);

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

        return args[0];
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

        return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_add_stop_obj, 3, 4, mp_ctx_add_stop);


static mp_obj_t mp_ctx_update(mp_obj_t self_in, mp_obj_t display_in)
{
#ifdef EMSCRIPTEN
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);

        int res = 0;
        Ctx *ctx = ctx_wasm_get_context(CTX_CB_KEEP_DATA);
        ctx_start_frame (ctx);
        ctx_render_ctx (self->ctx, ctx);
        ctx_end_frame (ctx);
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
        gc_collect ();
        return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_update_obj, mp_ctx_update);
/* CTX API functions }}} */

#ifdef EMSCRIPTEN
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


STATIC void generic_method_lookup(mp_obj_t obj, qstr attr, mp_obj_t *dest) {
    const mp_obj_type_t *type = mp_obj_get_type(obj);
    if (type->locals_dict != NULL) {
         // generic method lookup
         // this is a lookup in the object (ie not class or type)
         assert(type->locals_dict->base.type == &mp_type_dict); // MicroPython restriction, for now
         mp_map_t *locals_map = &type->locals_dict->map;
         mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
         if (elem != NULL) {
             mp_convert_member_lookup(obj, type, elem->value, dest);
         }
    }
}

extern const mp_obj_type_t mp_ctx_event_type;
struct _mp_ctx_event_obj_t {
	mp_obj_base_t base;
	CtxEvent  event;
	mp_obj_t  mp_ev;
};


static mp_obj_t mp_ctx_event_new (void)
{
	mp_ctx_event_obj_t *o = m_new_obj(mp_ctx_event_obj_t);
	o->base.type    = &mp_ctx_event_type;
	return MP_OBJ_FROM_PTR(o);
}

/** CtxEvent **/

STATIC mp_obj_t
mp_ctx_event_attr_op (mp_obj_t self_in, qstr attr, mp_obj_t set_val)
{
  mp_ctx_event_obj_t *self = MP_OBJ_TO_PTR(self_in);       
  if (set_val == MP_OBJ_NULL) {
    switch (attr)
    {
      case MP_QSTR_x:         return mp_obj_new_float(self->event.x);
      case MP_QSTR_y:         return mp_obj_new_float(self->event.y);
      case MP_QSTR_device_x:  return mp_obj_new_float(self->event.device_x);
      case MP_QSTR_device_y:  return mp_obj_new_float(self->event.device_y);
      case MP_QSTR_start_x:   return mp_obj_new_float(self->event.start_x);
      case MP_QSTR_start_y:   return mp_obj_new_float(self->event.start_y);
      case MP_QSTR_prev_x:    return mp_obj_new_float(self->event.prev_x);
      case MP_QSTR_prev_y:    return mp_obj_new_float(self->event.prev_y);
      case MP_QSTR_delta_x:   return mp_obj_new_float(self->event.delta_x);
      case MP_QSTR_delta_y:   return mp_obj_new_float(self->event.delta_y);
      case MP_QSTR_device_no: return mp_obj_new_int(self->event.device_no);
      case MP_QSTR_unicode:   return mp_obj_new_int(self->event.unicode);
      case MP_QSTR_scroll_direction:  return mp_obj_new_int(self->event.scroll_direction);
      case MP_QSTR_time:      return mp_obj_new_int(self->event.time);
      case MP_QSTR_modifier_state:   return mp_obj_new_int(self->event.state);
      case MP_QSTR_string:    if (self->event.string)
                                 // gambling on validity
                                 return mp_obj_new_str(self->event.string, strlen(self->event.string));
                              else
                                 return mp_obj_new_str("", 0);
    }
  }
  else
  {
     return set_val;
  }
  return self_in;
}

STATIC void mp_ctx_event_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_x
     ||attr == MP_QSTR_y

     ||attr == MP_QSTR_start_x
     ||attr == MP_QSTR_start_y
     ||attr == MP_QSTR_prev_x
     ||attr == MP_QSTR_prev_y
     ||attr == MP_QSTR_delta_x
     ||attr == MP_QSTR_delta_y
     ||attr == MP_QSTR_device_no
     ||attr == MP_QSTR_unicode
     ||attr == MP_QSTR_scroll_direction
     ||attr == MP_QSTR_time
     ||attr == MP_QSTR_modifier_state
     ||attr == MP_QSTR_string
     ||attr == MP_QSTR_device_x
     ||attr == MP_QSTR_device_y)
    {
        if (dest[0] == MP_OBJ_NULL) {
            // load attribute
            mp_obj_t val = mp_ctx_event_attr_op(obj, attr, MP_OBJ_NULL);
            dest[0] = val;
        } else {
            // delete/store attribute
            if (mp_ctx_event_attr_op(obj, attr, dest[1]) != MP_OBJ_NULL)
                dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
    else {
        // A method call
        generic_method_lookup(obj, attr, dest);
    }
}


static const mp_rom_map_elem_t mp_ctx_event_locals_dict_table[] = {
       // stop_propagate()
        // Instance attributes
       { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_device_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_device_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_start_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_start_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_prev_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_prev_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_delta_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_delta_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_device_no), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_unicode), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_scroll_direction), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_modifier_state), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_string), MP_ROM_INT(0) },
};
static MP_DEFINE_CONST_DICT(mp_ctx_event_locals_dict, mp_ctx_event_locals_dict_table);

static mp_obj_t mp_ctx_event_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *args
) {
	mp_ctx_event_obj_t *o = m_new_obj(mp_ctx_event_obj_t);
	o->base.type    = type;
	return MP_OBJ_FROM_PTR(o);
}


const mp_obj_type_t mp_ctx_event_type = {
	.base        = { &mp_type_type },
	.name        = MP_QSTR_CtxEvent,
	.make_new    = mp_ctx_event_make_new,
	.locals_dict = (mp_obj_t)&mp_ctx_event_locals_dict,
        .attr = mp_ctx_event_attr
};

static void mp_ctx_listen_cb_handler (CtxEvent *event, void *data1, void*data2)
{
  mp_obj_t event_in = data2;
  mp_ctx_event_obj_t *mp_ctx_event = MP_OBJ_TO_PTR(event_in);
  mp_ctx_event->event = *event;
  mp_sched_schedule (data1, event_in);//MP_OBJ_FROM_PTR(event_in));
  //fprintf (stderr, "schedlued\n");
  //if (mp_obj_is_true (mp_call_function_1((data1), event_in)))
  //  event->stop_propagate = 1;
}

static void mp_ctx_listen_cb_handler_stop_propagate (CtxEvent *event, void *data1, void*data2)
{
  mp_obj_t event_in = data2;
  mp_ctx_event_obj_t *mp_ctx_event = MP_OBJ_TO_PTR(event_in);
  mp_ctx_event->event = *event;
  mp_sched_schedule (data1, event_in);//MP_OBJ_FROM_PTR(event_in));
  event->stop_propagate = 1;
}

static mp_obj_t mp_ctx_listen (mp_obj_t self_in, mp_obj_t event_mask, mp_obj_t cb_in)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  if (cb_in != mp_const_none &&
      !mp_obj_is_callable(cb_in))
          mp_raise_ValueError(MP_ERROR_TEXT("invalid handler"));
  ctx_listen (self->ctx,
              mp_obj_get_int(event_mask),
              mp_ctx_listen_cb_handler,
              (cb_in), self->ctx_event);
  return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_listen_obj, mp_ctx_listen);

static mp_obj_t mp_ctx_listen_stop_propagate (mp_obj_t self_in, mp_obj_t event_mask, mp_obj_t cb_in)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  if (cb_in != mp_const_none &&
      !mp_obj_is_callable(cb_in))
          mp_raise_ValueError(MP_ERROR_TEXT("invalid handler"));
  ctx_listen (self->ctx,
              mp_obj_get_int(event_mask),
              mp_ctx_listen_cb_handler_stop_propagate,
              (cb_in), self->ctx_event);
  return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_listen_stop_propagate_obj, mp_ctx_listen_stop_propagate);

static mp_obj_t mp_ctx_tinyvg_get_size (mp_obj_t self_in, mp_obj_t path_in)
{
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
	return self_in;
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
	return self_in;
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
        o->ctx_event = mp_ctx_event_new ();

	return MP_OBJ_FROM_PTR(o);
}

extern const mp_obj_type_t mp_ctx_type;

static mp_obj_t mp_ctx_new_drawlist  (mp_obj_t width_in, mp_obj_t height_in)
{
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = &mp_ctx_type;
	o->ctx          = ctx_new_drawlist(mp_obj_get_float(width_in),
                                           mp_obj_get_float(height_in));

        o->ctx_event = mp_ctx_event_new ();
	return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_new_drawlist_obj, mp_ctx_new_drawlist);


static mp_obj_t mp_ctx_get_context (mp_obj_t name)
{
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = &mp_ctx_type;
	o->ctx          = ctx_wasm_get_context(CTX_CB_KEEP_DATA);
        o->ctx_event = mp_ctx_event_new ();
        //ctx_start_frame (o->ctx);
	return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_get_context_obj, mp_ctx_get_context);


STATIC mp_obj_t
mp_ctx_attr_op (mp_obj_t self_in, qstr attr, mp_obj_t set_val)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);       
  if (set_val == MP_OBJ_NULL) {
    switch (attr)
    {
       case MP_QSTR_font_size:
            return mp_obj_new_float(ctx_get_font_size (self->ctx));
       case MP_QSTR_line_width:
            return mp_obj_new_float(ctx_get_line_width (self->ctx));
       case MP_QSTR_line_dash_offset:
            return mp_obj_new_float(ctx_get_line_dash_offset (self->ctx));
       case MP_QSTR_miter_limit:
            return mp_obj_new_float(ctx_get_miter_limit (self->ctx));
       case MP_QSTR_global_alpha:
            return mp_obj_new_float(ctx_get_global_alpha (self->ctx));
       case MP_QSTR_width:
            return mp_obj_new_int(ctx_width (self->ctx));
       case MP_QSTR_height:
            return mp_obj_new_int(ctx_height (self->ctx));
       case MP_QSTR_x:
            return mp_obj_new_int(ctx_x (self->ctx));
       case MP_QSTR_y:
            return mp_obj_new_int(ctx_y (self->ctx));
    }
  }
  else
  {
    switch (attr)
    {
       case MP_QSTR_line_width:
         ctx_line_width (self->ctx, mp_obj_get_float (set_val)); break;
       case MP_QSTR_line_dash_offset:
         ctx_line_dash_offset (self->ctx, mp_obj_get_float (set_val)); break;
       case MP_QSTR_miter_limit:
         ctx_miter_limit (self->ctx, mp_obj_get_float (set_val)); break;
       case MP_QSTR_global_alpha:
         ctx_global_alpha (self->ctx, mp_obj_get_float (set_val)); break;
       case MP_QSTR_font_size:
         ctx_font_size (self->ctx, mp_obj_get_float (set_val)); break;
    }
    return set_val;
  }
  return self_in;
}


STATIC void mp_ctx_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_width
     ||attr == MP_QSTR_height
     ||attr == MP_QSTR_line_width
     ||attr == MP_QSTR_line_dash_offset
     ||attr == MP_QSTR_miter_limit
     ||attr == MP_QSTR_global_alpha
     ||attr == MP_QSTR_font_size
     ||attr == MP_QSTR_x
     ||attr == MP_QSTR_y)
    {
        if (dest[0] == MP_OBJ_NULL) {
            // load attribute
            mp_obj_t val = mp_ctx_attr_op(obj, attr, MP_OBJ_NULL);
            dest[0] = val;
        } else {
            // delete/store attribute
            if (mp_ctx_attr_op(obj, attr, dest[1]) != MP_OBJ_NULL)
                dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
    else {
        // A method call
        generic_method_lookup(obj, attr, dest);
    }
}

/* CTX class/type */

#define MP_CTX_INT_CONSTANT_UNPREFIXED(ident)                         \
        {                                                                      \
		MP_ROM_QSTR(MP_QSTR_##ident), MP_ROM_INT((int)CTX_##ident)     \
	}
#define MP_CTX_INT_CONSTANT(prefix, ident)                                     \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##ident), MP_ROM_INT((int)CTX_##prefix##_##ident)     \
	}
#define MP_CTX_METHOD(name)                                                    \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&mp_ctx_##name##_obj)  \
	}

static const mp_rom_map_elem_t mp_ctx_locals_dict_table[] = {



	MP_CTX_METHOD(begin_path),
	MP_CTX_METHOD(save),
	MP_CTX_METHOD(restore),
	MP_CTX_METHOD(start_group),
	MP_CTX_METHOD(end_group),
	MP_CTX_METHOD(clip),
	MP_CTX_METHOD(identity),
	MP_CTX_METHOD(rotate),
	MP_CTX_METHOD(font),
	MP_CTX_METHOD(scale),
	MP_CTX_METHOD(apply_transform),
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
	MP_CTX_METHOD(text),
	MP_CTX_METHOD(text_stroke),
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
	MP_CTX_METHOD(start_frame),
	MP_CTX_METHOD(end_frame),
	MP_CTX_METHOD(tinyvg_draw),
	MP_CTX_METHOD(tinyvg_get_size),
	MP_CTX_METHOD(listen),
	MP_CTX_METHOD(parse),
	MP_CTX_METHOD(listen_stop_propagate),

        MP_CTX_METHOD(in_fill),
        //MP_CTX_METHOD(in_stroke),

        //MP_CTX_METHOD(key_down),
        //MP_CTX_METHOD(key_up),
        //MP_CTX_METHOD(key_press),
        MP_CTX_METHOD(scrolled),
        MP_CTX_METHOD(pointer_motion),
        MP_CTX_METHOD(pointer_release),
        MP_CTX_METHOD(pointer_press),

        // Instance attributes
       { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_line_width), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_line_dash_offset), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_miter_limit), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_global_alpha), MP_ROM_INT(0) },
       { MP_ROM_QSTR(MP_QSTR_font_size), MP_ROM_INT(0) },
};
static MP_DEFINE_CONST_DICT(mp_ctx_locals_dict, mp_ctx_locals_dict_table);

const mp_obj_type_t mp_ctx_type = {
	.base        = { &mp_type_type },
	.name        = MP_QSTR_Ctx,
	.make_new    = mp_ctx_make_new,
	.locals_dict = (mp_obj_t)&mp_ctx_locals_dict,
        .attr = mp_ctx_attr
};

/* The globals table for this module */
static const mp_rom_map_elem_t mp_ctx_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ctx_graphics) },
	{ MP_ROM_QSTR(MP_QSTR_Ctx), MP_ROM_PTR(&mp_ctx_type) },
	{ MP_ROM_QSTR(MP_QSTR_CtxEvent), MP_ROM_PTR(&mp_ctx_event_type) },
//	{ MP_ROM_QSTR(MP_QSTR_new_for_buffer), MP_ROM_PTR(&mp_ctx_new_for_buffer_obj) },
	{ MP_ROM_QSTR(MP_QSTR_new_drawlist), MP_ROM_PTR(&mp_ctx_new_drawlist_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_context), MP_ROM_PTR(&mp_ctx_get_context_obj) },

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

	MP_CTX_INT_CONSTANT_UNPREFIXED(PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(MOTION),
	MP_CTX_INT_CONSTANT_UNPREFIXED(RELEASE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(ENTER),
	MP_CTX_INT_CONSTANT_UNPREFIXED(LEAVE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(TAP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(TAP_AND_HOLD),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_MOTION),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_RELEASE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_DOWN),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_UP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(SCROLL),
	MP_CTX_INT_CONSTANT_UNPREFIXED(MESSAGE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DROP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(SET_CURSOR),
};
static MP_DEFINE_CONST_DICT(mp_ctx_module_globals, mp_ctx_module_globals_table);

const mp_obj_module_t mp_ctx_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&mp_ctx_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_ctx_graphics, mp_ctx_module, MODULE_CTX_ENABLED);