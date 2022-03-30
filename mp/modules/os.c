#include "vfs.h"

#include "py/obj.h"
#include "py/runtime.h"

#include <string.h>
#include <strings.h>

#include <stdbool.h>

static mp_obj_t mp_os_reset(void)
{
	/* unreachable */
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(reset_obj, mp_os_reset);

static mp_obj_t mp_os_sync(void)
{
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

	int fd = mp_vfs_file_opendir(path);

	if (fd < 0) {
		mp_raise_OSError(-fd);
	}
	struct mp_vfs_stat entry;
	mp_obj_list_t *list = mp_obj_new_list(0, NULL);
	for (;;) {
		int res = mp_vfs_file_readdir(fd, &entry);
		if (res < 0) {
			m_del_obj(mp_obj_list_t, list);
			mp_vfs_file_close(fd);
			mp_raise_OSError(-res);
		}
		if (entry.type == EPICSTAT_NONE) {
			break;
		}
		mp_obj_list_append(
			list, mp_obj_new_str(entry.name, strlen(entry.name))
		);
	}
	mp_vfs_file_close(fd);
	return MP_OBJ_FROM_PTR(list);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(listdir_obj, 0, 1, mp_os_listdir);

static mp_obj_t mp_os_remove(mp_obj_t py_path)
{
	const char *path = mp_obj_str_get_str(py_path);
	int rc = mp_vfs_file_unlink(path);

	if (rc < 0) {
		mp_raise_OSError(-rc);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(remove_obj, mp_os_remove);

static mp_obj_t mp_os_mkdir(mp_obj_t py_path)
{
	const char *path = mp_obj_str_get_str(py_path);
	int rc           = mp_vfs_file_mkdir(path);

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
	int rc = mp_vfs_file_rename(oldp, newp);

	if (rc < 0) {
		mp_raise_OSError(-rc);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(rename_obj, mp_os_rename);


static const mp_rom_map_elem_t os_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_os) },
	{ MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&reset_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sync), MP_ROM_PTR(&sync_obj) },
	{ MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&listdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&remove_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mkdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&rename_obj) },
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

