#include "epicardium.h"

#include "py/obj.h"
#include "py/objlist.h"
#include "py/runtime.h"

#include <stdio.h>

static mp_obj_t mp_leds_set(mp_obj_t led_in, mp_obj_t color_in)
{
	int led = mp_obj_get_int(led_in);

	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}

	uint8_t red = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL)
	);
	uint8_t green = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL)
	);
	uint8_t blue = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL)
	);

	epic_leds_set(led, red, green, blue);

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_set_obj, mp_leds_set);

static mp_obj_t mp_leds_set_hsv(mp_obj_t led_in, mp_obj_t color_in)
{
	int led = mp_obj_get_int(led_in);

	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}

	float h = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL)
	);
	float s = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL)
	);
	float v = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL)
	);

	epic_leds_set_hsv(led, h, s, v);

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_set_hsv_obj, mp_leds_set_hsv);

static mp_obj_t mp_leds_prep(mp_obj_t led_in, mp_obj_t color_in)
{
	int led = mp_obj_get_int(led_in);

	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}

	uint8_t red = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL)
	);
	uint8_t green = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL)
	);
	uint8_t blue = mp_obj_get_int(
		mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL)
	);

	epic_leds_prep(led, red, green, blue);

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_prep_obj, mp_leds_prep);

static mp_obj_t mp_leds_get_rgb(mp_obj_t led_in)
{
	int led       = mp_obj_get_int(led_in);
	uint8_t rgb[] = { 0, 0, 0 };
	int retAPI    = epic_leds_get_rgb(led, rgb);
	if (retAPI == -EPERM) {
		mp_raise_ValueError(
			"no permission: maybe blocked by personal state"
		);
	}
	if (retAPI == -EINVAL) {
		mp_raise_ValueError(
			"invalid value: maybe the led does not exists"
		);
	}

	mp_obj_t ret          = mp_obj_new_tuple(3, NULL);
	mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(ret);

	tuple->items[0] = MP_OBJ_NEW_SMALL_INT(rgb[0]);
	tuple->items[1] = MP_OBJ_NEW_SMALL_INT(rgb[1]);
	tuple->items[2] = MP_OBJ_NEW_SMALL_INT(rgb[2]);

	return ret;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_get_rgb_obj, mp_leds_get_rgb);

static mp_obj_t mp_leds_prep_hsv(mp_obj_t led_in, mp_obj_t color_in)
{
	int led = mp_obj_get_int(led_in);

	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}

	float h = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL)
	);
	float s = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL)
	);
	float v = mp_obj_get_float(
		mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL)
	);

	epic_leds_prep_hsv(led, h, s, v);

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_prep_hsv_obj, mp_leds_prep_hsv);

static mp_obj_t mp_leds_set_all(mp_obj_t color_in)
{
	uint8_t len = mp_obj_get_int(mp_obj_len(color_in));
	uint8_t pattern[len][3];
	for (int i = 0; i < len; i++) {
		mp_obj_t color = mp_obj_subscr(
			color_in, mp_obj_new_int(i), MP_OBJ_SENTINEL
		);
		pattern[i][0] = mp_obj_get_int(mp_obj_subscr(
			color, mp_obj_new_int(0), MP_OBJ_SENTINEL)
		);
		pattern[i][1] = mp_obj_get_int(mp_obj_subscr(
			color, mp_obj_new_int(1), MP_OBJ_SENTINEL)
		);
		pattern[i][2] = mp_obj_get_int(mp_obj_subscr(
			color, mp_obj_new_int(2), MP_OBJ_SENTINEL)
		);
	}
	epic_leds_set_all((uint8_t *)pattern, len);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_set_all_obj, mp_leds_set_all);

static mp_obj_t mp_leds_set_all_hsv(mp_obj_t color_in)
{
	uint8_t len = mp_obj_get_int(mp_obj_len(color_in));
	float pattern[len][3];
	for (int i = 0; i < len; i++) {
		mp_obj_t color = mp_obj_subscr(
			color_in, mp_obj_new_int(i), MP_OBJ_SENTINEL
		);
		pattern[i][0] = mp_obj_get_float(mp_obj_subscr(
			color, mp_obj_new_int(0), MP_OBJ_SENTINEL)
		);
		pattern[i][1] = mp_obj_get_float(mp_obj_subscr(
			color, mp_obj_new_int(1), MP_OBJ_SENTINEL)
		);
		pattern[i][2] = mp_obj_get_float(mp_obj_subscr(
			color, mp_obj_new_int(2), MP_OBJ_SENTINEL)
		);
	}
	epic_leds_set_all_hsv((float *)pattern, len);

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_set_all_hsv_obj, mp_leds_set_all_hsv);

static mp_obj_t mp_leds_set_flashlight(mp_obj_t on_in)
{
	int on = mp_obj_get_int(on_in);
	epic_set_flashlight(on);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
	leds_set_flashlight_obj, mp_leds_set_flashlight
);

static mp_obj_t mp_leds_set_rocket(mp_obj_t led_in, mp_obj_t value_in)
{
	int led   = mp_obj_get_int(led_in);
	int value = mp_obj_get_int(value_in);

	if (value > 31) {
		mp_raise_ValueError("brightness must by < 32");
	}

	epic_leds_set_rocket(led, value);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_set_rocket_obj, mp_leds_set_rocket);

static mp_obj_t mp_leds_get_rocket(mp_obj_t led_in)
{
	int led = mp_obj_get_int(led_in);
	int ret = epic_leds_get_rocket(led);
	if (ret == -EINVAL) {
		mp_raise_ValueError(
			"invalid value: maybe the led does not exists"
		);
	}
	return MP_OBJ_NEW_SMALL_INT(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_get_rocket_obj, mp_leds_get_rocket);

static mp_obj_t
mp_leds_flash_rocket(mp_obj_t led_in, mp_obj_t value_in, mp_obj_t time_in)
{
	int led   = mp_obj_get_int(led_in);
	int value = mp_obj_get_int(value_in);
	int time  = mp_obj_get_int(time_in);

	if (value > 31) {
		mp_raise_ValueError("brightness must be < 32");
	}

	epic_leds_flash_rocket(led, value, time);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(leds_flash_rocket_obj, mp_leds_flash_rocket);

static mp_obj_t mp_leds_dim_top(mp_obj_t dim_in)
{
	int dim = mp_obj_get_int(dim_in);

	if (dim > 8) {
		mp_raise_ValueError("maximum allowed dim is 8");
	}

	epic_leds_dim_top(dim);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_dim_top_obj, mp_leds_dim_top);

static mp_obj_t mp_leds_dim_bottom(mp_obj_t dim_in)
{
	int dim = mp_obj_get_int(dim_in);

	if (dim > 8) {
		mp_raise_ValueError("maximum allowed dim is 8");
	}

	epic_leds_dim_bottom(dim);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_dim_bottom_obj, mp_leds_dim_bottom);

static mp_obj_t mp_leds_update()
{
	epic_leds_update();
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(leds_update_obj, mp_leds_update);

static mp_obj_t mp_leds_set_powersave(mp_obj_t eco_in)
{
	int eco = mp_obj_get_int(eco_in);
	epic_leds_set_powersave(eco);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(leds_set_powersave_obj, mp_leds_set_powersave);

static mp_obj_t
mp_leds_set_gamma(mp_obj_t rgb_channel_in, mp_obj_t gamma_table_in)
{
	int rgb_channel = mp_obj_get_int(rgb_channel_in);
	if (mp_obj_get_int(mp_obj_len(gamma_table_in)) != 256) {
		mp_raise_ValueError("table must have 256 elements");
	}
	uint8_t gamma_table[256];
	for (int i = 0; i < 256; i++) {
		gamma_table[i] = mp_obj_get_int(mp_obj_subscr(
			gamma_table_in, mp_obj_new_int(i), MP_OBJ_SENTINEL)
		);
	}
	epic_leds_set_gamma_table(rgb_channel, gamma_table);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(leds_set_gamma_obj, mp_leds_set_gamma);

static const mp_rom_map_elem_t leds_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_leds) },
	{ MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&leds_set_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_hsv), MP_ROM_PTR(&leds_set_hsv_obj) },
	{ MP_ROM_QSTR(MP_QSTR_prep), MP_ROM_PTR(&leds_prep_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_rgb), MP_ROM_PTR(&leds_get_rgb_obj) },
	{ MP_ROM_QSTR(MP_QSTR_prep_hsv), MP_ROM_PTR(&leds_prep_hsv_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_all), MP_ROM_PTR(&leds_set_all_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_all_hsv), MP_ROM_PTR(&leds_set_all_hsv_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_rocket), MP_ROM_PTR(&leds_set_rocket_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_rocket), MP_ROM_PTR(&leds_get_rocket_obj) },
	{ MP_ROM_QSTR(MP_QSTR_flash_rocket),
	  MP_ROM_PTR(&leds_flash_rocket_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_flashlight),
	  MP_ROM_PTR(&leds_set_flashlight_obj) },
	{ MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&leds_update_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_powersave),
	  MP_ROM_PTR(&leds_set_powersave_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_gamma), MP_ROM_PTR(&leds_set_gamma_obj) },
	{ MP_ROM_QSTR(MP_QSTR_dim_top), MP_ROM_PTR(&leds_dim_top_obj) },
	{ MP_ROM_QSTR(MP_QSTR_dim_bottom), MP_ROM_PTR(&leds_dim_bottom_obj) },
	{ MP_ROM_QSTR(MP_QSTR_BOTTOM_RIGHT), MP_OBJ_NEW_SMALL_INT(11) },
	{ MP_ROM_QSTR(MP_QSTR_BOTTOM_LEFT), MP_OBJ_NEW_SMALL_INT(12) },
	{ MP_ROM_QSTR(MP_QSTR_TOP_RIGHT), MP_OBJ_NEW_SMALL_INT(13) },
	{ MP_ROM_QSTR(MP_QSTR_TOP_LEFT), MP_OBJ_NEW_SMALL_INT(14) },
};
static MP_DEFINE_CONST_DICT(leds_module_globals, leds_module_globals_table);

const mp_obj_module_t leds_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&leds_module_globals,
};

/* Register the module to make it available in Python */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_sys_leds, leds_module, MODULE_LEDS_ENABLED);
