#include "epicardium.h"

#include "py/builtin.h"
#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t
mp_personal_state_set(mp_obj_t state_obj, mp_obj_t persistent_obj)
{
	int state = mp_obj_get_int(state_obj);
	int rc = epic_personal_state_set(state, mp_obj_is_true(persistent_obj));
	if (rc < 0)
		mp_raise_OSError(-rc);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(personal_state_set, mp_personal_state_set);

static mp_obj_t mp_personal_state_clear()
{
	int rc = epic_personal_state_set(STATE_NONE, 0);
	if (rc < 0)
		mp_raise_OSError(-rc);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(personal_state_clear, mp_personal_state_clear);

static mp_obj_t mp_personal_state_get()
{
	int state               = epic_personal_state_get();
	int persistent          = epic_personal_state_is_persistent();
	mp_obj_t persistent_obj = mp_const_false;
	if (persistent > 0)
		persistent_obj = mp_const_true;
	mp_obj_t tup[] = {
		mp_obj_new_int(state & 0xff),
		persistent_obj,
	};
	return mp_obj_new_tuple(2, tup);
}
static MP_DEFINE_CONST_FUN_OBJ_0(personal_state_get, mp_personal_state_get);

static const mp_rom_map_elem_t personal_state_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_personal_state) },
	{ MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&personal_state_set) },
	{ MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&personal_state_get) },
	{ MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&personal_state_clear) },
	{ MP_ROM_QSTR(MP_QSTR_NO_STATE), MP_OBJ_NEW_SMALL_INT(STATE_NONE) },
	{ MP_ROM_QSTR(MP_QSTR_NO_CONTACT),
	  MP_OBJ_NEW_SMALL_INT(STATE_NO_CONTACT) },
	{ MP_ROM_QSTR(MP_QSTR_CHAOS), MP_OBJ_NEW_SMALL_INT(STATE_CHAOS) },
	{ MP_ROM_QSTR(MP_QSTR_COMMUNICATION),
	  MP_OBJ_NEW_SMALL_INT(STATE_COMMUNICATION) },
	{ MP_ROM_QSTR(MP_QSTR_CAMP), MP_OBJ_NEW_SMALL_INT(STATE_CAMP) },
};
static MP_DEFINE_CONST_DICT(
	personal_state_module_globals, personal_state_module_globals_table
);

// Define module object.
const mp_obj_module_t personal_state_module = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&personal_state_module_globals,
};

/* Register the module to make it available in Python */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_personal_state, personal_state_module, MODULE_PERSONAL_STATE_ENABLED);
