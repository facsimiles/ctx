/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George and 2017, 2018 Rami Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "library.h"
#include "mphalport.h"
#include "emscripten.h"

#include "py/runtime.h"

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_js_write(str, len);
}

int _mp_quit = 0;

EMSCRIPTEN_KEEPALIVE
int mp_has_quit (void)
{
  int had_quit = _mp_quit;
  _mp_quit = 0;
  return had_quit;
}

EMSCRIPTEN_KEEPALIVE
int mp_quit (void)
{
  _mp_quit = 1;
  return 0;
}

uint32_t epic_get_led (int led);

void gc_collect(void);
void mp_idle (int ms) // 0 means do garbage collection
{
    if (_mp_quit)
    {
      _mp_quit = 0;
      mp_raise_type(&mp_type_SystemExit);
    }
    if (ms == 0)
    {
 //   gc_collect();
      emscripten_sleep (0);
    }
    if (epic_get_led (0))
    {
       mp_js_update_leds ();
    }
}

void mp_hal_delay_ms(mp_uint_t ms) {
    uint32_t start = mp_hal_ticks_ms();
    while (mp_hal_ticks_ms() - start < ms) {
      mp_idle (ms/32+1);
    }
    mp_idle (0);
}

void mp_hal_delay_us(mp_uint_t us) {
    uint32_t start = mp_hal_ticks_us();
    while (mp_hal_ticks_us() - start < us) {
      mp_idle (us/32+1);
    }
    mp_idle (0);
}
#include <sys/time.h>
#define usecs(time) ((uint64_t)((time.tv_sec - start_time.tv_sec) * 1000000) + time.tv_usec)

uint64_t emscripten_ticks_us (void)
{
  static int done = 0;
  static struct timeval start_time;
  struct timeval time;
  if (!done)
  {
    gettimeofday (&start_time, NULL);
    done = 1;
  }
  gettimeofday (&time, NULL);
  return usecs (time) - usecs (start_time);
}

#define JS_TICKS 0

mp_uint_t mp_hal_ticks_us(void) {
#if JS_TICKS
    return mp_js_ticks_ms () * 1000;
#else
    return emscripten_ticks_us();
#endif
}

mp_uint_t mp_hal_ticks_ms(void) {
#if JS_TICKS
      return mp_js_ticks_ms ();
#else
      return emscripten_ticks_us() / 1000;
#endif
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return 0;
}

extern int mp_interrupt_char;

int mp_hal_get_interrupt_char(void) {
    return mp_interrupt_char;
}
