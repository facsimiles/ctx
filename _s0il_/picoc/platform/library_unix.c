#include "../interpreter.h"
#ifdef CTX
#include "ctx.h"
#include "ui.h"

#define fun_int__ptr_ptr_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP);\
}

#define fun_int__ptr_ptr_ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer,\
          Param[3]->Val->Pointer);\
}

#define fun_int__ptr_ptr_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Integer);\
}

#define fun_int__ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer);\
}

#define fun_int__ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer);\
}

#define fun_int__ptr_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Integer);\
}

#define fun_float__ptr_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->FP = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Integer);\
}

#define fun_float__ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->FP = \
  funname(Param[0]->Val->Pointer);\
}

#define fun_ptr__ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Pointer = \
  funname(Param[0]->Val->Pointer);\
}

#define fun_ptr__int_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Pointer = \
  funname(Param[0]->Val->Integer,\
          Param[1]->Val->Integer);\
}

#define fun_ptr__int_int_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Pointer = \
  funname(Param[0]->Val->Integer,\
          Param[1]->Val->Integer,\
          Param[2]->Val->Pointer);\
}

#define fun_void__ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer);\
}

#define fun_void__ptr_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Integer);\
}

#define fun_void__ptr_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP);\
}

#define fun_void__ptr_ptr_ptr_int_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer,\
          Param[3]->Val->Integer,\
          Param[4]->Val->Integer);\
}

#define fun_void__ptr_ptr_ptr_ptr_int_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer,\
          Param[3]->Val->Pointer,\
          Param[4]->Val->Integer,\
          Param[5]->Val->Integer);\
}

#define fun_void__ptr_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP);\
}

#define fun_void__ptr_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP);\
}

#define fun_void__ptr_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP);\
}

#define fun_void__ptr_ptr_ptr_ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer,\
          Param[3]->Val->Pointer,\
          Param[4]->Val->Pointer);\
}


#define fun_void__ptr_ptr_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP);\
}

#define fun_void__ptr_ptr_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP);\
}

#define fun_void__ptr_float_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP);\
}


#define fun_int__int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Integer);\
}

#define fun_float__ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->FP = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer);\
}

#define fun_float__ptr_ptr_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->FP = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP);\
}

#define fun_int__ptr_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  ReturnValue->Val->Integer = \
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP);\
}

#define fun_void__ptr_float_float_float_float_float_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP,\
          Param[6]->Val->FP,\
          Param[7]->Val->FP,\
          Param[8]->Val->FP,\
          Param[9]->Val->FP);\
}

#define fun_void__ptr_float_float_float_float_float_int(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP,\
          Param[6]->Val->Integer);\
}

#define fun_void__ptr_float_float_float_float_float_float(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->FP,\
          Param[2]->Val->FP,\
          Param[3]->Val->FP,\
          Param[4]->Val->FP,\
          Param[5]->Val->FP,\
          Param[6]->Val->FP);\
}

#define fun_void__ptr_ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer);\
}

#define fun_void__ptr_ptr_ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer,\
          Param[2]->Val->Pointer,\
          Param[3]->Val->Pointer);\
}

#define fun_void__ptr_ptr(funname) \
void C##funname (struct ParseState *Parser, struct Value *ReturnValue,\
	         struct Value **Param, int NumArgs)\
{\
  funname(Param[0]->Val->Pointer,\
          Param[1]->Val->Pointer);\
}

fun_int__ptr_ptr(runv);
fun_int__ptr_ptr(runvp);
fun_int__ptr(runs);


fun_void__ptr_ptr(ui_text);
fun_ptr__ptr(ui_new);
fun_void__ptr(ui_start_frame);
fun_void__ptr(ui_end_frame);
fun_void__ptr(ui_destroy);
fun_int__ptr_ptr(ui_button);

fun_void__ptr_ptr(ui_main);
fun_void__ptr_ptr(ui_do);
fun_void__ptr(ui_pop_fun);
fun_void__ptr(ui_draw_bg);
fun_void__ptr_float(ui_scroll_to);
fun_void__ptr_float(ui_set_scroll_offset);
fun_int__ptr_ptr_int(ui_toggle);
fun_int__ptr(ui_keyboard_visible);
fun_void__ptr_ptr(ui_title);
fun_int__ptr_ptr_ptr_ptr(ui_entry);
fun_float__ptr_ptr_float_float_float_float(ui_slider);
fun_float__ptr(ui_get_font_size);
fun_float__ptr(ui_x);
fun_float__ptr(ui_y);
fun_void__ptr_float_float(ui_move_to);
fun_void__ptr_ptr_ptr_ptr(ui_register_view);
fun_void__ptr_ptr_ptr(ui_cb_do);
fun_void__ptr(ui_iteration);
fun_ptr__ptr(ui_ctx);
fun_ptr__ptr(ui_get_data);
fun_void__ptr(ui_keyboard);
fun_void__ptr_ptr(ui_load_file);
fun_void__ptr_ptr_ptr_int_int(magic_add);

// ui_push_fun ui_register_view
// set_data get_data  find_exec  ui_basename  elf_output_state 

struct LibraryFunction UiFunctions[] =
{
    {Cui_start_frame,        "void ui_start_frame(Ui*);"},
    {Cui_end_frame,          "void ui_end_frame(Ui*);"},
    {Cui_text,         "void ui_text(Ui*, char*str);"},
    {Cui_title,        "void ui_title(Ui*, char*str);"},
    {Cui_button,       "int ui_button(Ui*, char*string);"},
    {Cui_new,          "Ui *ui_new(Ctx*);"},
    {Cui_ctx,          "Ctx *ui_ctx(Ui*);"},
    {Cui_get_data,     "void *ui_get_data(Ui*);"},
    {Cui_load_file,    "void ui_load_file(Ui *,char*p);"},
    {Cmagic_add, "void magic_add(char*,char*,unsigned char*,int, int);"},
    {Cui_keyboard,     "void ui_keyboard(Ui *);"},
    {Cui_destroy,      "void ui_destroy(Ui *);"},
    {Cui_main,         "void ui_main(Ui *, char*);"},
    {Cui_do,           "void ui_do(Ui *, char*);"},
    {Cui_pop_fun,      "void ui_pop_fun(Ui *);"},
    {Cui_draw_bg,      "void ui_draw_bg(Ui *);"},
    {Cui_scroll_to,    "void ui_pop_scroll_to(Ui *,float);"},
    {Cui_set_scroll_offset,    "void ui_pop_set_scroll_offset(Ui *,float);"},
    {Cui_toggle,       "int ui_toggle(Ui *, char*, int);"},
    {Cui_entry,        "int ui_entry(Ui*, char*,char*,char**strptr);"},
    {Cui_slider,       "float ui_slider(Ui*, char*,float,float,float,float);"},
    {Cui_get_font_size,  "float ui_get_font_size(Ui*);"},
    {Cui_x,  "float ui_x(Ui*);"},
    {Cui_y,  "float ui_y(Ui*);"},
    {Cui_move_to,  "void ui_move_to(Ui*,float,float);"},
    {Cui_register_view,  "void ui_register_view(Ui*,char*,void*,char*);"},
    {Cui_cb_do,  "void ui_cb_do(void*,void*,void*);"},
    {Cui_iteration,  "void ui_iteration(Ui*);"},

    {Crunv,            "int runv(char *, char **);"},
    {Crunvp,           "int runvp(char *, char **);"},
    {Cruns,            "int runs(char *);"},

 
    {NULL, NULL}
};


fun_ptr__int_int_ptr(ctx_new);
fun_int__ptr(ctx_has_exited);
fun_void__ptr(ctx_destroy);
fun_void__ptr(ctx_start_frame);
fun_void__ptr(ctx_end_frame);
fun_void__ptr(ctx_clip);
fun_void__ptr(ctx_fill);
fun_void__ptr(ctx_stroke);
fun_void__ptr(ctx_paint);
fun_void__ptr(ctx_preserve);
fun_void__ptr(ctx_start_group);
fun_void__ptr(ctx_end_group);
fun_void__ptr(ctx_save);
fun_void__ptr(ctx_restore);
fun_void__ptr(ctx_queue_draw);
fun_void__ptr(ctx_exit);
fun_void__ptr(ctx_new_page);
fun_void__ptr(ctx_begin_path);
fun_void__ptr(ctx_stroke_source);
fun_void__ptr(ctx_close_path);
fun_void__ptr_float_float_float_float(ctx_rgba);
fun_void__ptr_float_float_float(ctx_rgb);
fun_void__ptr_float(ctx_gray);
fun_void__ptr_float(ctx_line_height);
fun_void__ptr_float(ctx_global_alpha);
fun_void__ptr_float(ctx_wrap_left);
fun_void__ptr_float(ctx_wrap_right);
fun_void__ptr_float(ctx_rotate);
fun_void__ptr_float(ctx_line_width);
fun_void__ptr_float(ctx_line_dash_offset);
fun_void__ptr_float(ctx_font_size);
fun_void__ptr_float(ctx_miter_limit);
fun_void__ptr_int(ctx_image_smoothing);

fun_void__ptr_float_float(ctx_scale);
fun_void__ptr_float_float(ctx_translate);
fun_void__ptr_float_float(ctx_move_to);
fun_void__ptr_float_float(ctx_line_to);
fun_void__ptr_float_float(ctx_rel_move_to);
fun_void__ptr_float_float(ctx_rel_line_to);

fun_void__ptr_ptr(ctx_font);
fun_void__ptr_ptr(ctx_parse);
fun_void__ptr_ptr(ctx_font_family);
// XXX line_dash font_extents
fun_void__ptr_float_float_float_float_float_float_float_float_float(ctx_apply_transform);
fun_void__ptr_float_float_float_float_float_float_float_float_float(ctx_set_transform);
fun_int__ptr_float_float(ctx_in_fill);
fun_int__ptr_float_float(ctx_in_stroke);
fun_void__ptr_float_float_float_float(ctx_quad_to);
fun_void__ptr_float_float_float_float_float_float(ctx_curve_to);
fun_void__ptr_float_float_float_float(ctx_rel_quad_to);
fun_void__ptr_float_float_float_float_float_float(ctx_rel_curve_to);
fun_void__ptr_float_float_float_float_float_int(ctx_arc);
fun_void__ptr_float_float_float_float_float(ctx_arc_to);
fun_void__ptr_float_float_float_float_float(ctx_rel_arc_to);
fun_void__ptr_float_float_float_float(ctx_rectangle);
fun_void__ptr_float_float_float_float(ctx_linear_gradient);
fun_void__ptr_float_float_float_float_float_float(ctx_radial_gradient);
fun_void__ptr_float_float_float_float_float(ctx_gradient_add_stop);
fun_void__ptr_float_float_float_float_float(ctx_round_rectangle);

fun_float__ptr_ptr(ctx_text_width);
fun_void__ptr_ptr(ctx_text);
fun_int__ptr_int(ctx_pointer_is_down);
fun_float__ptr(ctx_pointer_x);
fun_float__ptr(ctx_pointer_y);
fun_float__ptr(ctx_x);
fun_float__ptr(ctx_y);
fun_float__ptr(ctx_get_global_alpha);
fun_float__ptr(ctx_get_font_size);
fun_float__ptr(ctx_get_miter_limit);
fun_float__ptr(ctx_get_image_smoothing);
fun_float__ptr(ctx_get_line_dash_offset);
fun_float__ptr(ctx_get_wrap_left);
fun_float__ptr(ctx_get_wrap_right);
fun_float__ptr(ctx_get_line_height);

fun_int__ptr(ctx_need_redraw);
fun_int__ptr(ctx_get_blend_mode);
fun_int__ptr(ctx_get_extend);
fun_int__ptr(ctx_get_text_align);
fun_int__ptr(ctx_get_text_baseline);
//fun_int__ptr(ctx_get_text_direction);
fun_int__ptr(ctx_get_fill_rule);
fun_int__ptr(ctx_get_line_cap);
fun_int__ptr(ctx_get_line_join);
fun_int__ptr(ctx_width);
fun_int__ptr(ctx_height);


fun_void__ptr_int(ctx_blend_mode);
fun_void__ptr_int(ctx_extend);
fun_void__ptr_int(ctx_text_align);
fun_void__ptr_int(ctx_text_baseline);
fun_void__ptr_int(ctx_text_direction);
fun_void__ptr_int(ctx_fill_rule);
fun_void__ptr_int(ctx_line_cap);
fun_void__ptr_int(ctx_line_join);

fun_int__ptr(ctx_utf8_strlen);
fun_int__int(ctx_utf8_len);


// define_texture source_transform current_point get_transform clip_extentsA
// new_for_framebuffer get_image_data put_image_Data
// load_font_ttf dirty_rect

fun_ptr__int_int(ctx_new_drawlist);
fun_void__ptr(ctx_drawlist_clear);
fun_int__ptr_ptr_int(ctx_set_drawlist);
fun_int__ptr_ptr_int(ctx_append_drawlist);

fun_void__ptr_ptr_ptr_ptr_ptr(ctx_texture_load);
fun_void__ptr_ptr_float_float(ctx_texture);
fun_void__ptr_ptr_float_float_float_float(ctx_draw_texture);
fun_void__ptr_ptr_float_float_float_float(ctx_draw_image);




/* list of all library functions and their prototypes */
struct LibraryFunction CtxFunctions[] =
{
    {Cctx_new,         "void *ctx_new(int, int, char *);"},
    {Cctx_has_exited,    "int ctx_has_exited(Ctx*);"},
    {Cctx_start_frame, "void ctx_start_frame(Ctx*);"},
    {Cctx_end_frame,   "void ctx_end_frame(Ctx*);"},
    {Cctx_destroy,     "void ctx_destroy(Ctx*);"},
    {Cctx_clip,      "void ctx_clip(Ctx*);"},
    {Cctx_fill,      "void ctx_fill(Ctx*);"},
    {Cctx_stroke,    "void ctx_stroke(Ctx*);"},
    {Cctx_paint,     "void ctx_paint(Ctx*);"},
    {Cctx_preserve,     "void ctx_preserve(Ctx*);"},
    {Cctx_start_group,   "void ctx_start_group(Ctx*);"},
    {Cctx_end_group,     "void ctx_end_group(Ctx*);"},
    {Cctx_save,     "void ctx_save(Ctx*);"},
    {Cctx_new_page,    "void ctx_new_page(Ctx*);"},
    {Cctx_restore,     "void ctx_restore(Ctx*);"},
    {Cctx_queue_draw,  "void ctx_queue_draw(Ctx*);"},
    {Cctx_exit,     "void ctx_exit(Ctx*);"},
    {Cctx_begin_path,     "void ctx_begin_path(Ctx*);"},
    {Cctx_stroke_source,  "void ctx_stroke_source(Ctx*);"},
    {Cctx_close_path,     "void ctx_close_path(Ctx*);"},
    {Cctx_gray,    "void ctx_gray(Ctx*,float);"},
    {Cctx_rgb,     "void ctx_rgb(Ctx*,float,float,float);"},
    {Cctx_rgba,    "void ctx_rgba(Ctx*,float,float,float,float);"},
    {Cctx_line_height,     "void ctx_line_height(Ctx*,float);"},
    {Cctx_global_alpha,     "void ctx_global_alpha(Ctx*,float);"},
    {Cctx_wrap_left,     "void ctx_wrap_left(Ctx*,float);"},
    {Cctx_wrap_right,     "void ctx_wrap_right(Ctx*,float);"},
    {Cctx_rotate,     "void ctx_rotate(Ctx*,float);"},
    {Cctx_line_width, "void ctx_line_width(Ctx*,float);"},
    {Cctx_line_dash_offset,     "void ctx_line_dash_offset(Ctx*,float);"},
    {Cctx_font_size,     "void ctx_font_size(Ctx*,float);"},
    {Cctx_miter_limit,   "void ctx_miter_limit(Ctx*,float);"},
    {Cctx_image_smoothing,     "void ctx_image_smoothing(Ctx*,int);"},
    {Cctx_scale,       "void ctx_scale(Ctx*,float,float);"},
    {Cctx_translate,   "void ctx_translate(Ctx*,float,float);"},
    {Cctx_move_to,     "void ctx_move_to(Ctx*,float,float);"},
    {Cctx_line_to,     "void ctx_line_to(Ctx*,float,float);"},
    {Cctx_rel_move_to, "void ctx_rel_move_to(Ctx*,float,float);"},
    {Cctx_rel_line_to, "void ctx_rel_line_to(Ctx*,float,float);"},
    {Cctx_font, "void ctx_font(Ctx*,char *);"},
    {Cctx_font_family, "void ctx_font_family(Ctx*,char *);"},
    {Cctx_parse, "void ctx_parse(Ctx*,char *);"},
    {Cctx_apply_transform, "void ctx_apply_transform(Ctx*,float,float,float,float,float,float,float,float,float);"},
    {Cctx_set_transform, "void ctx_set_transform(Ctx*,float,float,float,float,float,float,float,float,float);"},
    {Cctx_in_fill, "int ctx_in_fill(Ctx*,float,float);"},
    {Cctx_in_stroke, "int ctx_in_stroke(Ctx*,float,float);"},
    {Cctx_quad_to, "void ctx_quad_to(Ctx*,float,float,float,float);"},
    {Cctx_rel_quad_to, "void ctx_rel_quad_to(Ctx*,float,float,float,float);"},
    {Cctx_rectangle, "void ctx_rectangle(Ctx*,float,float,float,float);"},
    {Cctx_linear_gradient, "void ctx_linear_gradient(Ctx*,float,float,float,float);"},
    {Cctx_curve_to, "void ctx_curve_to(Ctx*,float,float,float,float,float,float);"},
    {Cctx_rel_curve_to, "void ctx_rel_curve_to(Ctx*,float,float,float,float,float,float);"},
    {Cctx_radial_gradient, "void ctx_radial_gradient(Ctx*,float,float,float,float,float,float);"},
    {Cctx_round_rectangle, "void ctx_round_rectangle(Ctx*,float,float,float,float,float);"},
    {Cctx_gradient_add_stop, "void ctx_gradient_add_top(Ctx*,float,float,float,float,float);"},
    {Cctx_arc_to, "void ctx_arc_to(Ctx*,float,float,float,float,float);"},
    {Cctx_arc, "void ctx_arc(Ctx*,float,float,float,float,float,int);"},
    {Cctx_rel_arc_to, "void ctx_rel_arc_to(Ctx*,float,float,float,float,float);"},
    {Cctx_blend_mode, "void ctx_blend_mode(Ctx*,int);"},
    {Cctx_extend, "void ctx_extend(Ctx*,int);"},
    {Cctx_text_align, "void ctx_text_align(Ctx*,int);"},
    {Cctx_text_baseline, "void ctx_text_baseline(Ctx*,int);"},
    {Cctx_text_direction, "void ctx_text_direction(Ctx*,int);"},
    {Cctx_fill_rule, "void ctx_fill_rule(Ctx*,int);"},
    {Cctx_line_cap, "void ctx_line_cap(Ctx*,int);"},
    {Cctx_line_join, "void ctx_line_join(Ctx*,int);"},

    {Cctx_text, "void ctx_text(Ctx*,char*);"},
    {Cctx_text_width, "float ctx_text_width(Ctx*,char*);"},
    {Cctx_pointer_is_down, "int ctx_pointer_is_down(Ctx*,int);"},
    {Cctx_pointer_x, "float ctx_pointer_x(Ctx*);"},
    {Cctx_pointer_y, "float ctx_pointer_y(Ctx*);"},
    {Cctx_x, "float ctx_x(Ctx*);"},
    {Cctx_y, "float ctx_y(Ctx*);"},
    {Cctx_get_global_alpha, "float ctx_get_global_alpha(Ctx*);"},
    {Cctx_get_font_size, "float ctx_get_font_size(Ctx*);"},
    {Cctx_get_miter_limit, "float ctx_get_miter_limit(Ctx*);"},
    {Cctx_get_image_smoothing, "float ctx_get_image_smoothing(Ctx*);"},
    {Cctx_get_line_dash_offset, "float ctx_get_line_dash_offset(Ctx*);"},
    {Cctx_get_wrap_left, "float ctx_get_wrap_left(Ctx*);"},
    {Cctx_get_wrap_right, "float ctx_get_wrap_right(Ctx*);"},
    {Cctx_get_line_height, "float ctx_get_line_height(Ctx*);"},

    {Cctx_need_redraw, "int ctx_need_redraw(Ctx*);"},
    {Cctx_get_blend_mode, "int ctx_get_blend_mode(Ctx*);"},
    {Cctx_get_extend, "int ctx_get_extend(Ctx*);"},
    {Cctx_get_text_align, "int ctx_get_text_align(Ctx*);"},
    {Cctx_get_text_baseline, "int ctx_get_text_baseline(Ctx*);"},
    {Cctx_get_fill_rule, "int ctx_get_fill_rule(Ctx*);"},
    {Cctx_get_line_cap, "int ctx_get_line_cap(Ctx*);"},
    {Cctx_get_line_join, "int ctx_get_line_join(Ctx*);"},
    {Cctx_width, "int ctx_width(Ctx*);"},
    {Cctx_height, "int ctx_height(Ctx*);"},

    {Cctx_draw_image, "void ctx_draw_image(Ctx*,char*,float,float,float,float);"},
    {Cctx_draw_texture, "void ctx_draw_texture(Ctx*,char*,float,float,float,float);"},
    {Cctx_utf8_strlen, "int ctx_utf8_strlen(char*);"},
    {Cctx_utf8_len, "int ctx_utf8_len(int);"},
    {Cctx_new_drawlist, "Ctx *ctx_new_drawlist(int, int);"},
    {Cctx_drawlist_clear, "void ctx_drawlist_clear(Ctx*);"},
    {Cctx_set_drawlist, "int ctx_set_drawlist(Ctx*, void*,void*,int);"},
    {Cctx_append_drawlist, "int ctx_append_drawlist(Ctx*, void*,void*,int);"},
    {Cctx_texture, "void ctx_texture(Ctx*, void*,float,float);"},
    {Cctx_texture_load, "void ctx_texture_load(Ctx*, void*,void*,void*,void*);"},


    {NULL, NULL}
};
#endif
void PlatformLibraryInit(Picoc *pc)
{
#ifdef CTX
    IncludeRegister(pc, "ctx.h", NULL, &CtxFunctions[0],
    "#include <stdio.h>\n"
    "typedef struct _Ctx Ctx;\n"
    "#ifndef NULL\n"
    "#define NULL ((void*)0)\n"
    "#endif\n");

    IncludeRegister(pc, "ui.h", NULL, &UiFunctions[0],

    "#include <stdio.h>\n"
    "#include <string.h>\n"
    "#include <stdlib.h>\n"
    "#include <stdbool.h>\n"
    "#include <unistd.h>\n"
    "#include <math.h>\n"
    "#include <ctx.h>\n"
    "typedef struct _Ui Ui;\n"
    "#ifndef NULL\n"
    "#define NULL ((void*)0)\n"
    "#endif\n");
#endif
}
