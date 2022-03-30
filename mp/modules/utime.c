#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objint.h"
#include "py/runtime.h"
#include "extmod/utime_mphal.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

// Needs to be after the stdint include ...
#include "shared/timeutils/timeutils.h"

/* MicroPython has its epoch at 2000-01-01. Our RTC is in UTC */
#define EPOCH_OFFSET 946684800UL

/* Default time zone: CET */
#define TZONE_OFFSET 3600UL

uint64_t emscripten_ticks_us (void);

static uint32_t time_get_timezone_offset(void)
{
	static uint32_t offset = INT32_MAX;

	/* Only read the timezone setting once after starting
	 * the interpeter to not impact performance */
	if (offset == INT32_MAX) {
		offset = TZONE_OFFSET;

		char buf[128]="+0100";
		int ret = 0;//mp_vfs_config_get_string("timezone", buf, sizeof(buf));

		/* Understands formats like +0100, 0100, 100, -0800 */
		if (ret == 0 && strlen(buf) > 0) {
			errno             = 0;
			long int timezone = strtol(buf, NULL, 10);
			int sign          = 1;
			if (timezone < 0) {
				sign     = -1;
				timezone = -timezone;
			}
			if (errno == 0) {
				offset = (timezone / 100) * 3600;
				offset += (timezone % 100) * 60;
				offset *= sign;
			}
		}
	}
	return offset;
}

static mp_obj_t time_set_time(mp_obj_t secs)
{
	//uint64_t timestamp = mp_obj_get_int(secs) * 1000ULL +
	//		     EPOCH_OFFSET * 1000ULL -
	//		     time_get_timezone_offset() * 1000ULL;
	// NYI
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(time_set_time_obj, time_set_time);

static mp_obj_t time_set_time_ms(mp_obj_t msecs_obj)
{
	//uint64_t msecs = 0;
	//mp_obj_int_to_bytes_impl(msecs_obj, false, 8, (byte *)&msecs);
	//uint64_t timestamp = msecs + EPOCH_OFFSET * 1000ULL -
	//		     time_get_timezone_offset() * 1000ULL;
	// NYI
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(time_set_time_ms_obj, time_set_time_ms);

static mp_obj_t time_set_unix_time(mp_obj_t secs)
{
	//uint64_t timestamp = mp_obj_get_int(secs) * 1000ULL;
        // NYI
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(time_set_unix_time_obj, time_set_unix_time);

static mp_obj_t time_set_unix_time_ms(mp_obj_t msecs_obj)
{
	//uint64_t timestamp = 0;
	//mp_obj_int_to_bytes_impl(msecs_obj, false, 8, (byte *)&timestamp);
        // NYI
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
	time_set_unix_time_ms_obj, time_set_unix_time_ms
);

static mp_obj_t time_time(void)
{
	mp_int_t seconds;
	seconds = 
               (emscripten_ticks_us () / 1000 / 1000)
                - EPOCH_OFFSET +
		  time_get_timezone_offset();
	return mp_obj_new_int(seconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_time_obj, time_time);

static mp_obj_t time_time_ms(void)
{
	uint64_t milliseconds;
	milliseconds = (emscripten_ticks_us () / 1000)
                - EPOCH_OFFSET * 1000ULL +
		       time_get_timezone_offset() * 1000ULL;
	return mp_obj_new_int_from_ull(milliseconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_time_ms_obj, time_time_ms);

static mp_obj_t time_unix_time(void)
{
	mp_int_t seconds = (emscripten_ticks_us () / 1000 / 1000);
	return mp_obj_new_int(seconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_unix_time_obj, time_unix_time);

static mp_obj_t time_unix_time_ms(void)
{
	uint64_t milliseconds = (emscripten_ticks_us () / 1000);
	return mp_obj_new_int_from_ull(milliseconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_unix_time_ms_obj, time_unix_time_ms);

static mp_obj_t time_monotonic(void)
{
	mp_int_t seconds;
        seconds = emscripten_ticks_us () / 1000 / 1000;
	return mp_obj_new_int(seconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_monotonic_obj, time_monotonic);

static mp_obj_t time_monotonic_ms(void)
{
	uint64_t milliseconds;
	milliseconds = emscripten_ticks_us () / 1000;
	return mp_obj_new_int_from_ull(milliseconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_monotonic_ms_obj, time_monotonic_ms);

static mp_obj_t time_localtime(size_t n_args, const mp_obj_t *args)
{
	mp_int_t seconds;

	if (n_args == 0 || args[0] == mp_const_none) {
	        seconds = (emscripten_ticks_us () / 1000 / 1000);
		seconds += time_get_timezone_offset() - EPOCH_OFFSET;
	} else {
		seconds = mp_obj_get_int(args[0]);
	}

	timeutils_struct_time_t tm;
	timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
	mp_obj_t tuple[8] = {
		tuple[0] = mp_obj_new_int(tm.tm_year),
		tuple[1] = mp_obj_new_int(tm.tm_mon),
		tuple[2] = mp_obj_new_int(tm.tm_mday),
		tuple[3] = mp_obj_new_int(tm.tm_hour),
		tuple[4] = mp_obj_new_int(tm.tm_min),
		tuple[5] = mp_obj_new_int(tm.tm_sec),
		tuple[6] = mp_obj_new_int(tm.tm_wday),
		tuple[7] = mp_obj_new_int(tm.tm_yday),
	};
	return mp_obj_new_tuple(8, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
	time_localtime_obj, 0, 1, time_localtime
);

static mp_obj_t time_mktime(mp_obj_t tuple)
{
	size_t len;
	mp_obj_t *elem;
	mp_obj_get_array(tuple, &len, &elem);

	/* localtime generates a tuple of len 8. CPython uses 9, so we accept both. */
	if (len < 8 || len > 9) {
		nlr_raise(mp_obj_new_exception_msg_varg(
			&mp_type_TypeError,
			"mktime needs a tuple of length 8 or 9 (%d given)",
			len)
		);
	}

	return mp_obj_new_int_from_uint(timeutils_mktime(
		mp_obj_get_int(elem[0]),
		mp_obj_get_int(elem[1]),
		mp_obj_get_int(elem[2]),
		mp_obj_get_int(elem[3]),
		mp_obj_get_int(elem[4]),
		mp_obj_get_int(elem[5]))
	);
}
static MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

/* Schedule an alarm */
static mp_obj_t time_alarm(size_t n_args, const mp_obj_t *args)
{
#if 0
	mp_int_t timestamp = mp_obj_get_int(args[0]) + EPOCH_OFFSET -
			     time_get_timezone_offset();
	if (n_args == 2) {
		/* If a callback was given, register it for the RTC Alarm */
		mp_obj_t callback = args[1];
		mp_obj_t irq_id   = MP_OBJ_NEW_SMALL_INT(MP_VFS_INT_RTC_ALARM);
		mp_interrupt_set_callback(irq_id, callback);
		mp_interrupt_enable_callback(irq_id);
	}

	int res = mp_vfs_rtc_schedule_alarm(timestamp);
	if (res < 0) {
		mp_raise_OSError(-res);
	}
#endif
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_alarm_obj, 1, 2, time_alarm);

static const mp_rom_map_elem_t time_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_utime) },
	{ MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&time_time_obj) },
	{ MP_ROM_QSTR(MP_QSTR_time_ms), MP_ROM_PTR(&time_time_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_unix_time), MP_ROM_PTR(&time_unix_time_obj) },
	{ MP_ROM_QSTR(MP_QSTR_unix_time_ms),
	  MP_ROM_PTR(&time_unix_time_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_monotonic), MP_ROM_PTR(&time_monotonic_obj) },
	{ MP_ROM_QSTR(MP_QSTR_monotonic_ms),
	  MP_ROM_PTR(&time_monotonic_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_time), MP_ROM_PTR(&time_set_time_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_time_ms), MP_ROM_PTR(&time_set_time_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_unix_time),
	  MP_ROM_PTR(&time_set_unix_time_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_unix_time_ms),
	  MP_ROM_PTR(&time_set_unix_time_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_localtime), MP_ROM_PTR(&time_localtime_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mktime), MP_ROM_PTR(&time_mktime_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mp_utime_sleep_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sleep_us), MP_ROM_PTR(&mp_utime_sleep_us_obj) },
	{ MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
	{ MP_ROM_QSTR(MP_QSTR_alarm), MP_ROM_PTR(&time_alarm_obj) },
#if 0
	/* TODO: Implement those */
	{MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj)},
	{MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj)},
	{MP_ROM_QSTR(MP_QSTR_ticks_cpu), MP_ROM_PTR(&mp_utime_ticks_cpu_obj)},
	{MP_ROM_QSTR(MP_QSTR_ticks_add), MP_ROM_PTR(&mp_utime_ticks_add_obj)},
	{MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mp_utime_ticks_diff_obj)},
#endif
};
static MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t mp_module_utime = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&time_module_globals,
};

/* Register the module to make it available in Python */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_utime, mp_module_utime, MODULE_UTIME_ENABLED);
