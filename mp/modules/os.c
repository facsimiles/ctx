#include "epicardium.h"

#include "py/obj.h"
#include "py/runtime.h"

#include <string.h>
#include <strings.h>

#include <stdbool.h>

#include "os.h"

bool pycrd_filename_restricted(const char *path)
{
	// files that cannot be opened in write modes
	const char *const forbidden_files[] = {
		"card10.bin", "menu.py", "main.py", "card10.cfg"
	};

	const char *fname = strchr(path, '/');
	while (fname) {
		path  = fname + 1;
		fname = strchr(path, '/');
	}
	fname = strchr(path, '\\');
	while (fname) {
		path  = fname + 1;
		fname = strchr(path, '\\');
	}
	fname = path;

	for (size_t i = 0;
	     i < sizeof(forbidden_files) / sizeof(forbidden_files[0]);
	     i++) {
		if (strcasecmp(fname, forbidden_files[i]) == 0) {
			return true;
		}
	}
	return false;
}


static mp_obj_t mp_os_exit(size_t n_args, const mp_obj_t *args)
{
        int ret = 0;
	if (n_args == 1) {
	 ret = mp_obj_get_int(args[0]);
	}

	epic_exec("/main.py");

        for (;;){if(ret){}}

	/* unreachable */
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(exit_obj, 0, 1, mp_os_exit);

static mp_obj_t mp_os_exec(mp_obj_t name_in)
{
	const char *name_ptr;
	char name_str[256];
	size_t len, maxlen;

	name_ptr = mp_obj_str_get_data(name_in, &len);

	/*
	 * The string retrieved from MicroPython is not NULL-terminated so we
	 * first need to copy it and add a NULL-byte.
	 */
	maxlen = len < (sizeof(name_str) - 1) ? len : (sizeof(name_str) - 1);
	memcpy(name_str, name_ptr, maxlen);
	name_str[maxlen] = '\0';

	int ret = epic_exec(name_str);

	/*
	 * If epic_exec() returns, something went wrong.  We can raise an
	 * exception in all cases.
	 */
        if(0) mp_raise_OSError(-ret);

	/* unreachable */
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(exec_obj, mp_os_exec);

static mp_obj_t mp_os_reset(void)
{
	epic_system_reset();

	/* unreachable */
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(reset_obj, mp_os_reset);

static mp_obj_t mp_os_sync(void)
{
	//epic_system_reset();
        EM_ASM(
          FS.syncfs(false, function (err) { });

        );

	/* unreachable */
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(sync_obj, mp_os_sync);

static mp_obj_t mp_os_listdir(size_t n_args, const mp_obj_t *args)
{
	const char *path;
	if (n_args == 1) {
		path = mp_obj_str_get_str(args[0]);
	} else {
		path = "";
	}

	int fd = epic_file_opendir(path);

	if (fd < 0) {
		mp_raise_OSError(-fd);
	}
	struct epic_stat entry;
	mp_obj_list_t *list = mp_obj_new_list(0, NULL);
	for (;;) {
		int res = epic_file_readdir(fd, &entry);
		if (res < 0) {
			m_del_obj(mp_obj_list_t, list);
			epic_file_close(fd);
			mp_raise_OSError(-res);
		}
		if (entry.type == EPICSTAT_NONE) {
			break;
		}
		mp_obj_list_append(
			list, mp_obj_new_str(entry.name, strlen(entry.name))
		);
	}
	epic_file_close(fd);
	return MP_OBJ_FROM_PTR(list);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(listdir_obj, 0, 1, mp_os_listdir);

static mp_obj_t mp_os_remove(mp_obj_t py_path)
{
	const char *path = mp_obj_str_get_str(py_path);
	if (pycrd_filename_restricted(path)) {
		mp_raise_OSError(-EACCES);
	}
	int rc = epic_file_unlink(path);

	if (rc < 0) {
		mp_raise_OSError(-rc);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(remove_obj, mp_os_remove);

static mp_obj_t mp_os_mkdir(mp_obj_t py_path)
{
	const char *path = mp_obj_str_get_str(py_path);
	int rc           = epic_file_mkdir(path);

	if (rc < 0) {
		mp_raise_OSError(-rc);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mkdir_obj, mp_os_mkdir);

static mp_obj_t mp_os_rename(mp_obj_t py_oldp, mp_obj_t py_newp)
{
	const char *oldp = mp_obj_str_get_str(py_oldp);
	const char *newp = mp_obj_str_get_str(py_newp);
	if (pycrd_filename_restricted(oldp) ||
	    pycrd_filename_restricted(newp)) {
		mp_raise_OSError(-EACCES);
	}
	int rc = epic_file_rename(oldp, newp);

	if (rc < 0) {
		mp_raise_OSError(-rc);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(rename_obj, mp_os_rename);

static mp_obj_t mp_os_read_battery()
{
	float result;
	int res = epic_read_battery_voltage(&result);
	if (res < 0) {
		mp_raise_OSError(-res);
	}
	return mp_obj_new_float(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(read_battery_obj, mp_os_read_battery);

#ifndef EMSCRIPTEN
static mp_obj_t mp_os_urandom(mp_obj_t size_in)
{
	size_t size = mp_obj_get_int(size_in);
	vstr_t vstr;

	vstr_init_len(&vstr, size);
	epic_csprng_read((uint8_t *)vstr.buf, size);

	return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
static MP_DEFINE_CONST_FUN_OBJ_1(urandom_obj, mp_os_urandom);

enum usb_config_device {
	USB_DEVICE_NONE,
	USB_DEVICE_FLASH,
	USB_DEVICE_SERIAL,
};

static mp_obj_t mp_os_usbconfig(mp_obj_t dev)
{
	int device = mp_obj_get_int(dev);
	switch (device) {
	case USB_DEVICE_NONE:
		epic_usb_shutdown();
		break;
	case USB_DEVICE_FLASH:
		epic_usb_storage();
		break;
	case USB_DEVICE_SERIAL:
		epic_usb_cdcacm();
		break;
	default:
		mp_raise_ValueError("Invalid parameter");
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(usbconfig_obj, mp_os_usbconfig);

static mp_obj_t mp_os_fs_is_attached(void)
{
	if (epic_fs_is_attached()) {
		return mp_const_true;
	} else {
		return mp_const_false;
	}
}
static MP_DEFINE_CONST_FUN_OBJ_0(fs_is_attached_obj, mp_os_fs_is_attached);
#endif

static const mp_rom_map_elem_t os_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_os) },
	{ MP_ROM_QSTR(MP_QSTR_exit), MP_ROM_PTR(&exit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&reset_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sync), MP_ROM_PTR(&sync_obj) },
	{ MP_ROM_QSTR(MP_QSTR_exec), MP_ROM_PTR(&exec_obj) },
	{ MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&listdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&remove_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mkdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&rename_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_battery), MP_ROM_PTR(&read_battery_obj) },
#ifndef EMSCRIPTEN
	{ MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&urandom_obj) },
	{ MP_ROM_QSTR(MP_QSTR_usbconfig), MP_ROM_PTR(&usbconfig_obj) },
	{ MP_ROM_QSTR(MP_QSTR_fs_is_attached),
	  MP_ROM_PTR(&fs_is_attached_obj) },

	{ MP_ROM_QSTR(MP_QSTR_USB_SERIAL), MP_ROM_INT(USB_DEVICE_SERIAL) },
	{ MP_ROM_QSTR(MP_QSTR_USB_FLASH), MP_ROM_INT(USB_DEVICE_FLASH) },
	{ MP_ROM_QSTR(MP_QSTR_USB_NONE), MP_ROM_INT(USB_DEVICE_NONE) },
#endif
};

static MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

// Define module object.
const mp_obj_module_t os_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&os_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_os, os_module, MODULE_OS_ENABLED);

