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

#define CTX_TINYVG 1
#define CTX_DITHER 1

#define CTX_LIMIT_FORMATS       1
#define CTX_32BIT_SEGMENTS      0
#define CTX_ENABLE_RGB565             1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_BITPACK_PACKER      0
#define CTX_COMPOSITING_GROUPS  0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_GRADIENT_CACHE      1
#define CTX_ENABLE_CLIP         1
#define CTX_BLOATY_FAST_PATHS   0
#define CTX_1BIT_CLIP           1
#define CTX_CM                  0
#define CTX_SHAPE_CACHE         0
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 42
#define CTX_ENABLE_SHADOW_BLUR  0
#define CTX_FORMATTER           0
#define CTX_PARSER              0
#define CTX_FONTS_FROM_FILE     0
#define CTX_MAX_KEYDB          10
#define CTX_FRAGMENT_SPECIALIZE 1
#define CTX_ENABLE_RGBA8        1
#define CTX_FAST_FILL_RECT      1
#define CTX_MAX_TEXTURES        1
#define CTX_PARSER_MAXLEN       512
#define CTX_PARSER_FIXED_TEMP   1
#define CTX_CURRENT_PATH        1
#define CTX_BLOATY_FAST_PATHS        0
#define CTX_BLENDING_AND_COMPOSITING 0
#define CTX_STRINGPOOL_SIZE        256
#define CTX_MIN_EDGE_LIST_SIZE     2048

#define CTX_AUDIO               0
#define CTX_CLIENTS             0
#define CTX_EVENTS              1



/* we keep the ctx implementation here, this compilation taget changes less
 * than the micropython target
 */
#define CTX_IMPLEMENTATION
#define CTX_EXTERNAL_MALLOC

static inline void *ctx_malloc (size_t size)
{
  return m_malloc (size);
}

static inline void *ctx_calloc (size_t nmemb, size_t size)
{
  size_t byte_size = nmemb * size;
  char *ret        = (char *)m_malloc(byte_size);
  for (size_t i = 0; i < byte_size; i++)
    ret[i] = 0;
  return ret;
}

static inline void *ctx_realloc (void *ptr, size_t size)
{
  return m_realloc(ptr, size);
}

static inline void ctx_free (void *ptr)
{
  return m_free(ptr);
}

#include "ctx.h"
extern char epic_exec_path[256];

#if MICROPY_ENABLE_COMPILER

extern int _mp_quit;

Ctx *ctx_wasm_get_context(int flags);

void epic_set_ctx (Ctx *ctx);
Ctx *ctx_wasm_reset (void);

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
    epic_set_ctx (ctx_wasm_get_context(CTX_CB_KEEP_DATA));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_lib));
}

void mp_js_init_repl() {
    pyexec_event_repl_init();
}


int do_path (const char *path)
{
  mp_js_init (192 * 1024);
  mp_parse_input_kind_t input_kind = MP_PARSE_FILE_INPUT;
  int ret = 0;
  nlr_buf_t nlr;
  _mp_quit = 0;

  if (nlr_push(&nlr) == 0) {
      mp_lexer_t *lex = mp_lexer_new_from_file (path);
      epic_exec_path[0] = 0;
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
          if (epic_exec_path[0])
            return do_path (epic_exec_path);
      } else {
          mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
          ret = 1;
      }
  }
  mp_js_init_repl();
  return ret;
}

int do_str(const char *src, mp_parse_input_kind_t input_kind)
{
    int ret = 0;
    mp_js_init (192 * 1024);
    nlr_buf_t nlr;
    epic_exec_path[0] = 0;
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
            if (epic_exec_path[0])
              return do_path (epic_exec_path);
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

#if 0
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}
#endif

#if 0
mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
#endif

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
