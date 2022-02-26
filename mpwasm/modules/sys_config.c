#include "epicardium.h"

#include "py/obj.h"
#include "py/runtime.h"

#include <string.h>
#include <strings.h>

#include <stdbool.h>

static void extract_string(
	mp_obj_t obj, char *buf, size_t buf_len, const char *error_message
) {
	size_t len;
	const char *str_ptr = mp_obj_str_get_data(obj, &len);
	/*
	 * The string retrieved from MicroPython is not NULL-terminated so we
	 * first need to copy it and add a NULL-byte.
	 */
	if (len > (buf_len - 1)) {
		mp_raise_ValueError(error_message);
	}
	memcpy(buf, str_ptr, len);
	buf[len] = '\0';
}

static mp_obj_t mp_config_set_string(mp_obj_t key_in, mp_obj_t value_in)
{
	const char *const forbidden_key = "execute_elf";

	char key_str[128], value_str[128];

	extract_string(key_in, key_str, sizeof(key_str), "key too long");
	extract_string(
		value_in, value_str, sizeof(value_str), "value too long"
	);

	if (strstr(key_str, forbidden_key) != NULL ||
	    strstr(value_str, forbidden_key) != NULL) {
		/* A Permission Error might be nice but is not available in MP */
		mp_raise_ValueError("Not allowed");
	}

	int status = epic_config_set_string(key_str, value_str);
	if (status < 0) {
		mp_raise_OSError(-status);
	}

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(set_string_obj, mp_config_set_string);

static mp_obj_t mp_config_get_string(mp_obj_t key_in)
{
	char key_str[128], value_str[128];
	extract_string(key_in, key_str, sizeof(key_str), "key too long");

	int status =
		epic_config_get_string(key_str, value_str, sizeof(value_str));
	if (status < 0) {
		mp_raise_OSError(-status);
	}

	mp_obj_t ret = mp_obj_new_str(value_str, strlen(value_str));
	return ret;
}
static MP_DEFINE_CONST_FUN_OBJ_1(get_string_obj, mp_config_get_string);

static const mp_rom_map_elem_t config_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_config) },
	{ MP_ROM_QSTR(MP_QSTR_set_string), MP_ROM_PTR(&set_string_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_string), MP_ROM_PTR(&get_string_obj) },
};

static MP_DEFINE_CONST_DICT(config_module_globals, config_module_globals_table);

// Define module object.
const mp_obj_module_t config_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&config_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_sys_config, config_module, MODULE_CONFIG_ENABLED);
