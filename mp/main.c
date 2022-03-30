/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2021 Damien P. George and 2017, 2018 Rami Ali
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"

#include "emscripten.h"
#include "library.h"

#include "ctx.h"

#if MICROPY_ENABLE_COMPILER

extern int _mp_quit;

Ctx *ctx_wasm_get_context(int memory_budget);

void epic_set_ctx (Ctx *ctx);
void ctx_wasm_reset (void);

void mp_js_init(int heap_size) {
    #if MICROPY_ENABLE_GC
    static char *heap = NULL;
    if (!heap) heap = (char *)malloc(heap_size * sizeof(char));
    gc_init(heap, heap + heap_size);
    #endif

    #if MICROPY_ENABLE_PYSTACK
    static mp_obj_t pystack[1024];
    mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
    #endif

    mp_init();
    ctx_wasm_reset();
#if 0
    epic_set_ctx (ctx_wasm_get_context(23*1024)); // these are the flags that
                                            // apply, first call to
                                            // create the singleton
#endif
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_lib));
}

void mp_js_init_repl() {
    pyexec_event_repl_init();
}


int mp_js_heap_size = 192*1024;

void mp_js_set_heap_size (int heap_size)
{
  mp_js_heap_size = heap_size;
}

int do_str(const char *src, mp_parse_input_kind_t input_kind)
{
    int ret = 0;
    mp_js_init (mp_js_heap_size);
    nlr_buf_t nlr;
    _mp_quit = 0;

    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        if (mp_obj_is_subclass_fast(mp_obj_get_type((mp_obj_t)nlr.ret_val), &mp_type_SystemExit)) {
            mp_obj_t exit_val = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(nlr.ret_val));
            if (exit_val != mp_const_none) {
                mp_int_t int_val;
                if (mp_obj_get_int_maybe(exit_val, &int_val)) {
                    ret = int_val & 255;
                } else {
                    ret = 1;
                }
            }
        } else {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            ret = 1;
        }
    }

    mp_js_init_repl();
    return ret;
}
#endif


int mp_js_process_char(int c) {
    return pyexec_event_repl_process_char(c);
}

int mp_js_do_str(const char *code) {
    return do_str(code, MP_PARSE_FILE_INPUT);
}

STATIC void gc_scan_func(void *begin, void *end) {
    gc_collect_root((void **)begin, (void **)end - (void **)begin + 1);
}

void gc_collect(void) {
    gc_collect_start();
    emscripten_scan_stack(gc_scan_func);
    emscripten_scan_registers(gc_scan_func);
    gc_collect_end();
}

void nlr_jump_fail(void *val) {
    fprintf (stderr, "%s\n", __FUNCTION__);
#if 1
    exit(0);
    while (1) {
        ;
    }
#endif
}

void NORETURN __fatal_error(const char *msg) {
    fprintf (stderr, "%s\n", __FUNCTION__);
#if 1
    exit(0);
    while (1) {
        ;
    }
#endif
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
