#include "epicardium.h"

#include "py/builtin.h"
#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t mp_light_sensor_start()
{
	int status = epic_light_sensor_run();
	if (status == -EBUSY) {
		mp_raise_msg(
			&mp_type_RuntimeError, "timer could not be scheduled"
		);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(light_sensor_start_obj, mp_light_sensor_start);

static mp_obj_t mp_light_sensor_get_reading()
{
	uint16_t last;
	int status = epic_light_sensor_get(&last);
	if (status == -ENODATA) {
		mp_raise_ValueError("sensor not running");
		return mp_const_none;
	}
	return mp_obj_new_int_from_uint(last);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	light_sensor_get_obj, mp_light_sensor_get_reading
);

static mp_obj_t mp_light_sensor_stop()
{
	int status = epic_light_sensor_stop();
	if (status == -EBUSY) {
		mp_raise_msg(
			&mp_type_RuntimeError, "timer could not be stopped"
		);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(light_sensor_stop_obj, mp_light_sensor_stop);

static mp_obj_t mp_light_sensor_read()
{
	return mp_obj_new_int_from_uint(epic_light_sensor_read());
}
static MP_DEFINE_CONST_FUN_OBJ_0(light_sensor_read_obj, mp_light_sensor_read);

static const mp_rom_map_elem_t light_sensor_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_light_sensor) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&light_sensor_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&light_sensor_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_reading), MP_ROM_PTR(&light_sensor_get_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&light_sensor_read_obj) },
};
static MP_DEFINE_CONST_DICT(
	light_sensor_module_globals, light_sensor_module_globals_table
);

const mp_obj_module_t light_sensor_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&light_sensor_module_globals,
};

/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_light_sensor, light_sensor_module, MODULE_LIGHT_SENSOR_ENABLED);
