/*
 * based on micropython/ports/unix/file.c
 */

#include <stdio.h>

#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/stream.h"
#include "py/mperrno.h"

#include "vfs.h"

extern const mp_obj_type_t mp_type_textio;
#if MICROPY_PY_IO_FILEIO
extern const mp_obj_type_t mp_type_fileio;
#endif

typedef struct _fat_py_file_obj_t {
	mp_obj_base_t base;
	int fd;
} fat_py_file_obj_t;

STATIC void
file_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
	(void)kind;
	mp_printf(
		print,
		"<io.%s %p>",
		mp_obj_get_type_str(self_in),
		MP_OBJ_TO_PTR(self_in)
	);
}

STATIC mp_uint_t
file_obj_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode)
{
	fat_py_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int res                 = mp_vfs_file_read(self->fd, buf, size);
	if (res < 0) {
		*errcode = -res;
		return MP_STREAM_ERROR;
	}
	return res;
}

STATIC mp_uint_t
file_obj_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode)
{
	fat_py_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int res                 = mp_vfs_file_write(self->fd, buf, size);
	if (res < 0) {
		*errcode = -res;
		return MP_STREAM_ERROR;
	}
	return res;
}

STATIC mp_obj_t file_obj___exit__(size_t n_args, const mp_obj_t *args)
{
	(void)n_args;
	return mp_stream_close(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
	file_obj___exit___obj, 4, 4, file_obj___exit__
);

STATIC mp_uint_t
file_obj_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode)
{
	fat_py_file_obj_t *self = MP_OBJ_TO_PTR(o_in);
	int res;
	switch (request) {
	case MP_STREAM_FLUSH:
#ifdef EMSCRIPTEN
                fflush(NULL);
                res = 0;
#else
		res = mp_vfs_file_flush(self->fd);
#endif
		if (res < 0) {
			*errcode = -res;
			return MP_STREAM_ERROR;
		}
		return 0;
	case MP_STREAM_CLOSE:
		res = mp_vfs_file_close(self->fd);
		if (res < 0) {
			*errcode = -res;
			return MP_STREAM_ERROR;
		}
		return 0;
	case MP_STREAM_SEEK: {
		struct mp_stream_seek_t *s =
			(struct mp_stream_seek_t *)(uintptr_t)arg;
		res = mp_vfs_file_seek(self->fd, s->offset, s->whence);
		if (res < 0) {
			*errcode = -res;
			return MP_STREAM_ERROR;
		}
		s->offset = mp_vfs_file_tell(self->fd);
		return 0;
	}
	}
	//every valid case returns either success or error, so this is EINVAL land:
	*errcode = MP_EINVAL;
	return MP_STREAM_ERROR;
}

// Note: encoding is ignored for now; it's also not a valid kwarg for CPython's FileIO,
// but by adding it here we can use one single mp_arg_t array for open() and FileIO's constructor
STATIC const mp_arg_t file_open_args[] = {
	{ MP_QSTR_file,
	  MP_ARG_OBJ | MP_ARG_REQUIRED,
	  { .u_rom_obj = mp_const_none } },
	{ MP_QSTR_mode, MP_ARG_OBJ, { .u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r) } },
	{ MP_QSTR_encoding,
	  MP_ARG_OBJ | MP_ARG_KW_ONLY,
	  { .u_rom_obj = mp_const_none } },
};
#define FILE_OPEN_NUM_ARGS MP_ARRAY_SIZE(file_open_args)

STATIC mp_obj_t file_open(const mp_obj_type_t *type, mp_arg_val_t *args)
{
	//bool potentially_critical_access = false;
	const char *modeString           = mp_obj_str_get_str(args[1].u_obj);
	const char *mode_s               = modeString;
	// modes r w x a + are handled on epicardium side, binary / text
	// is relevant for python type so look for these here
	while (*mode_s) {
		switch (*mode_s++) {
#if MICROPY_PY_IO_FILEIO
		case 'b':
			type = &mp_type_fileio;
			break;
#endif
		case 't':
			type = &mp_type_textio;
			break;
		case 'w':
		case 'x':
		case 'a':
		case '+':
			//potentially_critical_access = true;
			break;
		}
	}

	fat_py_file_obj_t *o = m_new_obj_with_finaliser(fat_py_file_obj_t);
	o->base.type         = type;

	const char *fname = mp_obj_str_get_str(args[0].u_obj);

	int res = mp_vfs_file_open(fname, modeString);
	if (res < 0) {
		m_del_obj(fat_py_file_obj_t, o);
		mp_raise_OSError(-res);
	}
	o->fd = res;

	return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t file_obj_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *args
) {
	mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
	mp_arg_parse_all_kw_array(
		n_args,
		n_kw,
		args,
		FILE_OPEN_NUM_ARGS,
		file_open_args,
		arg_vals
	);
	return file_open(type, arg_vals);
}

// TODO gc hook to close the file if not already closed

STATIC const mp_rom_map_elem_t rawfile_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readline),
	  MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readlines),
	  MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
	{ MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&file_obj___exit___obj) },
};

STATIC MP_DEFINE_CONST_DICT(rawfile_locals_dict, rawfile_locals_dict_table);

#if MICROPY_PY_IO_FILEIO
STATIC const mp_stream_p_t fileio_stream_p = {
	.read  = file_obj_read,
	.write = file_obj_write,
	.ioctl = file_obj_ioctl,
};

const mp_obj_type_t mp_type_fileio = {
	{ &mp_type_type },
	.name        = MP_QSTR_FileIO,
	.print       = file_obj_print,
	.make_new    = file_obj_make_new,
	.getiter     = mp_identity_getiter,
	.iternext    = mp_stream_unbuffered_iter,
	.protocol    = &fileio_stream_p,
	.locals_dict = (mp_obj_dict_t *)&rawfile_locals_dict,
};
#endif

STATIC const mp_stream_p_t textio_stream_p = {
	.read    = file_obj_read,
	.write   = file_obj_write,
	.ioctl   = file_obj_ioctl,
	.is_text = true,
};

const mp_obj_type_t mp_type_textio = {
	{ &mp_type_type },
	.name        = MP_QSTR_TextIOWrapper,
	.print       = file_obj_print,
	.make_new    = file_obj_make_new,
	.getiter     = mp_identity_getiter,
	.iternext    = mp_stream_unbuffered_iter,
	.protocol    = &textio_stream_p,
	.locals_dict = (mp_obj_dict_t *)&rawfile_locals_dict,
};

// Factory function for I/O stream classes
mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
	// TODO: analyze buffering args and instantiate appropriate type
	mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
	mp_arg_parse_all(
		n_args,
		args,
		kwargs,
		FILE_OPEN_NUM_ARGS,
		file_open_args,
		arg_vals
	);
	return file_open(&mp_type_textio, arg_vals);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
