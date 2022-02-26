#include "py/obj.h"
#include "py/runtime.h"

#include "epicardium.h"

static mp_obj_t mp_gpio_set_mode(mp_obj_t pin_obj, mp_obj_t mode_obj)
{
	int pin  = mp_obj_get_int(pin_obj);
	int mode = mp_obj_get_int(mode_obj);
	int rc   = epic_gpio_set_pin_mode(pin, mode);
	if (rc < 0)
		mp_raise_OSError(-rc);

	return mp_const_none;
};
static MP_DEFINE_CONST_FUN_OBJ_2(gpio_set_mode, mp_gpio_set_mode);

static mp_obj_t mp_gpio_get_mode(mp_obj_t pin_obj)
{
	int pin  = mp_obj_get_int(pin_obj);
	int mode = epic_gpio_get_pin_mode(pin);
	if (mode < 0)
		mp_raise_OSError(-mode);

	return MP_OBJ_NEW_SMALL_INT(mode);
};
static MP_DEFINE_CONST_FUN_OBJ_1(gpio_get_mode, mp_gpio_get_mode);

static mp_obj_t mp_gpio_read(mp_obj_t pin_obj)
{
	int pin   = mp_obj_get_int(pin_obj);
	int value = epic_gpio_read_pin(pin);
	if (value < 0)
		mp_raise_OSError(-value);

	return MP_OBJ_NEW_SMALL_INT(value);
};
static MP_DEFINE_CONST_FUN_OBJ_1(gpio_read, mp_gpio_read);

static mp_obj_t mp_gpio_write(mp_obj_t pin_obj, mp_obj_t on_obj)
{
	int pin = mp_obj_get_int(pin_obj);
	bool on = mp_obj_is_true(on_obj);
	int rc  = epic_gpio_write_pin(pin, on);
	if (rc < 0)
		mp_raise_OSError(-rc);

	return mp_const_none;
};
static MP_DEFINE_CONST_FUN_OBJ_2(gpio_write, mp_gpio_write);

static const mp_rom_map_elem_t gpio_module_modes_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mode) },
	{ MP_ROM_QSTR(MP_QSTR_INPUT), MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_MODE_IN) },
	{ MP_ROM_QSTR(MP_QSTR_OUTPUT),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_MODE_OUT) },
	{ MP_ROM_QSTR(MP_QSTR_ADC), MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_MODE_ADC) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_UP),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_PULL_UP) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_DOWN),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_PULL_DOWN) },
};
static MP_DEFINE_CONST_DICT(gpio_module_modes_dict, gpio_module_modes_table);

const mp_obj_module_t gpio_module_modes = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&gpio_module_modes_dict,
};

/* The globals table for this module */
static const mp_rom_map_elem_t gpio_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gpio) },
	{ MP_ROM_QSTR(MP_QSTR_set_mode), MP_ROM_PTR(&gpio_set_mode) },
	{ MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&gpio_get_mode) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&gpio_read) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&gpio_write) },
	{ MP_ROM_QSTR(MP_QSTR_WRISTBAND_1),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_WRISTBAND_1) },
	{ MP_ROM_QSTR(MP_QSTR_WRISTBAND_2),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_WRISTBAND_2) },
	{ MP_ROM_QSTR(MP_QSTR_WRISTBAND_3),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_WRISTBAND_3) },
	{ MP_ROM_QSTR(MP_QSTR_WRISTBAND_4),
	  MP_OBJ_NEW_SMALL_INT(EPIC_GPIO_WRISTBAND_4) },
	{ MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&gpio_module_modes) },
};
static MP_DEFINE_CONST_DICT(gpio_module_globals, gpio_module_globals_table);

const mp_obj_module_t gpio_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&gpio_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_gpio, gpio_module, MODULE_GPIO_ENABLED);
