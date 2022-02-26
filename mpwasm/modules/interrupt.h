#pragma once

#include "py/obj.h"

void interrupt_init0(void);

mp_obj_t mp_interrupt_set_callback(mp_obj_t id_in, mp_obj_t callback_obj);
mp_obj_t mp_interrupt_enable_callback(mp_obj_t id_in);
mp_obj_t mp_interrupt_disable_callback(mp_obj_t id_in);
