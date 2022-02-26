#include "epicardium.h"

#include "py/builtin.h"
#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t mp_power_read_battery_voltage()
{
	float result;
	int status = epic_read_battery_voltage(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_battery_voltage_obj, mp_power_read_battery_voltage
);

static mp_obj_t mp_power_read_battery_current()
{
	float result;
	int status = epic_read_battery_current(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_battery_current_obj, mp_power_read_battery_current
);

static mp_obj_t mp_power_read_chargein_voltage()
{
	float result;
	int status = epic_read_chargein_voltage(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_chargein_voltage_obj, mp_power_read_chargein_voltage
);

static mp_obj_t mp_power_read_chargein_current()
{
	float result;
	int status = epic_read_chargein_current(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_chargein_current_obj, mp_power_read_chargein_current
);

static mp_obj_t mp_power_read_system_voltage()
{
	float result;
	int status = epic_read_system_voltage(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_system_voltage_obj, mp_power_read_system_voltage
);

static mp_obj_t mp_power_read_thermistor_voltage()
{
	float result;
	int status = epic_read_thermistor_voltage(&result);
	if (status < 0) {
		mp_raise_OSError(-status);
		return mp_const_none;
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(
	power_read_thermistor_voltage_obj, mp_power_read_thermistor_voltage
);

static const mp_rom_map_elem_t power_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_power) },
	{ MP_ROM_QSTR(MP_QSTR_read_battery_voltage),
	  MP_ROM_PTR(&power_read_battery_voltage_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_battery_current),
	  MP_ROM_PTR(&power_read_battery_current_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_chargein_voltage),
	  MP_ROM_PTR(&power_read_chargein_voltage_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_chargein_current),
	  MP_ROM_PTR(&power_read_chargein_current_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_system_voltage),
	  MP_ROM_PTR(&power_read_system_voltage_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_thermistor_voltage),
	  MP_ROM_PTR(&power_read_thermistor_voltage_obj) },
};
static MP_DEFINE_CONST_DICT(power_module_globals, power_module_globals_table);

const mp_obj_module_t power_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&power_module_globals,
};

/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_power, power_module, MODULE_POWER_ENABLED);
