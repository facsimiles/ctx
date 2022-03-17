#include "epicardium.h"

#include "py/builtin.h"
#include "py/binary.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/gc.h"

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void *lodepng_malloc(size_t size)
{
	return m_malloc(size);
}

void *lodepng_realloc(void *ptr, size_t new_size)
{
	return m_realloc(ptr, new_size);
}

void lodepng_free(void *ptr)
{
	m_free(ptr);
}

static mp_obj_t mp_image_decode(size_t n_args, const mp_obj_t *args)
{
	mp_buffer_info_t png_info;

	mp_obj_t png       = args[0];

	/* Load buffer and ensure it contains enough data */
	if (!mp_get_buffer(png, &png_info, MP_BUFFER_READ)) {
		mp_raise_TypeError("not a buffer");
	}

	int w, h;
	uint8_t *raw;
	int raw_len;
        raw = stbi_load_from_memory (png_info.buf, png_info.len,
                        &w, &h, NULL, 4);
        if (!raw)
           mp_raise_TypeError("failed to decode image");
        raw_len = w * h * 4;


	mp_obj_t mp_w   = MP_OBJ_NEW_SMALL_INT(w);
	mp_obj_t mp_h   = MP_OBJ_NEW_SMALL_INT(h);
	mp_obj_t mp_raw = mp_obj_new_memoryview(
		MP_OBJ_ARRAY_TYPECODE_FLAG_RW | BYTEARRAY_TYPECODE,
		raw_len,
		raw
	);
	mp_obj_t tup[] = { mp_w, mp_h, mp_raw };

	return mp_obj_new_tuple(3, tup);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(decode, 1, 1, mp_image_decode);

static int read_cb (void *user, char *data, int size)
{
  int fd = (int)user;
  return epic_file_read (fd, data, size);
}

static void skip_cb (void *user, int n)
{
  int fd = (int)user;
  int pos = epic_file_tell (fd);
  epic_file_seek (fd, pos + n, SEEK_SET);
}

static int file_size = 0;

static int eof_cb (void *user)
{
  int fd = (int)user;
  int pos = epic_file_tell (fd);
  if (pos >= file_size)
          return 1;
  return 0;
}


static stbi_io_callbacks clbk = {read_cb, skip_cb, eof_cb};



static mp_obj_t mp_image_load(size_t n_args, const mp_obj_t *args)
{
	const char *path = mp_obj_str_get_str (args[0]);
	int w, h;
	uint8_t *raw;
	int raw_len;

        int f = epic_file_open (path, "rb");
        if (f<0)
          mp_raise_ValueError("unable to open file");
        epic_file_seek (f, 0, SEEK_END);
        file_size = epic_file_tell (f);
        epic_file_seek (f, 0, SEEK_SET);

        raw = stbi_load_from_callbacks (&clbk, (void*)f,
                        &w, &h, NULL, 4);
        epic_file_close (f);
        if (!raw)
          mp_raise_ValueError("problem decoding image");
        raw_len = w * h * 4;

	mp_obj_t mp_w   = MP_OBJ_NEW_SMALL_INT(w);
	mp_obj_t mp_h   = MP_OBJ_NEW_SMALL_INT(h);
	mp_obj_t mp_raw = mp_obj_new_memoryview(
		MP_OBJ_ARRAY_TYPECODE_FLAG_RW | BYTEARRAY_TYPECODE,
		raw_len,
		raw
	);
	mp_obj_t tup[] = { mp_w, mp_h, mp_raw };

	return mp_obj_new_tuple(3, tup);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(load, 1, 1, mp_image_load);

static const mp_rom_map_elem_t image_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_image) },
	{ MP_ROM_QSTR(MP_QSTR_decode), MP_ROM_PTR(&decode) },
	{ MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&load) },
};
static MP_DEFINE_CONST_DICT(image_module_globals, image_module_globals_table);

// Define module object.
const mp_obj_module_t image_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&image_module_globals,
};

/* Register the module to make it available in Python */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_image, image_module, MODULE_IMAGE_ENABLED);
