#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "epicardium.h"
#include "mphalport.h"

extern const mp_obj_type_t mp_type_bhi160_sample;

STATIC mp_obj_t mp_bhi160_enable_sensor(size_t n_args, const mp_obj_t *args)
{
	int sensor_type = mp_obj_get_int(args[0]);

	struct bhi160_sensor_config cfg = { 0 };
	cfg.sample_buffer_len           = mp_obj_get_int(args[1]);
	cfg.sample_rate                 = mp_obj_get_int(args[2]);
	cfg.dynamic_range               = mp_obj_get_int(args[3]);

	int stream_id = epic_bhi160_enable_sensor(sensor_type, &cfg);

	return MP_OBJ_NEW_SMALL_INT(stream_id);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
	mp_bhi160_enable_sensor_obj, 4, 4, mp_bhi160_enable_sensor
);

STATIC mp_obj_t mp_bhi160_read_sensor(mp_obj_t stream_id_in)
{
	struct bhi160_data_vector buf[100];
	int stream_id = mp_obj_get_int(stream_id_in);

	int n = epic_stream_read(stream_id, buf, sizeof(buf));

	mp_obj_list_t *list = mp_obj_new_list(0, NULL);
	for (int i = 0; i < n; i++) {
		if (buf[i].data_type != BHI160_DATA_TYPE_VECTOR) {
			// other data types are currently not supported
			mp_raise_OSError(EINVAL);
		}
		mp_obj_t tuple[4];
		tuple[0] = mp_obj_new_int(buf[i].x);
		tuple[1] = mp_obj_new_int(buf[i].y);
		tuple[2] = mp_obj_new_int(buf[i].z);
		tuple[3] = mp_obj_new_int(buf[i].status);
		mp_obj_list_append(list, mp_obj_new_tuple(4, tuple));
	}

	return MP_OBJ_FROM_PTR(list);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(
	mp_bhi160_read_sensor_obj, mp_bhi160_read_sensor
);

STATIC mp_obj_t mp_bhi160_disable_sensor(mp_obj_t sensor_type_in)
{
	int sensor_type = mp_obj_get_int(sensor_type_in);

	int ret = epic_bhi160_disable_sensor(sensor_type);

	return MP_OBJ_NEW_SMALL_INT(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(
	mp_bhi160_disable_sensor_obj, mp_bhi160_disable_sensor
);

STATIC mp_obj_t mp_bhi160_disable_all_sensors()
{
	epic_bhi160_disable_all_sensors();

	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(
	mp_bhi160_disable_all_sensors_obj, mp_bhi160_disable_all_sensors
);

STATIC const mp_rom_map_elem_t bhi160_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_bhi160) },
	{ MP_ROM_QSTR(MP_QSTR_enable_sensor),
	  MP_ROM_PTR(&mp_bhi160_enable_sensor_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_sensor),
	  MP_ROM_PTR(&mp_bhi160_read_sensor_obj) },
	{ MP_ROM_QSTR(MP_QSTR_disable_sensor),
	  MP_ROM_PTR(&mp_bhi160_disable_sensor_obj) },
	{ MP_ROM_QSTR(MP_QSTR_disable_all_sensors),
	  MP_ROM_PTR(&mp_bhi160_disable_all_sensors_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bhi160_module_globals, bhi160_module_globals_table);

// Define module object.
const mp_obj_module_t bhi160_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&bhi160_module_globals,
};

/* clang-format off */
// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_sys_bhi160, bhi160_module, MODULE_BHI160_ENABLED);
