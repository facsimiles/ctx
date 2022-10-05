/* This file is not licensed (c) Øyvind Kolås pippin@gimp.org,
 * based on micro-raptor GUI
  */

#if CTX_EVENTS

#define CTX_MAX_STYLE_DEPTH  CTX_MAX_STATES
#define CTX_MAX_STATE_DEPTH  CTX_MAX_STATES
#define CTX_MAX_FLOATS           16
#define CTX_MAX_SELECTOR_LENGTH  64
#define CTX_MAX_CSS_STRINGLEN    512
#define CTX_MAX_CSS_RULELEN      32   // XXX doesnt have overflow protection
#define CTX_MAX_CSS_RULES        128

/* other important maximums */
#define CTX_MAX_TEXT_LISTEN      256
#define CTX_XML_INBUF_SIZE       1024

#define PROP(a)          (ctx_get_float(mrg->ctx, SQZ_##a))
#define PROPS(a)         (ctx_get_string(mrg->ctx, SQZ_##a))
#define SET_PROPh(a,v)   (ctx_set_float(mrg->ctx, a, v))
#define SET_PROP(a,v)    SET_PROPh(SQZ_##a, v)
#define SET_PROPS(a,v)   (ctx_set_string(mrg->ctx, SQZ_##a, v))
#define SET_PROPSh(a,v)  (ctx_set_string(mrg->ctx, a, v))

#define SQZ_1      374u
#define SQZ_Aelig  2540544426u
#define SQZ_AElig  2622343083u
#define SQZ_Aring  3473872814u
#define SQZ_Oslash 3911734189u

/*
 *  extra hashed strings to be picked up
 *
 *  SQZ_id  SQZ_class  SQZ_d SQZ_rel    SQZ_viewbox
 *
 */

#include "w3c-constants.h"


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <math.h>

typedef struct _Mrg          Mrg;


void mrg_clear (Mrg *mrg);

static void mrg_queue_draw (Mrg *mrg, CtxIntRectangle *rect)
{
}

typedef struct _CtxStyleNode CtxStyleNode;

#define CTX_STYLE_MAX_CLASSES 8
#define CTX_STYLE_MAX_PSEUDO  8

struct _CtxStyleNode
{
  uint32_t    element_hash;
  uint32_t    classes_hash[CTX_STYLE_MAX_CLASSES];
  uint32_t    id_hash;
  const char *pseudo[CTX_STYLE_MAX_PSEUDO];
  uint32_t    pseudo_hash[CTX_STYLE_MAX_PSEUDO];
              // TODO : to hash pseudos we need to store
              //   argument (like for nth-child)
  int         direct_descendant; /* for use in selector chains with > */
  const char *id;
};
 
typedef enum {
  CTX_FLOAT_NONE = 0,
  CTX_FLOAT_LEFT,
  CTX_FLOAT_RIGHT,
  CTX_FLOAT_FIXED
} CtxFloat;


typedef struct CtxFloatData {
  CtxFloat  type;
  float     x;
  float     y;
  float     width;
  float     height;
} CtxFloatData;

typedef void (*MrgNewText)       (const char *new_text, void *data);
typedef void (*UiRenderFun)      (Mrg *mrg, void *ui_data);

typedef struct _Mrg Mrg;

typedef enum {
  CTX_DISPLAY_INLINE = 0,
  CTX_DISPLAY_BLOCK,
  CTX_DISPLAY_LIST_ITEM,
  CTX_DISPLAY_NONE,
  CTX_DISPLAY_INLINE_BLOCK,
  CTX_DISPLAY_FLOW_ROOT,
  CTX_DISPLAY_FLEX,
  CTX_DISPLAY_GRID,
  CTX_DISPLAY_INLINE_FLEX,
  CTX_DISPLAY_INLINE_GRID,
  CTX_DISPLAY_INLINE_TABLE,
  CTX_DISPLAY_RUN_IN,
  CTX_DISPLAY_TABLE,
  CTX_DISPLAY_TABLE_CAPTION,
  CTX_DISPLAY_TABLE_COLUMN_GROUP,
  CTX_DISPLAY_TABLE_HEADER_GROUP,
  CTX_DISPLAY_TABLE_FOOTER_GROUP,
  CTX_DISPLAY_TABLE_ROW_GROUP,
  CTX_DISPLAY_TABLE_CELL,
  CTX_DISPLAY_TABLE_COLUMN
} CtxDisplay;

/* matches cairo order */
typedef enum
{
  CTX_FONT_WEIGHT_NORMAL = 0,
  CTX_FONT_WEIGHT_BOLD
} CtxFontWeight;

/* matches cairo order */
typedef enum
{
  CTX_FONT_STYLE_NORMAL = 0,
  CTX_FONT_STYLE_ITALIC,
  CTX_FONT_STYLE_OBLIQUE
} CtxFontStyle;

typedef enum
{
  CTX_BOX_SIZING_CONTENT_BOX = 0,
  CTX_BOX_SIZING_BORDER_BOX
} CtxBoxSizing;

/* matching nchanterm definitions */

typedef enum {
  CTX_TEXT_DECORATION_REGULAR     = 0,
  CTX_TEXT_DECORATION_BOLD        = (1 << 0),
  CTX_TEXT_DECORATION_DIM         = (1 << 1),
  CTX_TEXT_DECORATION_UNDERLINE   = (1 << 2),
  CTX_TEXT_DECORATION_REVERSE     = (1 << 3),
  CTX_TEXT_DECORATION_OVERLINE    = (1 << 4),
  CTX_TEXT_DECORATION_LINETHROUGH = (1 << 5),
  CTX_TEXT_DECORATION_BLINK       = (1 << 6)
} CtxTextDecoration;

typedef enum {
  CTX_POSITION_STATIC = 0,
  CTX_POSITION_RELATIVE,
  CTX_POSITION_FIXED,
  CTX_POSITION_ABSOLUTE
} CtxPosition;

typedef enum {
  CTX_OVERFLOW_VISIBLE = 0,
  CTX_OVERFLOW_HIDDEN,
  CTX_OVERFLOW_SCROLL,
  CTX_OVERFLOW_AUTO
} CtxOverflow;

typedef enum {
  CTX_CLEAR_NONE = 0,
  CTX_CLEAR_LEFT,  // 1
  CTX_CLEAR_RIGHT, // 2
  CTX_CLEAR_BOTH   // 3   ( LEFT | RIGHT )
} CtxClear;


typedef enum {
  CTX_WHITE_SPACE_NORMAL = 0,
  CTX_WHITE_SPACE_NOWRAP,
  CTX_WHITE_SPACE_PRE,
  CTX_WHITE_SPACE_PRE_LINE,
  CTX_WHITE_SPACE_PRE_WRAP
} CtxWhiteSpace;

typedef enum {
  CTX_VERTICAL_ALIGN_BASELINE = 0,
  CTX_VERTICAL_ALIGN_MIDDLE,
  CTX_VERTICAL_ALIGN_BOTTOM,
  CTX_VERTICAL_ALIGN_TOP,
  CTX_VERTICAL_ALIGN_SUB,
  CTX_VERTICAL_ALIGN_SUPER
} CtxVerticalAlign;

typedef enum {
  MRG_CURSOR_AUTO = 0,
  MRG_CURSOR_ALIAS,
  MRG_CURSOR_ALL_SCROLL,
  MRG_CURSOR_CELL,
  MRG_CURSOR_CONTEXT_MENU,
  MRG_CURSOR_COL_RESIZE,
  MRG_CURSOR_COPY,
  MRG_CURSOR_CROSSHAIR,
  MRG_CURSOR_DEFAULT,
  MRG_CURSOR_E_RESIZE,
  MRG_CURSOR_EW_RESIZE,
  MRG_CURSOR_HELP,
  MRG_CURSOR_MOVE,
  MRG_CURSOR_N_RESIZE,
  MRG_CURSOR_NE_RESIZE,
  MRG_CURSOR_NESW_RESIZE,
  MRG_CURSOR_NS_RESIZE,
  MRG_CURSOR_NW_RESIZE,
  MRG_CURSOR_NO_DROP,
  MRG_CURSOR_NONE,
  MRG_CURSOR_NOT_ALLOWED,
  MRG_CURSOR_POINTER,
  MRG_CURSOR_PROGRESS,
  MRG_CURSOR_ROW_RESIZE,
  MRG_CURSOR_S_RESIZE,
  MRG_CURSOR_SE_RESIZE,
  MRG_CURSOR_SW_RESIZE,
  MRG_CURSOR_TEXT,
  MRG_CURSOR_VERTICAL_TEXT,
  MRG_CURSOR_W_RESIZE,
  MRG_CURSOR_WAIT,
  MRG_CURSOR_ZOOM_IN,
  MRG_CURSOR_ZOOM_OUT
} MrgCursor;


typedef enum {
  CTX_UNICODE_BIDI_NORMAL = 0,
  CTX_UNICODE_BIDI_EMBED,
  CTX_UNICODE_BIDI_BIDI_OVERRIDE
} CtxUnicodeBidi;

typedef enum {
  CTX_VISIBILITY_VISIBLE = 0,
  CTX_VISIBILITY_HIDDEN
} CtxVisibility;

typedef enum {
  CTX_LIST_STYLE_OUTSIDE = 0,
  CTX_LIST_STYLE_INSIDE
} CtxListStyle;

/* This style class should be able to grow to contain some color names with
 * semantic meaning.
 */
struct _CtxStyle {
  /* text-related, we could potentially store *all* variables in keydb
   * that would both be slower and more bloatful than tightly packed bits,
   * some things currently in keydb should maybe be moved out for
   * performance.
   */
  float               font_size; // used for mrg_em() should be direct
  float               line_height;
  CtxVisibility       visibility:1;
  CtxFillRule         fill_rule:1;
  CtxFontStyle        font_style:3;
  CtxFontWeight       font_weight:4;
  CtxLineCap          stroke_linecap:2;
  CtxLineJoin         stroke_linejoin:2;
  CtxTextAlign        text_align:3;
  CtxPosition         position:2;
  CtxBoxSizing        box_sizing:1;
  CtxVerticalAlign    vertical_align:3;
  CtxWhiteSpace       white_space:3;
  CtxUnicodeBidi      unicode_bidi:2;
  CtxTextDirection    direction:2;
  CtxListStyle        list_style:1;
  CtxClear            clear:2;
  unsigned char       fill:1;
  MrgCursor           cursor:6;
  CtxTextDecoration   text_decoration:7;
  unsigned char       width_auto:1;
  unsigned char       margin_left_auto:1;
  unsigned char       margin_right_auto:1;
  unsigned char       print_symbols:1;
  CtxFloat            float_:2;
  unsigned char       stroke:1;
  CtxOverflow         overflow:2;
  CtxDisplay          display:5;
  void               *id_ptr;
  int                 z_index;
};

typedef struct _CtxStyle CtxStyle;

typedef struct MrgState {
  CtxStyleNode style_node;

  float        original_x;
  float        original_y;
  float        block_start_x;
  float        block_start_y;
  float        ptly;
  float        vmarg;
  int          flow_root; // is
  int          float_base;

  float      (*wrap_edge_left)  (Mrg *mrg, void *data);
  float      (*wrap_edge_right) (Mrg *mrg, void *data);
  void        *wrap_edge_data;
  float        edge_top;
  float        edge_left;
  float        edge_right;
  float        edge_bottom;

  int          skip_lines;  /* better with an em offset? */
  int          max_lines;   /* better with max-y in ems? ? */

  char        *style_id;
  CtxStyle     style;

  int          children;
  int          overflowed:1;
  int          span_bg_started:1;

  int          drawlist_start_offset;
} MrgState;

typedef struct _MrgAbsolute MrgAbsolute;

struct _MrgAbsolute {
  int       z_index;
  int       fixed;
  float     top;
  float     left;
  float     relative_x;
  float     relative_y;
  CtxEntry *entries;
  int       count; 
};


struct _Mrg {
  Ctx             *ctx;
  Ctx            *document_ctx;
  Ctx            *fixed_ctx;
  Ctx            *absolute_ctx;
  float            rem;
  float            ddpx;
  CtxList         *absolutes;
  CtxList         *stylesheet;
  void            *css_parse_state;
  CtxString       *style;
  CtxString       *style_global;
  int              quit;
  float            x; /* in px */
  float            y; /* in px */
  float            relative_x;
  float            relative_y;
  CtxIntRectangle     dirty;
  CtxIntRectangle     dirty_during_paint; // queued during painting
  MrgState        *state;
  MrgState         states[CTX_MAX_STATE_DEPTH];
  int              state_no;
  void            *backend_data;
  int              do_clip;
  int (*mrg_get_contents) (const char  *referer,
                           const char  *input_uri,
                           char       **contents,
                           long        *length,
                           void        *get_contents_data);
  void *get_contents_data;

  CtxFloatData float_data[CTX_MAX_FLOATS];
  int          floats;

    /** text editing state follows **/
  int              text_edited;
  int              got_edit;
  CtxString       *edited_str;
  char           **edited;

  int              text_edit_blocked;
  MrgNewText       update_string;
  void            *update_string_user_data;

  CtxDestroyNotify update_string_destroy_notify;
  void            *update_string_destroy_data;

  int              cursor_pos;
  float            e_x;
  float            e_y;
  float            e_ws;
  float            e_we;
  float            e_em;

  CtxEventType     text_listen_types[CTX_MAX_TEXT_LISTEN];
  CtxCb            text_listen_cb[CTX_MAX_TEXT_LISTEN];
  void            *text_listen_data1[CTX_MAX_TEXT_LISTEN];
  void            *text_listen_data2[CTX_MAX_TEXT_LISTEN];

  void     (*text_listen_finalize[CTX_MAX_TEXT_LISTEN])(void *listen_data, void *listen_data2, void *finalize_data);
  void      *text_listen_finalize_data[CTX_MAX_TEXT_LISTEN];
  int        text_listen_count;
  int        text_listen_active;

  int        line_level; // nesting level for active-line 
			 //
  float      line_max_height[CTX_MAX_STATES];
  int        line_got_baseline[CTX_MAX_STATES]; // whether the current mrg position of
						// baseline is correctly configured
						// for relative drawing.
						//
						// XXX refactor into a bitfield
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  //////////////////// end of css ////////////////

  // the following used to be the original ITK struct

  int (*ui_fun)(ITK *itk, void *data);
  void *ui_data;

  // the following should be removed in favor
  // of the css|mrg data?
  float edge_left;
  float edge_top;
  float edge_right;
  float edge_bottom;
  float width;
  float height;

  float font_size;
  float rel_hmargin;
  float rel_vmargin;
  float label_width;

  float scale;

  float rel_ver_advance;
  float rel_hpad;
  float rel_vgap;
  float scroll_speed;

  int   return_value; // when set to 1, we return the internally held from the
                      // defining app state when the widget was drawn/intercations
                      // started.

  float slider_value; // for reporting back slider value

  int   active;  // 0 not actively editing
                 // 1 currently in edit-mode of focused widget
                 // 2 means return edited value

  int   active_entry;
  int   focus_wraparound;

  int   focus_no;
  int   focus_x;
  int   focus_y;
  int   focus_width;
  char *focus_label;

  char *entry_copy;
  int   entry_pos;
  ITKPanel *panel;
  char *uri_base;

  CtxList *old_controls;
  CtxList *controls;
  CtxList *choices;
  CtxList *panels;
  int hovered_no;
  int control_no;
  int choice_active;

  int choice_no;  // the currenlt active choice if the choice context is visible (or the current control is a choice)

  int popup_x;
  int popup_y;
  int popup_width;
  int popup_height;

  char *active_menu_path;
  char *menu_path;

  uint64_t next_flags;
  void    *next_id; // to pre-empt a control and get it a more unique
                 // identifier than the numeric pos
  int   line_no;
  int   lines_drawn;
  int   light_mode;


////////////////////////////////

  int   in_choices;

  int   unresolved_line;

};

float itk_panel_scroll (ITK *itk);

static Ctx *mrg_ctx (Mrg *mrg)
{
  return mrg->ctx;
}


/* XXX: stopping sibling grabs should be an addtion to stop propagation,
 * this would permit multiple events to co-register, and use that
 * to signal each other,.. or perhaps more coordination is needed
 */
void _mrg_clear_text_closures (Mrg *mrg)
{
  int i;
  for (i = 0; i < mrg->text_listen_count; i ++)
  {
    if (mrg->text_listen_finalize[i])
       mrg->text_listen_finalize[i](
         mrg->text_listen_data1[i],
         mrg->text_listen_data2[i],
         mrg->text_listen_finalize_data[i]);
  }
  mrg->text_listen_count  = 0;
  mrg->text_listen_active = 0;
}

static CtxList *interns = NULL;

const char * mrg_intern_string (const char *str)
{
  CtxList *i;
  for (i = interns; i; i = i->next)
  {
    if (!strcmp (i->data, str))
      return i->data;
  }
  str = strdup (str);
  ctx_list_append (&interns, (void*)str);
  return str;
}

int
mrg_get_contents (Mrg         *mrg,
                  const char  *referer,
                  const char  *input_uri,
                  char       **contents,
                  long        *length);

/*
 * Copyright (c) 2002, 2003, Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#if 0
void _ctX_bindings_key_down (CtxEvent *event, void *data1, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxEvents *mrg = &ctx->events;
  int i;
  int handled = 0;

  for (i = mrg->n_bindings-1; i>=0; i--)
    if (!strcmp (mrg->bindings[i].nick, event->string))
    {
      if (mrg->bindings[i].cb)
      {
        mrg->bindings[i].cb (event, mrg->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
        handled = 1;
      }
    }
  if (!handled)
  for (i = mrg->n_bindings-1; i>=0; i--)
    if (!strcmp (mrg->bindings[i].nick, "any"))
    {
      if (mrg->bindings[i].cb)
      {
        mrg->bindings[i].cb (event, mrg->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
      }
    }
}
#endif

#ifndef XMLTOK_H
#define XMLTOK_H

#include <stdio.h>

typedef struct _Mrg    Mrg;
typedef struct _MrgXml MrgXml;

enum
{
  t_none = 0,
  t_whitespace,
  t_prolog,
  t_dtd,
  t_comment,
  t_word,
  t_tag,
  t_closetag,
  t_closeemptytag,
  t_endtag,
  t_att = 10,
  t_val,
  t_eof,
  t_entity,
  t_error
};

MrgXml *xmltok_new     (FILE *file_in);
MrgXml *xmltok_buf_new (char *membuf);
void    xmltok_free    (MrgXml *t);
int     xmltok_lineno  (MrgXml *t);
int     xmltok_get     (MrgXml *t, char **data, int *pos);

#endif /*XMLTOK_H */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _MrgXml
{
  FILE      *file_in;
  int        state;
  CtxString *curdata;
  CtxString *curtag;
  int        c;
  int        c_held;

  unsigned char *inbuf;
  int            inbuflen;
  int            inbufpos;
  int            line_no;
};

enum
{
  s_null = 0,
  s_start,
  s_tag,
  s_tagnamestart,
  s_tagname,
  s_tagnamedone,
  s_intag,
  s_attstart,
  s_attname,
  s_attdone,
  s_att,
  s_atteq,
  s_eqquot,
  s_eqvalstart,
  s_eqapos,
  s_eqaposval,
  s_eqaposvaldone,
  s_eqval,
  s_eqvaldone,
  s_eqquotval,
  s_eqquotvaldone,
  s_tagend,
  s_empty,
  s_inempty,
  s_emptyend,
  s_whitespace,
  s_whitespacedone,
  s_entitystart,
  s_entity,
  s_entitydone,
  s_word,
  s_worddone,
  s_tagclose,
  s_tagclosenamestart,
  s_tagclosename,
  s_tagclosedone,
  s_tagexcl,
  s_commentdash1,
  s_commentdash2,
  s_incomment,
  s_commentenddash1,
  s_commentenddash2,
  s_commentdone,
  s_dtd,
  s_prolog,
  s_prologq,
  s_prologdone,
  s_eof,
  s_error
};

char     *c_ws = " \n\r\t";

enum
{
  c_nil = 0,
  c_eat = 1,                    /* request that another char be used for the next state */
  c_store = 2                   /* store the current char in the output buffer */
};


typedef struct
{
  int           state;
  char         *chars;
  unsigned char r_start;
  unsigned char r_end;
  int           next_state;
  int           resetbuf;
  int           charhandling;
  int           return_type;        /* if set return current buf, with type set to the type */
}
state_entry;

#define max_entries 20

static state_entry state_table[s_error][max_entries];

static void
a (int state,
   char *chars,
   unsigned char r_start,
   unsigned char r_end, int charhandling, int next_state)
{
  int       no = 0;
  while (state_table[state][no].state != s_null)
    no++;
  state_table[state][no].state = state;
  state_table[state][no].r_start = r_start;
  if (chars)
    state_table[state][no].chars = chars;
  state_table[state][no].r_end = r_end;
  state_table[state][no].charhandling = charhandling;
  state_table[state][no].next_state = next_state;
}

static void
r (int state, int return_type, int next_state)
{
  state_table[state][0].state = state;
  state_table[state][0].return_type = return_type;
  state_table[state][0].next_state = next_state;
}

/* *INDENT-OFF* */

static void
init_statetable (void) {
    static int inited=0;
    if(inited)
        return;
    inited=1;
    memset(state_table,0,sizeof(state_table));
    a(s_start,        "<",  0,0,              c_eat,            s_tag);
    a(s_start,        c_ws, 0,0,              c_eat+c_store,    s_whitespace);
    a(s_start,        "&",  0,0,              c_eat,            s_entitystart);
    a(s_start,        NULL, 0,255,            c_eat+c_store,    s_word);
    a(s_tag,          c_ws, 0,0,              c_eat,            s_tag);
    a(s_tag,          "/",  0,0,              c_eat,            s_tagclose);
    a(s_tag,          "!",  0,0,              c_eat,            s_tagexcl);
    a(s_tag,          "?",  0,0,              c_eat,            s_prolog);
    a(s_tag,          NULL, 0,255,            c_eat+c_store,    s_tagnamestart);
    a(s_tagclose,     NULL, 0,255,         c_eat+c_store,    s_tagclosenamestart);
    a(s_tagclosenamestart,    ">",    0,0,    c_eat,            s_tagclosedone);
    a(s_tagclosenamestart,    NULL,    0,255, c_eat+c_store,    s_tagclosename);
    a(s_tagclosename,    ">",    0,0,         c_eat,            s_tagclosedone);
    a(s_tagclosename,    NULL,    0,255,      c_eat+c_store,    s_tagclosename);
    r(s_tagclosedone,    t_closetag,                            s_start);

    a(s_whitespace,        c_ws,    0,0,      c_eat+c_store,    s_whitespace);
    a(s_whitespace,        NULL,    0,255,    c_nil,            s_whitespacedone);
    r(s_whitespacedone,    t_whitespace,                        s_start);

    a(s_entitystart,";",    0,0,              c_eat,            s_entitydone);
    a(s_entitystart,NULL,    0,255,           c_eat+c_store,    s_entity);
    a(s_entity,        ";",    0,0,           c_eat,            s_entitydone);
    a(s_entity,NULL,        0,255,            c_eat+c_store,    s_entity);
    r(s_entitydone,    t_entity,                                s_start);

    a(s_word,        c_ws,    0,0,            c_nil,            s_worddone);
    a(s_word,        "<&",    0,0,            c_nil,            s_worddone);
    a(s_word,        NULL,    0,255,          c_eat+c_store,    s_word);
    r(s_worddone,    t_word,                                    s_start);

    a(s_tagnamestart,c_ws,    0,0,            c_nil,            s_tagnamedone);
    a(s_tagnamestart,    "/>",    0,0,        c_nil,            s_tagnamedone);
    a(s_tagnamestart,NULL,    0,255,          c_eat+c_store,    s_tagname);
    a(s_tagname,    c_ws,    0,0,             c_nil,            s_tagnamedone);
    a(s_tagname,    "/>",    0,0,             c_nil,            s_tagnamedone);
    a(s_tagname,    NULL,    0,255,           c_eat+c_store,    s_tagname);
    r(s_tagnamedone,    t_tag,                                s_intag);

    a(s_intag,        c_ws,    0,0,           c_eat,            s_intag);
    a(s_intag,        ">",    0,0,            c_eat,            s_tagend);
    a(s_intag,        "/",    0,0,            c_eat,            s_empty);
    a(s_intag,        NULL,    0,255,         c_eat+c_store,    s_attstart);

    a(s_attstart,    c_ws,    0,0,            c_eat,            s_attdone);
    a(s_attstart,    "=/>",    0,0,           c_nil,            s_attdone);
    a(s_attstart,    NULL,    0,255,          c_eat+c_store,    s_attname);
    a(s_attname,    "=/>",    0,0,            c_nil,            s_attdone);
    a(s_attname,    c_ws,    0,0,             c_eat,            s_attdone);
    a(s_attname,    NULL,    0,255,           c_eat+c_store,    s_attname);
    r(s_attdone,    t_att,                                    s_att);
    a(s_att,        c_ws,    0,0,             c_eat,            s_att);
    a(s_att,        "=",    0,0,              c_eat,            s_atteq);
    a(s_att,        NULL,    0,255,           c_eat,            s_intag);
    a(s_atteq,        "'",    0,0,            c_eat,            s_eqapos);
    a(s_atteq,        "\"",    0,0,           c_eat,            s_eqquot);
    a(s_atteq,        c_ws,    0,0,           c_eat,            s_atteq);
    a(s_atteq,        NULL,    0,255,         c_nil,            s_eqval);

    a(s_eqapos,        "'",    0,0,           c_eat,            s_eqaposvaldone);
    a(s_eqapos,        NULL,    0,255,        c_eat+c_store,    s_eqaposval);
    a(s_eqaposval,        "'",    0,0,        c_eat,            s_eqaposvaldone);
    a(s_eqaposval,        NULL,    0,255,     c_eat+c_store,    s_eqaposval);
    r(s_eqaposvaldone,    t_val,                                    s_intag);

    a(s_eqquot,        "\"",    0,0,          c_eat,            s_eqquotvaldone);
    a(s_eqquot,        NULL,    0,255,        c_eat+c_store,    s_eqquotval);
    a(s_eqquotval,     "\"",    0,0,          c_eat,            s_eqquotvaldone);
    a(s_eqquotval,     NULL,    0,255,        c_eat+c_store,    s_eqquotval);
    r(s_eqquotvaldone, t_val,                                    s_intag);

    a(s_eqval,        c_ws,    0,0,          c_nil,            s_eqvaldone);
    a(s_eqval,        "/>",    0,0,          c_nil,            s_eqvaldone);
    a(s_eqval,        NULL,    0,255,        c_eat+c_store,    s_eqval);

    r(s_eqvaldone,    t_val,                 s_intag);

    r(s_tagend,       t_endtag,              s_start);

    r(s_empty,          t_endtag,                               s_inempty);
    a(s_inempty,        ">",0,0,             c_eat,            s_emptyend);
    a(s_inempty,        NULL,0,255,          c_eat,            s_inempty);
    r(s_emptyend,    t_closeemptytag,        s_start);

    a(s_prolog,        "?",0,0,              c_eat,            s_prologq);
    a(s_prolog,        NULL,0,255,           c_eat+c_store,    s_prolog);

    a(s_prologq,    ">",0,0,                 c_eat,            s_prologdone);
    a(s_prologq,    NULL,0,255,              c_eat+c_store,    s_prolog);
    r(s_prologdone,    t_prolog,             s_start);

    a(s_tagexcl,    "-",0,0,                 c_eat,            s_commentdash1);
    a(s_tagexcl,    "D",0,0,                 c_nil,            s_dtd);
    a(s_tagexcl,    NULL,0,255,              c_eat,            s_start);

    a(s_commentdash1,    "-",0,0,            c_eat,            s_commentdash2);
    a(s_commentdash1,    NULL,0,255,         c_eat,            s_error);

    a(s_commentdash2,    "-",0,0,            c_eat,            s_commentenddash1);
    a(s_commentdash2,    NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_incomment,       "-",0,0,            c_eat,            s_commentenddash1);
    a(s_incomment,       NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_commentenddash1, "-",0,0,            c_eat,            s_commentenddash2);
    a(s_commentenddash1, NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_commentenddash2, ">",0,0,            c_eat,            s_commentdone);
    a(s_commentenddash2, NULL,0,255,         c_eat+c_store,    s_incomment);

    r(s_commentdone,     t_comment,          s_start);

}

/* *INDENT-ON* */

static int
is_oneof (char c, char *chars)
{
  while (*chars)
    {
      if (c == *chars)
        return 1;
      chars++;
    }
  return 0;
}

static int
nextchar (MrgXml *t)
{
  int       ret;

  if (t->file_in)
    {
      if (t->inbufpos >= t->inbuflen)
        {
          t->inbuflen = fread (t->inbuf, 1, CTX_XML_INBUF_SIZE, t->file_in);
          t->inbufpos = 0;
          if (!t->inbuflen)
            return -1;
        }

      ret = (int) t->inbuf[t->inbufpos++];

      if (ret == '\n')
        t->line_no++;
    }
  else
    {
      if (t->inbufpos >= t->inbuflen)
        {
          return -1;
        }
      ret = (int) t->inbuf[t->inbufpos++];
      if (ret == '\n')
        t->line_no++;
    }
  return ret;
}

int
xmltok_get (MrgXml *t, char **data, int *pos)
{
  state_entry *s;

  init_statetable ();
  ctx_string_clear (t->curdata);
  while (1)
    {
      if (!t->c_held)
        {
          t->c = nextchar (t);
          if (t->c == -1)
          {
            if (pos)*pos = t->inbufpos;
            return t_eof;
          }
          t->c_held = 1;
        }
      if (t->state == s_dtd)
        {     /* FIXME: should make better code for skipping DTD */

          /*            int angle = 0; */
          int       squote = 0;
          int       dquote = 0;
          int       abracket = 1;

          /*            int sbracket = 0; */

          ctx_string_append_byte (t->curdata, t->c);

          while (abracket)
            {
              switch (t->c = nextchar (t))
                {
                case -1:
                  return t_eof;
                case '<':
                  if ((!squote) && (!dquote))
                    abracket++;
                  ctx_string_append_byte (t->curdata, t->c);
                  break;
                case '>':
                  if ((!squote) && (!dquote))
                    abracket--;
                  if (abracket)
                    ctx_string_append_byte (t->curdata, t->c);
                  break;
                case '"':
                case '\'':
                case '[':
                case ']':
                default:
                  ctx_string_append_byte (t->curdata, t->c);
                  break;
                }
            }
          t->c_held = 0;
          t->state = s_start;

          if (pos)*pos = t->inbufpos;
          return t_dtd;
        }
      s = &state_table[t->state][0];
      while (s->state)
        {
          if (s->return_type != t_none)
            {
              *data = (char *) ctx_string_get (t->curdata);
              t->state = s->next_state;
              if (s->return_type == t_tag)
                ctx_string_set (t->curtag, ctx_string_get (t->curdata));
              if (s->return_type == t_endtag)
                *data = (char *) ctx_string_get (t->curtag);
              if (s->return_type == t_closeemptytag)
                *data = (char *) ctx_string_get (t->curtag);
              if (pos)
                *pos = t->inbufpos;
              return s->return_type;
            }
          if ((s->chars && is_oneof (t->c, s->chars))
              || ((s->r_start + s->r_end)
                  && (t->c >= s->r_start && t->c <= s->r_end)))
            {
              if (s->charhandling & c_store)
                {
                  ctx_string_append_byte (t->curdata, t->c);
                }
              if (s->charhandling & c_eat)
                {
                  t->c_held = 0;
                }
              t->state = s->next_state;
              break;
            }
          s++;
        }
    }
  if (pos)
    *pos = t->inbufpos;
  return t_eof;
}

MrgXml *
xmltok_new (FILE * file_in)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = file_in;
  ret->state = s_start;
  ret->curtag = ctx_string_new ("");
  ret->curdata = ctx_string_new ("");
  ret->inbuf = calloc (1, CTX_XML_INBUF_SIZE);
  return ret;
}

MrgXml *
xmltok_buf_new (char *membuf)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = NULL;
  ret->state = s_start;
  ret->curtag = ctx_string_new ("");
  ret->curdata = ctx_string_new ("");
  ret->inbuf = (void*)membuf;
  ret->inbuflen = strlen (membuf);
  ret->inbufpos = 0;
  return ret;
}

void
xmltok_free (MrgXml *t)
{
  ctx_string_free (t->curtag, 1);
  ctx_string_free (t->curdata, 1);

  if (t->file_in)
    {
      /*        fclose (t->file_in); */
      free (t->inbuf);
    }
  free (t);
}

char     *empty_tags[] = {
  "img", "IMG", "br", "BR", "hr", "HR", "META", "meta", "link", "LINK",
  NULL
};

char     *endomission_tags[] = {
  "li", "LI", "p", "P", "td", "TD", "tr", "TR", NULL
};

int
xmltok_lineno (MrgXml *t)
{
  return t->line_no;
}


void ctx_events_clear (Ctx *ctx)
{
  if (ctx_events_frozen (ctx))
    return;

  ctx_events_clear_items (ctx);
  //if (mrg->backend->mrg_clear)
  //  mrg->backend->mrg_clear (mrg);

  ctx_clear_bindings (ctx);
}

void mrg_clear (Mrg *mrg)
{
  ctx_events_clear (mrg->ctx);
  _mrg_clear_text_closures (mrg);
}

void itk_set_edge_right (Mrg *mrg, float val);
void itk_set_edge_left (Mrg *mrg, float val);
void itk_set_edge_top (Mrg *mrg, float val);
void itk_set_edge_bottom (Mrg *mrg, float val);
float mrg_edge_right (Mrg *mrg);
float mrg_edge_left (Mrg *mrg);
float mrg_y (Mrg *mrg);
float mrg_x (Mrg *mrg);

float mrg_em (Mrg *mrg);
void mrg_set_xy (Mrg *mrg, float x, float y);

static float _mrg_dynamic_edge_right2 (Mrg *mrg, MrgState *state)
{
  float ret = mrg_edge_right (mrg);
  float y   = mrg_y (mrg);
  float em  = mrg_em (mrg);
  int i;

  if (mrg->floats)
    for (i = state->float_base; i < mrg->floats; i++)
    {
      CtxFloatData *f = &mrg->float_data[i];
      if (f->type == CTX_FLOAT_RIGHT &&
          y >= f->y  &&
          y - em < f->y + f->height &&

          f->x < ret)
          ret = f->x;
    }
  return ret;
}

static float _mrg_dynamic_edge_left2 (Mrg *mrg, MrgState *state)
{
  float ret = mrg_edge_left (mrg);
  float y   = mrg_y (mrg);
  float em  = mrg_em (mrg);
  int i;

  if (mrg->floats)
    for (i = state->float_base; i < mrg->floats; i++)
    {
      CtxFloatData *f = &mrg->float_data[i];
      if (f->type == CTX_FLOAT_LEFT &&
          y >= f->y &&
          y - em < f->y + f->height &&
          f->x + f->width > ret)
          ret = f->x + f->width;
    }
  return ret;
}

static float _mrg_parent_dynamic_edge_left (Mrg *mrg)
{
  MrgState *state = mrg->state;
  if (mrg->state_no)
    state = &mrg->states[mrg->state_no-1];
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float _mrg_parent_dynamic_edge_right (Mrg *mrg)
{
  MrgState *state = mrg->state;
  if (mrg->state_no)
    state = &mrg->states[mrg->state_no-1];
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static float _mrg_dynamic_edge_left (Mrg *mrg)
{
  if (mrg->state->wrap_edge_left)
    return mrg->state->wrap_edge_left (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_left;
}
int  mrg_width (Mrg *mrg)
{
  if (!mrg) return 640;
  return ctx_width (mrg->ctx) / mrg->ddpx;
}

int  mrg_height (Mrg *mrg)
{
  if (!mrg) return 480;
  return ctx_height (mrg->ctx) / mrg->ddpx;
}

static float _mrg_dynamic_edge_right (Mrg *mrg)
{
  if (mrg->state->wrap_edge_right)
    return mrg->state->wrap_edge_right (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_right;
}

static float wrap_edge_left (Mrg *mrg, void *data)
{
  MrgState *state = mrg->state;
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float wrap_edge_right (Mrg *mrg, void *data)
{
  MrgState *state = mrg->state;
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static void clear_left (Mrg *mrg)
{
  float y = mrg_y (mrg);
  int i;

  if (mrg->floats)
  {
    for (i = mrg->state->float_base; i < mrg->floats; i++)
      {
        CtxFloatData *f = &mrg->float_data[i];
        {
          if (f->type == CTX_FLOAT_LEFT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

static void clear_right (Mrg *mrg)
{
  float y = mrg_y (mrg);
  int i;

  if (mrg->floats)
  {
    for (i = mrg->state->float_base; i < mrg->floats; i++)
      {
        CtxFloatData *f = &mrg->float_data[i];
        {
          if (f->type == CTX_FLOAT_RIGHT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  //fprintf (stderr, "%f}", y);
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

/**
 * ctx_style:
 * @mrg the mrg-context
 *
 * Returns the currently 
 *
 */
CtxStyle *ctx_style (Mrg *mrg)
{
  return &mrg->state->style;
}

static void clear_both (Mrg *mrg)
{
#if 0
  clear_left (mrg);
  clear_right (mrg);
#else
  float y = mrg_y (mrg);
  int i;

  if (!mrg->state)return;
  if (mrg->floats)
  {
    for (i = mrg->state->float_base; i < mrg->floats; i++)
      {
        CtxFloatData *f = &mrg->float_data[i];
        {
          if (f->y + f->height > y)
            y = f->y + f->height;
        }
      }
  }
  //y += mrg_em (mrg) * ctx_style(mrg)->line_height;
  mrg_set_xy (mrg, mrg_x (mrg), y);
#endif
}

static float mrg_edge_bottom  (Mrg *mrg)
{
  return mrg->state->edge_bottom;
}

static float mrg_edge_top  (Mrg *mrg)
{
  return mrg->state->edge_top;
}

float mrg_edge_left  (Mrg *mrg)
{
  return mrg->state->edge_left;
}

float mrg_edge_right (Mrg *mrg)
{
  return mrg->state->edge_right;
}

float _mrg_dynamic_edge_right (Mrg *mrg);
float _mrg_dynamic_edge_left (Mrg *mrg);

void itk_set_edge_top (ITK *itk, float edge)
{
  Mrg *mrg = itk;
  itk->state->edge_top = edge;

  // we always set top last, since it causes the
  // reset of line handling
  //
  mrg_set_xy (mrg, _mrg_dynamic_edge_left (mrg) +
		  ctx_get_float (mrg_ctx(mrg), SQZ_text_indent)
      , mrg->state->edge_top);// + mrg_em (mrg));


   itk->edge_top = edge;
   itk->edge_bottom = itk->height + itk->edge_top;
}

void  itk_set_edge_left (Mrg *itk, float val)
{
  itk->state->edge_left = val;
  itk->edge_left = val;
  itk->edge_right = itk->width + itk->edge_left;
}

void itk_set_edge_bottom (ITK *itk, float edge)
{
   itk->state->edge_bottom = edge;
   itk->edge_bottom = edge;
   itk->height = itk->edge_bottom - itk->edge_top;
}

void itk_set_edge_right (ITK *itk, float edge)
{
   itk->state->edge_right = edge;
   itk->edge_right = edge;
   itk->width      = itk->edge_right - itk->edge_left;
}

float mrg_rem (Mrg *mrg)
{
  return mrg->rem;
}


void itk_start            (Mrg *mrg, const char *class_name, void *id_ptr);
void itk_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style);
void itk_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                            const char *format, ...);

static void ctx_parse_style_id (Mrg          *mrg,
                                const char   *style_id,
                                CtxStyleNode *node)
{
  const char *p;
  char temp[128] = "";
  int  temp_l = 0;
  if (!style_id)
  {
    return; // XXX: why does this happen?
  }

  memset (node, 0, sizeof (CtxStyleNode));

  for (p = style_id; ; p++)
  {
    switch (*p)
    {
      case '.':
      case ':':
      case '#':
      case 0:
        if (temp_l)
        {
          switch (temp[0])
          {
            case '.':
              {
                int i = 0;
                for (i = 0; node->classes_hash[i]; i++);
                node->classes_hash[i] = ctx_strhash (&temp[1]);
              }
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; node->pseudo[i]; i++);
                node->pseudo[i] = mrg_intern_string (&temp[1]);
                for (i = 0; node->pseudo_hash[i]; i++);
                node->pseudo_hash[i] = ctx_strhash (&temp[1]);
              }
              break;
            case '#':
              node->id = mrg_intern_string (&temp[1]);
              node->id_hash = ctx_strhash (&temp[1]);
              break;
            default:
              node->element_hash = ctx_strhash (temp);
              break;
          }
          temp_l = 0;
        }
        if (*p == 0)
          return;
        temp[temp_l++] = *p;  // XXX: added to make reported fallthrough
        temp[temp_l]=0;       //      not be reported - butexplicit
        break;
      default:
        temp[temp_l++] = *p;
        temp[temp_l]=0;
    }
  }
}

void _ctx_initial_style (Mrg *mrg)
{
  CtxStyle *s = ctx_style (mrg);

  /* things not set here, are inherited from the parent style context,
   * more properly would be to rig up a fresh context, and copy inherited
   * values over, that would permit specifying inherit on any propery.
   */

  s->text_decoration= 0;
  s->display  = CTX_DISPLAY_INLINE;
  s->float_   = CTX_FLOAT_NONE;
  s->clear    = CTX_CLEAR_NONE;
  s->overflow = CTX_OVERFLOW_VISIBLE;
  s->position = CTX_POSITION_STATIC;

  SET_PROP(border_top_width,    0);
  SET_PROP(border_left_width,   0);
  SET_PROP(border_right_width,  0);
  SET_PROP(border_bottom_width, 0);
  SET_PROP(margin_top,    0);
  SET_PROP(margin_left,   0);
  SET_PROP(margin_right,  0);
  SET_PROP(margin_bottom, 0);
  SET_PROP(padding_top,    0);
  SET_PROP(padding_left,   0);
  SET_PROP(padding_right,  0);
  SET_PROP(padding_bottom, 0);
  SET_PROP(top,    0);
  SET_PROP(left,   0);
  SET_PROP(right,  0);
  SET_PROP(bottom, 0);

  SET_PROP(min_width, 0);
  SET_PROP(max_width, 0);
  s->width_auto = 1;
  s->margin_left_auto = 0;
  s->margin_right_auto = 0;
  SET_PROP(width, 42);

  SET_PROPS(class,"");
  SET_PROPS(id,"");

  ctx_set_float (mrg->ctx, SQZ_stroke_width, 0.2);

  CtxColor *color = ctx_color_new ();
  ctx_get_color (mrg->ctx, SQZ_color, color);

  ctx_set_color (mrg->ctx, SQZ_border_top_color, color);
  ctx_set_color (mrg->ctx, SQZ_border_bottom_color, color);
  ctx_set_color (mrg->ctx, SQZ_border_left_color, color);
  ctx_set_color (mrg->ctx, SQZ_border_right_color, color);

#if 0
  s->stroke_color.red = 1;
  s->stroke_color.green = 0;
  s->stroke_color.blue = 1;
  s->stroke_color.alpha = 1;
  s->fill_color.red = 1;
  s->fill_color.green = 1;
  s->fill_color.blue = 0;
  s->fill_color.alpha = 1;
#endif

  ctx_color_set_from_string (mrg->ctx, color, "transparent");
  ctx_set_color (mrg->ctx, SQZ_background_color, color);
  ctx_color_free (color);
}



/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

const char * html_css =
"html,address,\n"
"blockquote,\n"
"body,dd, div,\n"
"dl,dt,fieldset, form,\n"
"frame,frameset,\n"
"h1,h2,h3, h4,\n"
"h5,h6,noframes,\n"
"ol,p,ul,center,\n"
"dir,hr,menu,pre{display:block;unicode-bidi:embed}\n"
"h1,h2,h3,h4,h5{page-break-after:avoid}\n"
"li{display:list-item}\n"
"table{display:block;}\n"
//"table{display:table}\n"
//"tr{display:table-row}\n"
"tr{display:block}\n"
"thead{display:table-header-group }\n"
"tbody{display:table-row-group }\n"
"tfoot{display:table-footer-group }\n"
"col{display:table-column}\n"
"img{display:inline-block}\n"
"colgroup{display:table-column-group}\n"
"td,th{display:block-inline}\n"
//"td,th{display:table-cell}\n"
"caption{display:table-caption}\n"
"th{font-weight:bolder;text-align:center}\n"
"caption{text-align:center}\n"
"body{margin:8px;}\n" // was 0.5em
"h1{font-size:2em;margin:.67em 0;}\n"
"h2{font-size:1.5em;margin:.75em 0}\n"
"h3{font-size:1.17em;margin:.83em 0}\n"
"h4,p,"
"blockquote,ul,"
"fieldset,form,"
"ol,dl,dir,"
"menu{margin:1.12em 0}\n"
"h5{font-size:.83em;margin: 1.5em 0}\n"
"h6{font-size:.75em;margin: 1.67em 0}\n"
"h1,h2,h3,h4,\n"
"h5,h6,b,\n"
"strong{font-weight:bolder}\n"
"blockquote{margin-left:4em;margin-right:4em}\n"
"i,cite,em,\n"
"var,address{font-style:italic}\n"
"pre,tt,code,\n"
"kbd,samp{font-family:monospace}\n"
"pre{white-space:pre}\n"
"button,textarea,\n"
"input,select{display:inline-block}\n"
"big{font-size:1.17em}\n"
"small,sub,sup{font-size:.83em}\n"
"sub{vertical-align:sub}\n"
"sup{vertical-align:super}\n"
"table{border-spacing:2px;}\n"
"thead,tbody,\n"
"tfoot{vertical-align:middle}\n"
"td,th,tr{vertical-align:inherit}\n"
"s,strike,del{text-decoration:line-through}\n"
"ol,ul,dir,"
"menu{padding-left:2.5em}\n"
"dd{margin-left:3em}\n"
"ol{list-style-type:decimal}\n"
"ol ul,ul ol,"
"ul ul,ol ol{margin-top: 0;margin-bottom: 0}\n"
"u,ins{text-decoration:underline}\n"
//"br:before{content:\"\\A\";white-space:pre-line}\n"
"center{text-align:center}\n"
":link,:visited{text-decoration:underline}\n"
":focus{outline:thin dotted invert}\n"
".cursor{color:white;background: black;} \n"
"br{display:block;}\n"
"html{font-weight:normal;background-color:white;}\n"
"body{background-color:transparent;color:black;}\n"
"a{color:blue;text-decoration: underline;}\n"
"a:hover{background:black;color:white;text-decoration:underline; }\n"
"head{display:none;}\n"
"title{display:none;}\n"
//"hr{border:1px inset black;margin-bottom: 0.5em;margin-top:0.5em;}\n"
"hr{font-size:1px;border-bottom: 1px solid red;display:block;}\n"

".scroll{background:blue;}\n"
".wallpaper, .itk{background:#124;color:#f43}\n"
".focused{background:#fff;color:#000}\n"

" .children{ border:1px solid green;padding-left:3em; }\n"
" .selection{background:white; color: red; }\n"
" :focused {color: yellow; }\n"
" .fnord { color: green; }\n"
"   .item { border: 1px solid transparent; color: white; }"
"   .item:focused { color: yellow; border: 1px solid blue;}"
 /*  .item>.text::before { content:"-";display:block;position:absolute;margin-left:-1.2em; } */

" .file { border: 1px solid green; margin-left:0;margin-right:0.2em; width: 3em; margin-bottom: 0.2em; height: 3em; display: inline-block;  }"


//"h1,h2,h3,h4,h5,h6,p,div,b,span,ul,li,ol,dl,dt,dl,propline,button{border-left-color:gray;border-right-color:gray;}\n"

//"html{font-size:10.0px;color:white;}\n" // from ACID1 - not parsed right

//// itk defaults stylesheet
"toggle        {border: 1px solid green;border: 1px solid red;color:yellow;display:block;}\n"
"button        {border: 1px solid green;}\n"
"choice        {border: 1px solid brown;display:inline-block;}\n"
".choice_menu_wrap  {border: 1px solid red;display:block;position:relative;top:-1.2em;left:4em;height:0;width:0;}\n"
".choice_menu  {border: 1px solid red;display:block;position:absolute;width:7em;}\n"
".choice       {border: 1px solid brown;display:block;background:black}\n"
".choice:chosen{border: 1px solid brown;display:block;background:red;}\n"
"button:focused{color:yellow;border: 1px solid yellow;background:blue;}\n"
"label         {display:inline; color:white;}\n"
"slider        {display:inline-block; color:white;}\n"
"propline      {display:block;margin-top:0.25em;margin-bottom:0.25em;border:1 px solid transparent;}\n"
"propline:focused {border:1px solid red;background:#faf}\n"
;


typedef struct CtxStyleEntry {
  char        *selector;
  CtxStyleNode parsed[CTX_MAX_SELECTOR_LENGTH];
  int          sel_len;
  char        *css;
  int          specificity;
} CtxStyleEntry;

static void free_entry (CtxStyleEntry *entry)
{
  free (entry->selector);
  free (entry->css);
  free (entry);
}

static int ctx_css_compute_specifity (const char *selector, int priority)
{
  const char *p;
  int n_id = 0;
  int n_class = 0;
  int n_tag = 0;
  int n_pseudo = 0;
  int in_word = 0;

  for (p = selector; *p; p++)
  {
    switch (*p)
    {
      case ' ':
        in_word = 0;
        break;
      case ':':
        in_word = 1;
        n_pseudo++;
        break;
      case '.':
        in_word = 1;
        n_class++;
        break;
      case '#':
        in_word = 1;
        n_id++;
        break;
      default:
        if (!in_word)
        {
          in_word = 1;
          n_tag++;
        }
    }
  }
  return priority * 100000 + n_pseudo * 10000 + n_id * 1000 + n_class * 100 + n_tag * 10;
}

static void ctx_css_parse_selector (Mrg *mrg, const char *selector, CtxStyleEntry *entry)
{
  const char *p = selector;
  char section[256];
  int sec_l = 0;

  char type = ' ';

  int direct_descendant = 0;
  for (p = selector; ; p++)
  {
    switch (*p)
    {
	case '.': case ':': case '#': case ' ': case 0: case '>':
        if (sec_l)
        {
          switch (type)
          {
            case ' ':
              entry->parsed[entry->sel_len].element_hash = ctx_strhash (section);
	      entry->parsed[entry->sel_len].direct_descendant = direct_descendant;
	      direct_descendant = 0;
              break;
            case '#':
              entry->parsed[entry->sel_len].id = mrg_intern_string (section);
              entry->parsed[entry->sel_len].id_hash = ctx_strhash (section);
	      entry->parsed[entry->sel_len].direct_descendant = direct_descendant;
	      direct_descendant = 0;
              break;
            case '.':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].classes_hash[i]; i++);
                entry->parsed[entry->sel_len].classes_hash[i] = ctx_strhash (section);
              }
	      entry->parsed[entry->sel_len].direct_descendant = direct_descendant;
	      direct_descendant = 0;
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].pseudo[i]; i++);
                entry->parsed[entry->sel_len].pseudo[i] = mrg_intern_string (section);
                for (i = 0; entry->parsed[entry->sel_len].pseudo_hash[i]; i++);
                entry->parsed[entry->sel_len].pseudo_hash[i] = ctx_strhash (section);
  	        entry->parsed[entry->sel_len].direct_descendant = direct_descendant;
	        direct_descendant = 0;
              }
              break;
          }
        if (*p == ' ' || *p == 0 || *p == '>')
	{
          entry->sel_len ++;
	}
        }
        type = *p;
	if (*p == '>')
	{
          direct_descendant = 1;
	  type = ' ';
	}
        if (*p == 0)
        {

#if 0
  fprintf (stderr, "%s: ", selector);
  for (int i = 0; i < entry->sel_len; i++)
  {
    if (entry->parsed[i].direct_descendant)
	    fprintf (stderr, "DP ");
    fprintf (stderr, "e: %s ", ctx_str_decode (entry->parsed[i].element_hash));
    for (int j = 0; entry->parsed[i].classes_hash[j]; j++)
      fprintf (stderr, "c: %s ", ctx_str_decode (entry->parsed[i].classes_hash[j]));
  }
  fprintf (stderr, "\n");
#endif

          return;
        }
        section[(sec_l=0)] = 0;
        break;
      default:
        section[sec_l++] = *p;
        section[sec_l] = 0;
        break;
    }
  }

  // not reached
}

static void ctx_stylesheet_add_selector (Mrg *mrg, const char *selector, const char *css, int priority)
{
  CtxStyleEntry *entry = calloc (sizeof (CtxStyleEntry), 1);
  entry->selector = strdup (selector);
  entry->css = strdup (css);
  entry->specificity = ctx_css_compute_specifity (selector, priority);

  //fprintf (stderr, "\nsel:%s]\ncss:%s\npri:%i\n", selector, css, priority);

  ctx_css_parse_selector (mrg, selector, entry);
  ctx_list_prepend_full (&mrg->stylesheet, entry, (void*)free_entry, NULL);
}


#define MAKE_ERROR \
 if (error)\
 {\
   char errbuf[128];\
   sprintf (errbuf, "%i unexpected %c at %i'  %c%c%c", __LINE__, *p, (int)(p-css),\
     p[0], p[1], p[2]);\
   *error = strdup (errbuf);\
 }


typedef struct _CtxCssParseState CtxCssParseState;

struct _CtxCssParseState {
  int   state;
  char  rule[CTX_MAX_CSS_RULES][CTX_MAX_CSS_RULELEN];
  int   rule_no ;
  int   rule_l[CTX_MAX_CSS_RULES];
  char  val[CTX_MAX_CSS_STRINGLEN];
  int   val_l;
};

/* XXX: this funciton has no buffer bounds checking */
/* doesnt balance {} () [] and quotes */

enum
{
  NEUTRAL = 0,
  NEUTRAL_COMMENT,
  IN_RULE,
  RULE_COMMENT,
  IN_ARULE,
  ARULE_COMMENT,
  IN_IMPORT,
  IMPORT_COMMENT,
  EXPECT_COLON,
  COLON_COMMENT,
  EXPECT_VAL,
  EXPECT_VAL_COMMENT,
  IN_VAL,
  VAL_COMMENT,
};


static void _ctx_stylesheet_add (CtxCssParseState *ps, Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  const char *p;
  if (!ps)
    ps = mrg->css_parse_state = calloc (sizeof (CtxCssParseState), 1);

  if (!css)
    return;

  for (p = css; *p; p++)
  {
    switch (ps->state)
    {
      case NEUTRAL_COMMENT:
      case RULE_COMMENT:
      case ARULE_COMMENT:
      case IMPORT_COMMENT:
      case VAL_COMMENT:
      case EXPECT_VAL_COMMENT:
      case COLON_COMMENT:      if (p[0] == '*' && p[1] == '/') { p++; ps->state--; } break;
      case NEUTRAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = NEUTRAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\r':
          case '\t':
          case ';':
            break;
          case '{':
          case '}':
            MAKE_ERROR;
            return;
          case '@':
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_ARULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = ARULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL; // should be AVAL for media sections...
            break;
          case '\n':
          case '\r':
          case '\t':
          case ' ':
            if (!strcmp (ps->rule[0], "import"))
            {
              MAKE_ERROR;
            }
            else
            {
              ps->state = IN_IMPORT;
            }
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_IMPORT:
        switch (*p)
        {
          int no;
          case '/': if (p[1] == '*') { p++; ps->state = IMPORT_COMMENT; } break;
          case ';':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\r' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            while (ps->val_l && ps->val[ps->val_l-1] != ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == '"')
              ps->val_l--;
            ps->val[ps->val_l]=0;

            if(mrg->mrg_get_contents){
              char *contents = NULL;
              long length;
              char *uri = ps->val;

              /* really ugly string trimming to get to import uri.. */
              while (uri[0]==' ') uri++;
              if (!memcmp (uri, "url", 3)) uri+=3;
              if (uri[0]=='(') uri++;
              if (uri[0]=='"') uri++;
      
              /* XXX: should parse out the media part, and check if we should
               * really include this file
               */

              mrg_get_contents (mrg, mrg->uri_base, uri, &contents, &length);
              if (contents)
              {
                CtxCssParseState child_parser = {0,};
                _ctx_stylesheet_add (&child_parser, mrg, contents, uri, priority, error);
                free (contents);
              }
	      else
	      {
		 fprintf (stderr, "404 - %s\n", uri);
	      }
            }

            for (no = 0; no < ps->rule_no+1; no ++)
              ps->rule[no][ps->rule_l[no]=0] = 0;
            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_IMPORT;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
      case IN_RULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = RULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL;
            break;
          case '\n':
          case '\r':
          case '\t':
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = ' ';
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          case ',':
            ps->state = NEUTRAL;
            ps->rule_no++;
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case EXPECT_COLON:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = COLON_COMMENT; } break;
          case ' ':
          case '\n':
          case '\r':
          case '\t':
            break;
          case ':':
            ps->state = EXPECT_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case EXPECT_VAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = EXPECT_VAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\r':
          case '\t':
          case ';':
            break;
          case '{':
            ps->state = IN_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case IN_VAL:
        switch (*p)
        {
          int no;

          case '/': if (p[1] == '*') { p++; ps->state = VAL_COMMENT; } break;
          case '}':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\r' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            ps->val[ps->val_l]=0;

            for (no = 0; no < ps->rule_no+1; no ++)
            {
              while (ps->rule_l[no] && (
                  ps->rule[no][ps->rule_l[no]-1] == ' ' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\n' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\r' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\t'))
                ps->rule_l[no]--;
              ps->rule[no][ps->rule_l[no]]=0;

              ctx_stylesheet_add_selector (mrg, ps->rule[no], ps->val, priority);
              ps->rule[no][ps->rule_l[no]=0] = 0;
            }

            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_VAL;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
    }
  }
}

void ctx_stylesheet_add (Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  CtxCssParseState *ps = mrg->css_parse_state;
  _ctx_stylesheet_add (ps, mrg, css, uri_base, priority, error);
}
#define CTX_STYLE_INTERNAL 10
#define CTX_STYLE_GLOBAL   15
#define CTX_STYLE_XML      20
#define CTX_STYLE_APP      20
#define CTX_STYLE_INLINE   25
#define CTX_STYLE_CODE     30

void itk_css_default (Mrg *mrg)
{
  char *error = NULL;
  ctx_stylesheet_add (mrg, html_css, NULL, CTX_STYLE_INTERNAL, &error);
  if (error)
  {
    fprintf (stderr, "Mrg css parsing error: %s\n", error);
  }

  ctx_stylesheet_add (mrg,
"bold{font-weight:bold;}"
"dim*,dim{opacity:0.5;}"
"underline*,underline{text-decoration:underline;}"
"reverse*,selected*,reverse,selected{text-decoration:reverse;}"
"unhandled{color:cyan;}"
"binding:key{background-color:white;color:black;}"
"binding:label{color:cyan;}"
      ,NULL, CTX_STYLE_INTERNAL, &error);

  if (error)
  {
    fprintf (stderr, "mrg css parsing error: %s\n", error);
  }
}

void itk_stylesheet_clear (Mrg *mrg)
{
  if (mrg->stylesheet)
    ctx_list_free (&mrg->stylesheet);
  itk_css_default (mrg);
}

typedef struct CtxStyleMatch
{
  CtxStyleEntry *entry;
  int score;
} CtxStyleMatch;

static int compare_matches (const void *a, const void *b, void *d)
{
  const CtxStyleMatch *ma = a;
  const CtxStyleMatch *mb = b;
  return mb->score - ma->score;
}

static inline int _ctx_nth_match (const char *selector, int child_no)
{
  const char *tmp = selector + 10;
  int a = 0;
  int b = 0;

  if (!strcmp (tmp, "odd)"))
  {
    a = 2; b = 1;
  }
  else if (!strcmp (tmp, "even)"))
  {
    a = 2; b = 0;
  }
  else
  {
    if (ctx_strchr (tmp, 'n'))
    {
      a = atoi (tmp);
      b = atoi (ctx_strchr (tmp, 'n')+1);
    }
    else
    {
      b = atoi (tmp);
    }
  }

  if (!a)
    return child_no == b;
  else
    if (child_no == b ||
       ((child_no - b > 0) == (a>0)))
      return !((child_no - b) % a);

  return 0;
}

int _ctx_child_no (Mrg *mrg)
{
  return mrg->states[mrg->state_no-1].children;
}

static inline int match_nodes (Mrg *mrg, CtxStyleNode *sel_node, CtxStyleNode *subject)
{
  int j, k;

  if (sel_node->element_hash &&
      sel_node->element_hash != subject->element_hash)
    return 0;

  if (sel_node->id &&
      sel_node->id != subject->id)
    return 0;

  for (j = 0; sel_node->classes_hash[j]; j++)
  {
    int found = 0;
    for (k = 0; subject->classes_hash[k] && !found; k++)
    {
      if (sel_node->classes_hash[j] == subject->classes_hash[k])
        found = 1;
    }
    if (!found)
      return 0;
  }
  for (j = 0; sel_node->pseudo[j]; j++)
  {
    if (ctx_strhash (sel_node->pseudo[j]) == SQZ_first_child)
    {
      if (!(_ctx_child_no (mrg) == 1))
        return 0;
    }
    else if (!strncmp (sel_node->pseudo[j], "nth-child(", 10))
    {
      if (!_ctx_nth_match (sel_node->pseudo[j], _ctx_child_no (mrg)))
        return 0;
    }
    else
    {
      int found = 0;

      for (k = 0; subject->pseudo[k] && !found; k++)
      {
        if (sel_node->pseudo_hash[j] == subject->pseudo_hash[k])
          found = 1;
      }
      if (!found)
        return 0;
    }
  }
  return 1;
}

static int ctx_selector_vs_ancestry (Mrg *mrg,
                                     CtxStyleEntry *entry,
                                     CtxStyleNode **ancestry,
                                     int a_depth)
{
  int s = entry->sel_len - 1;
#if 1
  /* right most part of selector must match */
  if (entry->parsed[s].direct_descendant == 0)
  {
    if (!match_nodes (mrg, &entry->parsed[s], ancestry[a_depth-1]))
      return 0;

    s--;
    a_depth--;
  }

  if (s < 0)
    return 1;
#endif

  while (s >= 0)
  {
    int ai;
    int found_node = 0;

    if (entry->parsed[s].direct_descendant && s > 0)
    {  // s>0 should always be true when direct_descendant is true
      ai = a_depth-1;
      {
        if (s >0 && ai >0 && match_nodes (mrg, &entry->parsed[s], ancestry[ai]) &&
            match_nodes (mrg, &entry->parsed[s-1], ancestry[ai-1]))
          found_node = 1;
      }
      ai--;
      s-=2;
    }
    else
    {
      for (ai = a_depth-1; ai >= 0 && !found_node; ai--)
      {
        if (match_nodes (mrg, &entry->parsed[s], ancestry[ai]))
          found_node = 1;
      }
      s--;
    }
    if (found_node)
    {
      a_depth = ai;
    }
    else
    {
      return 0;
    }
  }

  return 1;
}

static int ctx_css_selector_match (Mrg *mrg, CtxStyleEntry *entry, CtxStyleNode **ancestry, int a_depth)
{
  if (entry->selector[0] == '*' &&
      entry->selector[1] == 0)
    return entry->specificity;

  if (a_depth == 0)
    return 0;

  if (ctx_selector_vs_ancestry (mrg, entry, ancestry, a_depth))
    return entry->specificity;

  return 0;
}

static char *_ctx_css_compute_style (Mrg *mrg, CtxStyleNode **ancestry, int a_depth)
{
  CtxList *l;
  CtxList *matches = NULL;
  int totlen = 2;
  char *ret = NULL;

  for (l = mrg->stylesheet; l; l = l->next)
  {
    CtxStyleEntry *entry = l->data;
    int score = ctx_css_selector_match (mrg, entry, ancestry, a_depth);

    if (score)
    {
      CtxStyleMatch *match = malloc (sizeof (CtxStyleMatch));
      match->score = score;
      match->entry = entry;
      ctx_list_prepend_full (&matches, match, (void*)free, NULL);
      totlen += strlen (entry->css) + 1;
    }
  }

  if (matches)
  {
    CtxList *l;
    char *p;

    p = ret = malloc (totlen);

    ctx_list_sort (&matches, compare_matches, NULL);
    for (l = matches; l; l = l->next)
    {
      CtxStyleMatch *match = l->data;
      CtxStyleEntry *entry = match->entry;
      strcpy (p, entry->css);
      p += strlen (entry->css);
      strcpy (p, ";");
      p ++;
    }
    ctx_list_free (&matches);
  }
  return ret;
}

static int _ctx_get_ancestry (Mrg *mrg, CtxStyleNode **ancestry)
{
  int i, j;
  for (i = 0, j = 0; i <= mrg->state_no; i++)
    if (mrg->states[i].style_id)
    {
      ancestry[j++] = &(mrg->states[i].style_node);
    }
  ancestry[j] = NULL;
  return j;
}

char *_ctx_stylesheet_collate_style (Mrg *mrg)
{
  CtxStyleNode *ancestry[CTX_MAX_STYLE_DEPTH];
  int ancestors = _ctx_get_ancestry (mrg, ancestry);
  char *ret = _ctx_css_compute_style (mrg, ancestry, ancestors);
  return ret;
}

void  mrg_set_line_height (Mrg *mrg, float line_height)
{
  ctx_style (mrg)->line_height = line_height;
}

float mrg_line_height (Mrg *mrg)
{
  return ctx_style (mrg)->line_height;
}

void  mrg_set_rem         (Mrg *mrg, float em)
{
  mrg->rem = em;
}

float mrg_em (Mrg *mrg)
{
  return mrg->state->style.font_size;
}

void  mrg_set_em (Mrg *mrg, float em)
{
  mrg->state->style.font_size = em;
}

void ctx_css_set (Mrg *mrg, const char *css)
{
  ctx_string_set (mrg->style, css);
}

void ctx_css_add (Mrg *mrg, const char *css)
{
  ctx_string_append_str (mrg->style, css);
}

void itk_stylesheet_clear (Mrg *mrg);
void ctx_stylesheet_add (Mrg *mrg, const char *css, const char *uri,
                         int priority,
                         char **error);

void ctx_css_set (Mrg *mrg, const char *css);
void ctx_css_add (Mrg *mrg, const char *css);

static inline float mrg_parse_px_x (Mrg *mrg, const char *str, char **endptr)
{
  float result = 0;
  char *end = NULL;
#define PPI   96

  if (!str)
    return 0.0;

  result = _ctx_parse_float (str, &end);
  if (endptr)
    *endptr=end;

  //if (end[0]=='%v') /// XXX  % of viewport; regard less of stacking
  if (end[0]=='%')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));

    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'h')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'w')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='r' && end[1]=='e' && end[2]=='m')
  {
    result *= mrg_rem (mrg);
    if (endptr)
      *endptr=end + 3;
  }
  else if (end[0]=='e' && end[1]=='m')
  {
    result *= mrg_em (mrg);
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='x')
  {
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='t')
  {
    result = (result / PPI) * 72;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='c')
  {
    result = (result / PPI) * 72 / 12;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='i' && end[1]=='n')
  {
    result = result / PPI;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='c' && end[1]=='m')
  {
    result = (result / PPI) * 2.54;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='m' && end[1]=='m')
  {
    result = (result / PPI) * 25.4;
    if (endptr)
      *endptr=end + 2;
  }
  return result;
}

static inline float mrg_parse_px_y (Mrg *mrg, const char *str, char **endptr)
{
  float result = 0;
  char *end = NULL;
  if (!str)
    return 0.0;

  result = _ctx_parse_float (str, &end);
  if (endptr)
    *endptr=end;

  if (end[0]=='%')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'h')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'w')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='r' && end[1]=='e' && end[2]=='m')
  {
    result *= mrg_rem (mrg);
    if (endptr)
      *endptr=end + 3;
  }
  else if (end[0]=='e' && end[1]=='m')
  {
    result *= mrg_em (mrg);
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='x')
  {
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='t')
  {
    result = (result / PPI) * 72;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='c')
  {
    result = (result / PPI) * 72 / 12;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='i' && end[1]=='n')
  {
    result = result / PPI;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='c' && end[1]=='m')
  {
    result = (result / PPI) * 2.54;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='m' && end[1]=='m')
  {
    result = (result / PPI) * 25.4;
    if (endptr)
      *endptr=end + 2;
  }
  return result;
}

static inline int mrg_parse_pxs (Mrg *mrg, const char *str, float *vals)
{
  int n_floats = 0;
  char *p =    (void*)str;
  char *prev = (void *)NULL;

  for (; p && p != prev && *p; )
  {
    float val;
    prev = p;
    val = n_floats%2==1?
      mrg_parse_px_x (mrg, p, &p):mrg_parse_px_y (mrg, p, &p);
    if (p != prev)
    {
      vals[n_floats++] = val;
    }
  }

  return n_floats;
}


static inline void ctx_css_handle_property_pass0 (Mrg *mrg, uint32_t key,
                                                  const char *value)
{
  /* pass0 deals with properties that parsing of many other property
   * definitions rely on */
  if (key == SQZ_font_size)
  {
    float parsed;
    
    if (mrg->state_no)
    {
      mrg->state_no--;
      parsed = mrg_parse_px_y (mrg, value, NULL);
      mrg->state_no++;
    }
    else
    {
      parsed = mrg_parse_px_y (mrg, value, NULL);
    }
    mrg_set_em (mrg, parsed);
  }
  else if (key == SQZ_color)
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, value);
    ctx_set_color (mrg->ctx, SQZ_color, color);
    ctx_color_free (color);
  }
}

static void ctx_css_handle_property_pass1 (Mrg *mrg, uint32_t key,
                                           const char *value)
{
  CtxStyle *s = ctx_style (mrg);
  uint32_t val_hash = ctx_strhash (value);

  switch (key)
  {
    default:
      SET_PROPSh(key, value);
      break;
    case SQZ_right:
    case SQZ_bottom:
    case SQZ_width:     // handled in pass1m
    case SQZ_color:     // handled in pass0
    case SQZ_font_size: // handled in pass0
      break;
    case SQZ_top:
    case SQZ_height:
    case SQZ_line_width:
    case SQZ_text_indent:
    case SQZ_letter_spacing:
    case SQZ_word_spacing:
    case SQZ_stroke_width:
    case SQZ_text_stroke_width:
    case SQZ_line_height:
    case SQZ_border_top_width:
    case SQZ_border_bottom_width:
    case SQZ_margin_top:
    case SQZ_margin_bottom:
    case SQZ_padding_bottom:
    case SQZ_padding_top:
    case SQZ_min_height:
    case SQZ_max_height:
      SET_PROPh(key, mrg_parse_px_y (mrg, value, NULL));
      break;
    case SQZ_border_right_width:
    case SQZ_border_left_width:
    case SQZ_left:
    case SQZ_tab_size:
    case SQZ_min_width:
    case SQZ_max_width:
    case SQZ_padding_left:
    case SQZ_padding_right:
      SET_PROPh(key, mrg_parse_px_x (mrg, value, NULL));
      break;
    case SQZ_margin:
      {
        float vals[10];
        int   n_vals;
        n_vals = mrg_parse_pxs (mrg, value, vals);
	float res[4] = {0.0f,0.0f,0.0f,0.0f};
        switch (n_vals)
        {
          case 1:
            res[0] = res[1] = res[2] = res[3] = vals[0];
            break;
          case 2:
            res[0] = vals[0]; res[1] = vals[1]; res[0] = vals[0]; res[1] = vals[1];
            break;
          case 3:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[1];
            break;
          case 4:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[3];
            break;
        }
        SET_PROP(margin_top,    res[0]);
        SET_PROP(margin_right,  res[1]);
        SET_PROP(margin_bottom, res[2]);
        SET_PROP(margin_left,   res[3]);
      }
      break;
    case SQZ_margin_left:
    {
      if (val_hash == SQZ_auto)
      {
        s->margin_left_auto = 1;
      }
      else
      {
        SET_PROP(margin_left, mrg_parse_px_x (mrg, value, NULL));
        s->margin_left_auto = 0;
      }
    }
      break;
    case SQZ_margin_right:
    {
      if (val_hash == SQZ_auto)
      {
        s->margin_right_auto = 1;
      }
      else
      {
        SET_PROP(margin_right, mrg_parse_px_x (mrg, value, NULL));
        s->margin_right_auto = 0;
      }
    }
      break;
  
    case SQZ_padding:
      {
        float vals[10];
        int   n_vals;
        n_vals = mrg_parse_pxs (mrg, value, vals);
	float res[4] = {0.0f,0.0f,0.0f,0.0f};
        switch (n_vals)
        {
          case 1:
            res[0] = res[1] = res[2] = res[3] = vals[0];
            break;
          case 2:
            res[0] = vals[0]; res[1] = vals[1]; res[0] = vals[0]; res[1] = vals[1];
            break;
          case 3:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[1];
            break;
          case 4:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[3];
            break;
        }
        SET_PROP(padding_top,    res[0]);
        SET_PROP(padding_right,  res[1]);
        SET_PROP(padding_bottom, res[2]);
        SET_PROP(padding_left,   res[3]);
      }
      break;
    case SQZ_visibility:
    {
      if      (val_hash == SQZ_visible) s->visibility = CTX_VISIBILITY_VISIBLE;
      else if (val_hash == SQZ_hidden)  s->visibility = CTX_VISIBILITY_HIDDEN;
      else                              s->visibility = CTX_VISIBILITY_VISIBLE;
    }
    break;
  
    case SQZ_border_width:
      {
        float vals[10];
        int   n_vals;
        n_vals = mrg_parse_pxs (mrg, value, vals);
	float res[4] = {0.0f,0.0f,0.0f,0.0f};
        switch (n_vals)
        {
          case 1:
            res[0] = res[1] = res[2] = res[3] = vals[0];
            break;
          case 2:
            res[0] = vals[0]; res[1] = vals[1]; res[0] = vals[0]; res[1] = vals[1];
            break;
          case 3:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[1];
            break;
          case 4:
            res[0] = vals[0]; res[1] = vals[1]; res[2] = vals[2]; res[3] = vals[3];
            break;
        }
        SET_PROP(border_top_width,    res[0]);
        SET_PROP(border_right_width,  res[1]);
        SET_PROP(border_bottom_width, res[2]);
        SET_PROP(border_left_width,   res[3]);
      }
      break;
    case SQZ_border_color:
      {
        CtxColor *color = ctx_color_new ();
        ctx_color_set_from_string (mrg->ctx, color, value);
        ctx_set_color (mrg->ctx, SQZ_border_top_color, color);
        ctx_set_color (mrg->ctx, SQZ_border_left_color, color);
        ctx_set_color (mrg->ctx, SQZ_border_right_color, color);
        ctx_set_color (mrg->ctx, SQZ_border_bottom_color, color);
        ctx_color_free (color);
      }
      break;
    case SQZ_border:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word);
                if ((word[0] >= '0' && word[0]<='9') || word[0] == '.')
                {
                  float valf = mrg_parse_px_y (mrg, word, NULL);
                  SET_PROP(border_top_width, valf);
                  SET_PROP(border_bottom_width, valf);
                  SET_PROP(border_right_width, valf);
                  SET_PROP(border_left_width, valf);
                } else if (word_hash == SQZ_solid ||
                           word_hash == SQZ_dotted ||
                           word_hash == SQZ_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, SQZ_border_top_color, color);
                  ctx_set_color (mrg->ctx, SQZ_border_left_color, color);
                  ctx_set_color (mrg->ctx, SQZ_border_right_color, color);
                  ctx_set_color (mrg->ctx, SQZ_border_bottom_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
    case SQZ_border_bottom:
    case SQZ_border_left:
    case SQZ_border_right:
    case SQZ_border_top:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word);
                if ((word[0] >= '0' && word[0]<='9') || (word[0] == '.'))
                {
                  float valf = mrg_parse_px_x (mrg, word, NULL);
		  if (key == SQZ_border_bottom)
                    SET_PROP(border_bottom_width, valf);
		  else if (key == SQZ_border_left)
                    SET_PROP(border_left_width, valf);
		  else if (key == SQZ_border_right)
                    SET_PROP(border_right_width, valf);
		  else
                    SET_PROP(border_top_width, valf);
                } else if (word_hash == SQZ_solid ||
                           word_hash == SQZ_dotted ||
                           word_hash == SQZ_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
		  if (key == SQZ_border_bottom)
                    ctx_set_color (mrg->ctx, SQZ_border_bottom_color, color);
		  else if (key == SQZ_border_top)
                    ctx_set_color (mrg->ctx, SQZ_border_top_color, color);
		  else if (key == SQZ_border_left)
                    ctx_set_color (mrg->ctx, SQZ_border_left_color, color);
		  else if (key == SQZ_border_right)
                    ctx_set_color (mrg->ctx, SQZ_border_right_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
  
    case SQZ_background_color:
    case SQZ_background:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, SQZ_background_color, color);
      ctx_color_free (color);
    }
      break;
    case SQZ_fill_color:
    case SQZ_fill:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, SQZ_fill_color, color);
      ctx_color_free (color);
    }
      break;
    case SQZ_stroke_color:
    case SQZ_stroke:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, SQZ_stroke_color, color);
      ctx_color_free (color);
    }
      break;
    case SQZ_text_stroke_color:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, SQZ_text_stroke_color, color);
      ctx_color_free (color);
    }
      break;
    case SQZ_text_stroke:
    {
      char *col = NULL;
      SET_PROP(text_stroke_width, mrg_parse_px_y (mrg, value, &col));
  
      if (col)
      {
        CtxColor *color = ctx_color_new ();
        ctx_color_set_from_string (mrg->ctx, color, col + 1);
        ctx_set_color (mrg->ctx, SQZ_text_stroke_color, color);
        ctx_color_free (color);
      }
    }
      break;
    case SQZ_opacity:
    {
      ctx_global_alpha (mrg->ctx, _ctx_parse_float (value, NULL));
      SET_PROP(opacity, _ctx_parse_float (value, NULL));
    }
    break;
    case SQZ_z_index:
      s->z_index = _ctx_parse_float (value, NULL);
    break;
    case SQZ_print_symbols:
    switch (val_hash)
    {
        case SQZ_true:
        case SQZ_1:
        case SQZ_yes:
          s->print_symbols = 1;
          break;
        default:
          s->print_symbols = 0;
    }
      break;
    case SQZ_font_weight:
      switch (val_hash)
      {
        case SQZ_bold:
        case SQZ_bolder:
          s->text_decoration |= CTX_TEXT_DECORATION_BOLD;
          s->font_weight = CTX_FONT_WEIGHT_BOLD;
          break;
        default:
          s->text_decoration ^= (s->text_decoration & CTX_TEXT_DECORATION_BOLD);
          s->font_weight = CTX_FONT_WEIGHT_NORMAL;
      }
  #if 0 // XXX 
        cairo_select_font_face (mrg_ctx (mrg),
            s->font_family,
            s->font_style,
            s->font_weight);
  #endif
      break;
    case SQZ_white_space:
      {
        switch (val_hash)
        {
          default:
          case SQZ_normal:   s->white_space = CTX_WHITE_SPACE_NORMAL; break;
          case SQZ_nowrap:   s->white_space = CTX_WHITE_SPACE_NOWRAP; break;
          case SQZ_pre:      s->white_space = CTX_WHITE_SPACE_PRE; break;
          case SQZ_pre_line: s->white_space = CTX_WHITE_SPACE_PRE_LINE; break;
          case SQZ_pre_wrap: s->white_space = CTX_WHITE_SPACE_PRE_WRAP; break;
        }
      }
      break;
    case SQZ_box_sizing:
      {
        if (val_hash == SQZ_border_box)
        {
          s->box_sizing = CTX_BOX_SIZING_BORDER_BOX;
          s->box_sizing = CTX_BOX_SIZING_CONTENT_BOX;
        }
      }
      break;
    case SQZ_float:
      switch (val_hash)
      {
        case SQZ_left:  s->float_ = CTX_FLOAT_LEFT;  break;
        case SQZ_right: s->float_ = CTX_FLOAT_RIGHT; break;
        default:        s->float_ = CTX_FLOAT_NONE;
      }
      break;
    case SQZ_overflow:
      switch(val_hash)
      {
        case SQZ_visible: s->overflow = CTX_OVERFLOW_VISIBLE; break;
        case SQZ_hidden:  s->overflow = CTX_OVERFLOW_HIDDEN;  break;
        case SQZ_scroll:  s->overflow = CTX_OVERFLOW_SCROLL;  break;
        case SQZ_auto:    s->overflow = CTX_OVERFLOW_AUTO;    break;
        default:          s->overflow = CTX_OVERFLOW_VISIBLE; break;
      }
      break;
    case SQZ_clear:
      switch(val_hash)
      {
        case SQZ_left:  s->clear = CTX_CLEAR_LEFT;  break;
        case SQZ_right: s->clear = CTX_CLEAR_RIGHT; break;
        case SQZ_both:  s->clear = CTX_CLEAR_BOTH;  break;
        default:        s->clear = CTX_CLEAR_NONE;  break;
      }
      break;
    case SQZ_font_style:
      switch(val_hash)
      {
        case SQZ_italic:  s->font_style = CTX_FONT_STYLE_ITALIC;  break;
        case SQZ_oblique: s->font_style = CTX_FONT_STYLE_OBLIQUE; break;
        default:          s->font_style = CTX_FONT_STYLE_NORMAL;
  #if 0 // XXX
        cairo_select_font_face (mrg_ctx (mrg),
            s->font_family,
            s->font_style,
            s->font_weight);
  #endif
      }
      break;
    case SQZ_font_family:
      {
        SET_PROPS(font_family, value);
        ctx_font (mrg_ctx (mrg), value);
      }
      break;
    case SQZ_syntax_highlight:
      SET_PROPS(syntax_highlight, value);
      break;
    case SQZ_fill_rule:
      switch (val_hash)
      { 
        default:
        case  SQZ_evenodd: s->fill_rule = CTX_FILL_RULE_EVEN_ODD; break;
        case  SQZ_nonzero: s->fill_rule = CTX_FILL_RULE_WINDING;  break;
      }
      if (s->fill_rule == CTX_FILL_RULE_EVEN_ODD)
        ctx_fill_rule (mrg_ctx (mrg), CTX_FILL_RULE_EVEN_ODD);
      else
        ctx_fill_rule (mrg_ctx (mrg), CTX_FILL_RULE_WINDING);
      break;
    case SQZ_stroke_linejoin:
      switch (val_hash)
      { 
        case SQZ_miter: s->stroke_linejoin = CTX_JOIN_MITER; break;
        case SQZ_round: s->stroke_linejoin = CTX_JOIN_ROUND; break;
        case SQZ_bevel: s->stroke_linejoin = CTX_JOIN_BEVEL; break;
        default:        s->stroke_linejoin = CTX_JOIN_MITER;
      }
      ctx_line_join (mrg_ctx (mrg), (CtxLineJoin)s->stroke_linejoin);
      break;
    case SQZ_stroke_linecap:
      switch (val_hash)
      { 
        case  SQZ_butt:   s->stroke_linecap = CTX_CAP_NONE;   break;
        case  SQZ_round:  s->stroke_linecap = CTX_CAP_ROUND;  break;
        case  SQZ_square: s->stroke_linecap = CTX_CAP_SQUARE; break;
        default:          s->stroke_linecap = CTX_CAP_NONE;
      }
      ctx_line_cap (mrg_ctx (mrg), s->stroke_linecap);
      break;
    case SQZ_vertical_align:
      switch (val_hash)
      {
        case SQZ_middle: s->vertical_align = CTX_VERTICAL_ALIGN_MIDDLE; break;
        case SQZ_top:    s->vertical_align = CTX_VERTICAL_ALIGN_TOP;    break;
        case SQZ_sub:    s->vertical_align = CTX_VERTICAL_ALIGN_SUB;    break;
        case SQZ_super:  s->vertical_align = CTX_VERTICAL_ALIGN_SUPER;  break;
        case SQZ_bottom: s->vertical_align = CTX_VERTICAL_ALIGN_BOTTOM; break;
        default:         s->vertical_align = CTX_VERTICAL_ALIGN_BASELINE;
      }
      break;
    case SQZ_cursor:
      switch (val_hash)
      {
        default:
        case SQZ_default:       s->cursor = MRG_CURSOR_DEFAULT;      break;
        case SQZ_auto:          s->cursor = MRG_CURSOR_AUTO;         break;
        case SQZ_alias:         s->cursor = MRG_CURSOR_ALIAS;        break;
        case SQZ_all_scroll:    s->cursor = MRG_CURSOR_ALL_SCROLL;   break;
        case SQZ_cell:          s->cursor = MRG_CURSOR_CELL;         break;
        case SQZ_context_menu:  s->cursor = MRG_CURSOR_CONTEXT_MENU; break;
        case SQZ_col_resize:    s->cursor = MRG_CURSOR_COL_RESIZE;   break;
        case SQZ_copy:          s->cursor = MRG_CURSOR_COPY;         break;
        case SQZ_crosshair:     s->cursor = MRG_CURSOR_CROSSHAIR;    break;
        case SQZ_e_resize:      s->cursor = MRG_CURSOR_E_RESIZE;     break;
        case SQZ_ew_resize:     s->cursor = MRG_CURSOR_EW_RESIZE;    break;
        case SQZ_help:          s->cursor = MRG_CURSOR_HELP;         break;
        case SQZ_move:          s->cursor = MRG_CURSOR_MOVE;         break;
        case SQZ_n_resize:      s->cursor = MRG_CURSOR_N_RESIZE;     break;
        case SQZ_ne_resize:     s->cursor = MRG_CURSOR_NE_RESIZE;    break;
        case SQZ_nesw_resize:   s->cursor = MRG_CURSOR_NESW_RESIZE;  break;
        case SQZ_ns_resize:     s->cursor = MRG_CURSOR_NS_RESIZE;    break;
        case SQZ_nw_resize:     s->cursor = MRG_CURSOR_NW_RESIZE;    break;
        case SQZ_no_drop:       s->cursor = MRG_CURSOR_NO_DROP;      break;
        case SQZ_none:          s->cursor = MRG_CURSOR_NONE;         break;
        case SQZ_not_allowed:   s->cursor = MRG_CURSOR_NOT_ALLOWED;  break;
        case SQZ_pointer:       s->cursor = MRG_CURSOR_POINTER;      break;
        case SQZ_progress:      s->cursor = MRG_CURSOR_PROGRESS;     break;
        case SQZ_row_resize:    s->cursor = MRG_CURSOR_ROW_RESIZE;   break;
        case SQZ_s_resize:      s->cursor = MRG_CURSOR_S_RESIZE;     break;
        case SQZ_se_resize:     s->cursor = MRG_CURSOR_SE_RESIZE;    break;
        case SQZ_sw_resize:     s->cursor = MRG_CURSOR_SW_RESIZE;    break;
        case SQZ_text:          s->cursor = MRG_CURSOR_TEXT;         break;
        case SQZ_vertical_text: s->cursor = MRG_CURSOR_VERTICAL_TEXT;break;
        case SQZ_w_resize:      s->cursor = MRG_CURSOR_W_RESIZE;     break;
        case SQZ_cursor_wait:   s->cursor = MRG_CURSOR_WAIT;         break;
        case SQZ_zoom_in:       s->cursor = MRG_CURSOR_ZOOM_IN;      break;
        case SQZ_zoom_out:      s->cursor = MRG_CURSOR_ZOOM_OUT;     break;
      }
      break;
    case SQZ_display:
      switch (val_hash)
      {
        case SQZ_none:
        case SQZ_hidden:       s->display = CTX_DISPLAY_NONE;         break;
        case SQZ_block:        s->display = CTX_DISPLAY_BLOCK;        break;
        case SQZ_list_item:    s->display = CTX_DISPLAY_LIST_ITEM;    break;
        case SQZ_inline_block: s->display = CTX_DISPLAY_INLINE_BLOCK; break;
	case SQZ_flow_root:    s->display = CTX_DISPLAY_FLOW_ROOT;    break;
        default:               s->display = CTX_DISPLAY_INLINE;
      }
      break;
    case SQZ_position:
      switch (val_hash)
      {
        case SQZ_relative:  s->position = CTX_POSITION_RELATIVE; break;
        case SQZ_static:    s->position = CTX_POSITION_STATIC;   break;
        case SQZ_absolute:  s->position = CTX_POSITION_ABSOLUTE; break;
        case SQZ_fixed:     s->position = CTX_POSITION_FIXED;    break;
        default:            s->position = CTX_POSITION_STATIC;
      }
      break;
    case SQZ_direction:
      switch (val_hash)
      {
        case SQZ_rtl: s->direction = CTX_TEXT_DIRECTION_RTL; break;
        case SQZ_ltr: s->direction = CTX_TEXT_DIRECTION_LTR; break;
        default:      s->direction = CTX_TEXT_DIRECTION_LTR;
      }
      break;
    case SQZ_unicode_bidi:
      switch (val_hash)
      {
        case SQZ_normal: s->unicode_bidi = CTX_UNICODE_BIDI_NORMAL; break;
        case SQZ_embed:  s->unicode_bidi = CTX_UNICODE_BIDI_EMBED;  break;
        case SQZ_bidi_override: s->unicode_bidi = CTX_UNICODE_BIDI_BIDI_OVERRIDE; break;
        default:         s->unicode_bidi = CTX_UNICODE_BIDI_NORMAL; break;
      }
      break;
    case SQZ_text_align:
      switch (val_hash)
      {
        case SQZ_start:   s->text_align = CTX_TEXT_ALIGN_START;   break;
        case SQZ_end:     s->text_align = CTX_TEXT_ALIGN_END;     break;
        case SQZ_left:    s->text_align = CTX_TEXT_ALIGN_LEFT;    break;
        case SQZ_right:   s->text_align = CTX_TEXT_ALIGN_RIGHT;   break;
        case SQZ_justify: s->text_align = CTX_TEXT_ALIGN_JUSTIFY; break;
        case SQZ_center:  s->text_align = CTX_TEXT_ALIGN_CENTER;  break;
        default:          s->text_align = CTX_TEXT_ALIGN_LEFT;
      }
      break;
    case SQZ_text_decoration:
      switch (val_hash)
      {
        case SQZ_reverse:     s->text_decoration|= CTX_TEXT_DECORATION_REVERSE; break;
        case SQZ_underline:   s->text_decoration|= CTX_TEXT_DECORATION_UNDERLINE; break;
        case SQZ_overline:    s->text_decoration|= CTX_TEXT_DECORATION_OVERLINE; break;
        case SQZ_linethrough: s->text_decoration|= CTX_TEXT_DECORATION_LINETHROUGH; break;
        case SQZ_blink:       s->text_decoration|= CTX_TEXT_DECORATION_BLINK; break;
        case SQZ_none:
          s->text_decoration ^= (s->text_decoration &
        (CTX_TEXT_DECORATION_UNDERLINE|CTX_TEXT_DECORATION_REVERSE|CTX_TEXT_DECORATION_OVERLINE|CTX_TEXT_DECORATION_LINETHROUGH|CTX_TEXT_DECORATION_BLINK));
        break;
      }
      break;
  }
}

static void ctx_css_handle_property_pass1med (Mrg *mrg, uint32_t key,
                                              const char *value)
{
  CtxStyle *s = ctx_style (mrg);

  if (key == SQZ_width)
  {
    if (ctx_strhash (value) == SQZ_auto)
    {
      s->width_auto = 1;
      SET_PROP(width, 42);
    }
    else
    {
      s->width_auto = 0;
      SET_PROP(width, mrg_parse_px_x (mrg, value, NULL));

      if (s->position == CTX_POSITION_FIXED) // XXX: seems wrong
      {
        //s->width -= s->border_left_width + s->border_right_width;
      }
    }
  }
}

enum
{
  MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL = 0,
  MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME,
  MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON,
  MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL,
  MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL
};

static void css_parse_properties (Mrg *mrg, const char *style,
  void (*handle_property) (Mrg *mrg, uint32_t key,
                           const char *value))
{
  const char *p;
  char name[CTX_MAX_CSS_STRINGLEN] = "";
  char string[CTX_MAX_CSS_STRINGLEN] = "";
  int name_l = 0;
  int string_l = 0;
  int state = MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL;
  if (!style)
    return;
  for (p = style; *p; p++)
  {
    switch (state)
    {
      case MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL:
        switch (*p)
        {
          case ' ':
          case '\t':
          case ';':
          case '\n':
          case '\r':
            break;
          default:
            name[name_l++]=*p;
            name[name_l]=0;
            state = MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME:
        switch (*p)
        {
          case ':':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL;
            break;
          case ' ':
          case '\n':
          case '\r':
          case '\t':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON;
            break;
          default:
            name[name_l++]=*p;
            name[name_l]=0;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON:
        switch (*p)
        {
          case ':':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL;
            break;
          default:
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL:
        switch (*p)
        {
          case ' ':
          case '\n':
          case '\r':
          case '\t':
            break;
          default:
            string[string_l++]=*p;
            string[string_l]=0;
            state = MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL:
        switch (*p)
        {
          case ';':
            handle_property (mrg, ctx_strhash (name), string);
            state = MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL;
            name_l = 0;
            name[0] = 0;
            string_l = 0;
            string[0] = 0;
            break;
          default:
            string[string_l++]=*p;
            string[string_l]=0;
            break;
        }
        break;
    }
  }
  if (name[0])
  handle_property (mrg, ctx_strhash (name), string);
}


static void ctx_css_handle_property_pass2 (Mrg *mrg, uint32_t key,
                                           const char *value)
{
  /* this pass contains things that might depend on values
   * generated by the previous pass.
   */
  CtxStyle *s = ctx_style (mrg);

  if (key == SQZ_right)
  {
    float width = PROP(width);
    float right = mrg_parse_px_x (mrg, value, NULL);

    SET_PROP(right, right);

    if (width == 0)
    {
      width = 8 * s->font_size;
      mrg_queue_draw (mrg, NULL);
    }
    SET_PROP(right, right);
    SET_PROP(left,
         (mrg_width(mrg)-right) - width - PROP(border_left_width) - PROP(padding_left) - PROP(padding_right) - PROP(border_right_width) - PROP(margin_right));
  }
  else if (key == SQZ_bottom)
  {
    float height = PROP(height);

    SET_PROP (bottom, mrg_parse_px_y (mrg, value, NULL));

    if (height == 0)
    {
            // XXX
      height = 2 * s->font_size;
      mrg_queue_draw (mrg, NULL);
    }
    SET_PROP(top, mrg_height(mrg) - PROP(bottom) - height - PROP(padding_top) - PROP(border_top_width) - PROP(padding_bottom) - PROP(border_bottom_width) - PROP(margin_bottom));
  }
}


static float deco_width (Mrg *mrg)
{
  return PROP (padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width);
}

void itk_set_style (Mrg *mrg, const char *style)
{
  CtxStyle *s;

  css_parse_properties (mrg, style, ctx_css_handle_property_pass0);
  css_parse_properties (mrg, style, ctx_css_handle_property_pass1);
  css_parse_properties (mrg, style, ctx_css_handle_property_pass1med);

  s = ctx_style (mrg);

  if (s->position == CTX_POSITION_STATIC &&
      !s->float_)
  {
    if (s->width_auto && (s->margin_right_auto || s->margin_left_auto))
    {
      if (s->margin_left_auto && s->margin_right_auto)
      {
        s->margin_right_auto = 0;
      }
      else if (s->margin_left_auto)
        s->margin_left_auto = 0;
      else
        s->margin_right_auto = 0;
    }

    if ( s->margin_left_auto && !s->width_auto && !s->margin_right_auto)
    {
      SET_PROP (margin_left,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_right) - PROP(width));
    }
    else if ( !s->margin_left_auto &&
              s->width_auto &&
              !s->margin_right_auto)
    {
      SET_PROP (width,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_left) - PROP(margin_right));
    }
    else if ( !s->margin_left_auto && !s->width_auto && s->margin_right_auto)
    {
      SET_PROP (margin_right,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_left) - PROP(width));
    }
    else if ( s->margin_left_auto && !s->width_auto && s->margin_right_auto)
    {
      float avail_width = (mrg->state->edge_right - mrg->state->edge_left) - deco_width (mrg);
      float width = PROP(width);
      if (width == 0.0f)
      {
	 float max_width = PROP(max_width);
	 float min_width = PROP(min_width);
	 if (max_width != 0.0f)
           width = ctx_minf (max_width, avail_width);
	 else
	   width = avail_width; // should not happen?
	 if (min_width != 0.0f)
	 {
	   if (width < min_width) width = min_width;
	 }

      }

      {
        float val = (avail_width - width)/2;
        SET_PROP (margin_left, val);
        SET_PROP (margin_right, val);
      }
    }
  }
  css_parse_properties (mrg, style, ctx_css_handle_property_pass2);
}

void _itk_set_style_properties (Mrg *mrg, const char *style_properties)
{
  _ctx_initial_style (mrg);

  if (style_properties)
  {
    itk_set_style (mrg, style_properties);
  }
}

void
itk_set_stylef (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  itk_set_style (mrg, buffer);
  free (buffer);
}

void ctx_style_defaults (Mrg *mrg)
{
  Ctx *ctx = mrg->ctx;
  float em = 32;
  mrg_set_em (mrg,  em);
  mrg_set_rem (mrg, em);
  itk_set_edge_left (mrg, 0);
  itk_set_edge_right (mrg, mrg_width (mrg));
  itk_set_edge_bottom (mrg, mrg_height (mrg));
  itk_set_edge_top (mrg, 0);
  mrg_set_line_height (mrg, 1.2);

  SET_PROP(stroke_width, 1.0f);
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, "transparent");
    ctx_set_color (ctx, SQZ_stroke_color, color);
    ctx_color_free (color);
  }
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, "black");
    ctx_set_color (ctx, SQZ_fill_color, color);
    ctx_color_free (color);
  }
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, "green");
    ctx_set_color (ctx, SQZ_color, color);
    ctx_color_free (color);
  }

  itk_stylesheet_clear (mrg);
  _ctx_initial_style (mrg);

  if (mrg->style_global->length)
  {
    ctx_stylesheet_add (mrg, mrg->style_global->str, NULL, CTX_STYLE_GLOBAL, NULL);
  }

  if (mrg->style->length)
    ctx_stylesheet_add (mrg, mrg->style->str, NULL, CTX_STYLE_GLOBAL, NULL);
}

void mrg_set_mrg_get_contents (Mrg *mrg,
  int (*mrg_get_contents) (const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length,
                      void        *get_contents_data),
  void *get_contents_data)

{
  mrg->mrg_get_contents = mrg_get_contents;
  mrg->get_contents_data = get_contents_data;
}

int
mrg_get_contents (Mrg         *mrg,
                  const char  *referer,
                  const char  *input_uri,
                  char       **contents,
                  long        *length)
{
  if (!referer) referer = mrg->uri_base;
  if (mrg->mrg_get_contents)
  {
    int ret;
    ret = mrg->mrg_get_contents (referer, input_uri, contents, length,
                                 mrg->get_contents_data);
    return ret;
  }
  else
  {
    *contents = NULL;
    *length = 0;
    return -1;
  }
}

int itk_print (Mrg *mrg, const char *string);
static int is_block_item (CtxStyle *style)
{
  return ((style->display == CTX_DISPLAY_BLOCK
           ||style->float_
           ||style->display == CTX_DISPLAY_LIST_ITEM
           ||style->display == CTX_DISPLAY_FLOW_ROOT
           ||style->display == CTX_DISPLAY_INLINE_BLOCK
	   ));
}

float mrg_ddpx (Mrg *mrg)
{
  return mrg->ddpx;
}

static void
mrg_ctx_set_source_color (Ctx *ctx, CtxColor *color)
{
   float rgba[4];
   ctx_color_get_rgba (ctx_get_state (ctx), color, rgba);
   ctx_rgba (ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void set_line_height (Ctx *ctx, void *userdata, const char *name, int count, float *x, float *y, float *w, float *h)
{
  float *fptr = (float*)userdata;
  *y = fptr[0];
}

static void _mrg_nl (Mrg *mrg)
{
  float ascent, descent;
  ctx_font_extents (mrg->ctx, &ascent, &descent, NULL);
  mrg->y += mrg->line_max_height[mrg->line_level] * mrg->state->style.line_height;
  float val = mrg->line_max_height[mrg->line_level]* ascent;
  char name[10]="lin_";
  name[3]=mrg->line_level+2;

  ctx_resolve (mrg->ctx, name, set_line_height, &val);
  mrg->line_max_height[mrg->line_level]=0.0f;
  mrg->line_got_baseline[mrg->line_level]=0;

  mrg->x = _mrg_dynamic_edge_left(mrg);
#ifdef SNAP
  float em = mrg_em (mrg);  /* XXX: a global body-line spacing 
                               snap is better grid design */
  mrg->x = ceil (mrg->x / em) * em;
  mrg->y = ceil (mrg->y / em) * em;
#endif

  if (mrg->y >= 
      mrg->state->edge_bottom - PROP(padding_bottom))
  {
    mrg->state->overflowed=1;
  }
}

void _mrg_layout_pre (Mrg *mrg)
{
  CtxStyle *style;
  float dynamic_edge_left, dynamic_edge_right;

  //mrg->state_no++;
  //mrg->state = &mrg->states[mrg->state_no];
  //*mrg->state = mrg->states[mrg->state_no-1];

  style = ctx_style (mrg);

  mrg->state->original_x = mrg_x (mrg);
  mrg->state->original_y = mrg_y (mrg);

  mrg->state->flow_root = (style->display == CTX_DISPLAY_FLOW_ROOT ||
                            style->float_ != CTX_FLOAT_NONE ||
                           style->display == CTX_DISPLAY_INLINE_BLOCK ||
                          style->overflow != CTX_OVERFLOW_VISIBLE);
  if (mrg->state->flow_root)
    mrg->state->float_base = mrg->floats;

  // newline hacks
  if (mrg->state->style_node.element_hash == SQZ_br
      || ( mrg->unresolved_line && is_block_item (style))
		  )
  {
    _mrg_nl (mrg);
  }

  if (is_block_item (style))
  {
     mrg->line_level++;
     mrg->line_max_height[mrg->line_level] = 0.0f;
     mrg->line_got_baseline[mrg->line_level]=0;


  }
     mrg->unresolved_line = 0;



  if (mrg->state_no)
  {
    dynamic_edge_right = _mrg_parent_dynamic_edge_right(mrg);
    dynamic_edge_left  = _mrg_parent_dynamic_edge_left(mrg);
  }
  else
  {
    dynamic_edge_right = mrg_edge_right(mrg);
    dynamic_edge_left  = mrg_edge_left(mrg);
  }

  if (style->clear & CTX_CLEAR_RIGHT)
    clear_right (mrg);
  if (style->clear & CTX_CLEAR_LEFT)
    clear_left (mrg);

  if (style->display == CTX_DISPLAY_BLOCK ||
      style->display == CTX_DISPLAY_FLOW_ROOT ||
      style->display == CTX_DISPLAY_LIST_ITEM)
  {
    if (PROP(padding_left) + PROP(margin_left) + PROP(border_left_width)
        != 0)
    {
      itk_set_edge_left (mrg, mrg_edge_left (mrg) +
        PROP(padding_left) + PROP(margin_left) + PROP(border_left_width));
    }
    if (PROP(padding_right) + PROP(margin_right) + PROP(border_right_width)
        != 0)
    {
      itk_set_edge_right (mrg, mrg_edge_right (mrg) -
        (PROP(padding_right) + PROP(margin_right) + PROP(border_right_width)));
    }


    /* collapsing of vertical margins */
    float actual_top = PROP(margin_top) ;
    if (actual_top >= mrg->state->vmarg)
      actual_top = actual_top - mrg->state->vmarg;
    else
      actual_top = 0;

    itk_set_edge_top (mrg, mrg_y (mrg) + PROP(border_top_width) + PROP(padding_top) + actual_top);

    mrg->state->block_start_x = mrg_edge_left (mrg);
    mrg->state->block_start_y = mrg_y (mrg);
 // mrg->floats = 0;
  }
  else if (style->display == CTX_DISPLAY_INLINE_BLOCK)
  {


    float left_margin_pad_and_border = PROP(padding_left) + PROP(margin_left) + PROP(border_left_width);
    float right_margin_pad_and_border = PROP(padding_right) + PROP(margin_right) + PROP(border_right_width);

    if (mrg_x (mrg) + PROP(width) + left_margin_pad_and_border + right_margin_pad_and_border
		    >= dynamic_edge_right)
       _mrg_nl (mrg);

    if (left_margin_pad_and_border != 0.0f)
    {
      itk_set_edge_left (mrg, mrg_x (mrg) +
        PROP(padding_left) + PROP(margin_left) + PROP(border_left_width));
    }


#if 0
    if (right_margin_pad_and_border != 0.0f)
    {
      itk_set_edge_right (mrg, mrg_edge_right (mrg) - right_margin_pad_and_border);
    }
#else
      itk_set_edge_right (mrg, mrg_x (mrg) + PROP(width));
#endif

    itk_set_edge_top (mrg, mrg_y (mrg) + PROP(border_top_width));// + actual_top);

    mrg->state->block_start_x = mrg_x (mrg);
    mrg->state->block_start_y = mrg_y (mrg);
 //   mrg->floats = 0;
  }
	  

  /* the list-item is a cheap hack; can be implemented directly
   * in later versions of css
   */
  if (style->display == CTX_DISPLAY_LIST_ITEM)
  {
    float x = mrg->x;
    
    mrg->x -= mrg_em (mrg) * 0.5;
    itk_print (mrg, "•");
    mrg->x = x;
    mrg->line_max_height[mrg->line_level] = 0.0f;
    mrg->line_got_baseline[mrg->line_level]=0;
  }

  switch (style->position)
  {
    case CTX_POSITION_RELATIVE:
      /* XXX: deal with style->right and style->bottom */
      ctx_translate (mrg_ctx (mrg), PROP(left), PROP(top));
      mrg->relative_x += PROP(left);
      mrg->relative_y += PROP(top);
      /* fallthrough */

    case CTX_POSITION_STATIC:

      if (style->float_ == CTX_FLOAT_RIGHT)
      {
        float width = PROP(width);

        if (width == 0.0)
        {
          width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + PROP(padding_right) + PROP(padding_left) + PROP(border_left_width) + PROP(border_right_width));


        if (width + PROP(margin_left) + PROP(margin_right) >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (mrg);
          itk_set_edge_left (mrg, mrg_edge_right (mrg) - width);
          itk_set_edge_right (mrg, mrg_edge_right (mrg) - (PROP(margin_right) + PROP(padding_right) + PROP(border_right_width)));

        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + PROP(margin_left) + PROP(margin_right))
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(mrg);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(mrg);
        }

        itk_set_edge_left (mrg, dynamic_edge_right - width);
        itk_set_edge_right (mrg, dynamic_edge_right - (PROP(margin_right) + PROP(padding_right) + PROP(border_right_width)));

        }

        itk_set_edge_top (mrg, mrg_y (mrg) + (PROP(margin_top))); // - mrg->state->vmarg));

        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
        //mrg->floats = 0;

      } else if (style->float_ == CTX_FLOAT_LEFT)
      {
        float left;
        float width = PROP(width);

        if (width == 0.0)
        {
          width = 4 * mrg_em (mrg);//mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + PROP(padding_right) + PROP(padding_left) + PROP(border_left_width) + PROP(border_right_width));

        if (width + PROP(margin_left) + PROP(margin_right) >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (mrg);
          left = mrg_edge_left (mrg) + PROP(padding_left) + PROP(border_left_width) + PROP(margin_left);
        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + PROP(margin_left) + PROP(margin_right))
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(mrg);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(mrg);
        }
          left = dynamic_edge_left;// + PROP(padding_left) + PROP(border_left_width) + PROP(margin_left);
        }

        itk_set_edge_left (mrg, left);
        itk_set_edge_right (mrg,  left + width +
            PROP(padding_left) /* + PROP(border_right_width)*/);
        itk_set_edge_top (mrg, mrg_y (mrg) + (PROP(margin_top)));// - mrg->state->vmarg));
                        //));//- mrg->state->vmarg));
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);// + PROP(padding_top) + PROP(border_top_width);
        //mrg->floats = 0;

        /* change cursor point after floating something left; if pushed far
         * down, the new correct
         */
#if 0
        if(0)	
        mrg_set_xy (mrg, mrg->state->original_x = left + width + PROP(padding_left) + PROP(border_right_width) + PROP(padding_right) + PROP(margin_right) + PROP(margin_left) + PROP(border_left_width),
            mrg_y (mrg) + PROP(padding_top) + PROP(border_top_width));
#endif
      } /* XXX: maybe spot for */
      else if (1)
      {
         float width = PROP(width);
         if (width)
           itk_set_edge_right (mrg, mrg->state->block_start_x  + width);
      }
      break;
    case CTX_POSITION_ABSOLUTE:
      ctx_get_drawlist (mrg->ctx, &mrg->state->drawlist_start_offset);
      mrg->state->drawlist_start_offset--;
      {
	float left = PROP(left);
	float top = PROP(top);

        if (left == 0.0f) // XXX 0.0 should also be a valid value!
	  left = mrg->x;
        if (top == 0.0f)  // XXX 0.0 should also be a valid value!
	  top = mrg->y;

        //mrg->floats = 0;
        itk_set_edge_left (mrg, left + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left));
        itk_set_edge_right (mrg, left + PROP(width));
        itk_set_edge_top (mrg, top + PROP(margin_top) + PROP(border_top_width) + PROP(padding_top));
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
      }
      break;
    case CTX_POSITION_FIXED:
      ctx_get_drawlist (mrg->ctx, &mrg->state->drawlist_start_offset);
      mrg->state->drawlist_start_offset--;
      {
        int width = PROP(width);

        if (!width)
        {
          width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

	ctx_translate (mrg_ctx(mrg), 0, itk_panel_scroll (mrg));
        ctx_scale (mrg_ctx(mrg), mrg_ddpx (mrg), mrg_ddpx (mrg));
        //mrg->floats = 0;

        itk_set_edge_left (mrg, PROP(left) + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left));
        itk_set_edge_right (mrg, PROP(left) + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left) + width);//mrg_width (mrg) - PROP(padding_right) - PROP(border_right_width) - PROP(margin_right)); //PROP(left) + PROP(width)); /* why only padding and not also border?  */
        itk_set_edge_top (mrg, PROP(top) + PROP(margin_top) + PROP(border_top_width) + PROP(padding_top));
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
      }
      break;
  }

  if (is_block_item (style))
  {
     float height = PROP(height);
     float width = PROP(width);
     

     if (!height)
       {
         height = mrg_em (mrg) * 4;
       }
     if (!width)
       {
         width = mrg_em (mrg) * 4;
       }

    if (height  /* XXX: if we knew height of dynamic elements
                        from previous frame, we could use it here */
       && style->overflow == CTX_OVERFLOW_HIDDEN)
       {
         ctx_rectangle (mrg_ctx(mrg),
            mrg->state->block_start_x - PROP(padding_left) - PROP(border_left_width),
            mrg->state->block_start_y - mrg_em(mrg) - PROP(padding_top) - PROP(border_top_width),
            width + PROP(border_right_width) + PROP(border_left_width) + PROP(padding_left) + PROP(padding_right), //mrg_edge_right (mrg) - mrg_edge_left (mrg) + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width),
            height + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width));
         ctx_clip (mrg_ctx(mrg));
       }

    mrg->state->ptly = 0;
    char name[10]="ele_";
    name[3]=mrg->state_no+2;

  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (mrg->ctx, SQZ_background_color, background_color);
  if (!ctx_color_is_transparent (background_color))
  {
    mrg_ctx_set_source_color (mrg->ctx, background_color);
    if (is_block_item (style))
    {
      ctx_begin_path (mrg->ctx); // XXX : this should not need be here!
      ctx_deferred_rectangle (mrg->ctx, name,
         mrg->state->block_start_x - PROP(padding_left),
         mrg->state->block_start_y - PROP(padding_top),
         width + PROP(padding_left) + PROP(padding_right),
         height + PROP(padding_top) + PROP(padding_bottom));
    }
    else
    {
      ctx_deferred_rectangle (mrg->ctx, name,
         mrg_x (mrg), mrg_y (mrg),
         width  + PROP(padding_left) + PROP(padding_right),
         height + PROP(padding_top) + PROP(padding_bottom));
    }
    ctx_fill (mrg->ctx);
  }
  ctx_color_free (background_color);
  }
}
void _mrg_layout_post (Mrg *mrg, CtxFloatRectangle *ret_rect);

void itk_set_style (Mrg *mrg, const char *style);

void itk_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style)
{
  mrg->states[mrg->state_no].children++;
  if (mrg->state_no+1 >= CTX_MAX_STATES)
    return;
  mrg->state_no++; // XXX bounds check!
  mrg->state = &mrg->states[mrg->state_no];
  *mrg->state = mrg->states[mrg->state_no-1];
  mrg->states[mrg->state_no].children = 0;

  mrg->state->style_id = style_id ? strdup (style_id) : NULL;

  ctx_parse_style_id (mrg, mrg->state->style_id, &mrg->state->style_node);

  mrg->state->style.id_ptr = id_ptr;

  ctx_save (mrg_ctx (mrg));

  _ctx_initial_style (mrg);

  {
    char *collated_style = _ctx_stylesheet_collate_style (mrg);
    if (collated_style)
    {
      itk_set_style (mrg, collated_style);
      free (collated_style);
    }
  }
  if (style)
  {
    itk_set_style (mrg, style);
  }
  _mrg_layout_pre (mrg);
}

void
itk_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                       const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  itk_start_with_style (mrg, style_id, id_ptr, buffer);
  free (buffer);
}

void itk_start (Mrg *mrg, const char *style_id, void *id_ptr)
{
  itk_start_with_style (mrg, style_id, id_ptr, NULL);
}

static int compare_zindex (const void *a, const void *b, void *d)
{
  const MrgAbsolute *ma = a;
  const MrgAbsolute *mb = b;
  return mb->z_index- ma->z_index;
}

void itk_end (Mrg *mrg, CtxFloatRectangle *ret_rect)
{
  _mrg_layout_post (mrg, ret_rect);
  if (mrg->state_no == 0)
  {
    ctx_list_reverse (&mrg->absolutes);
    ctx_list_sort (&mrg->absolutes, compare_zindex, NULL);

    /* TODO: handle negative z-index */
    /* TODO: also copy/paste registered interaction points */
    while (mrg->absolutes)
    {
      MrgAbsolute *absolute = mrg->absolutes->data;
      ctx_save (mrg->ctx);
      ctx_translate (mrg->ctx, absolute->relative_x, absolute->relative_y);
      ctx_append_drawlist (mrg->ctx, absolute->entries+1, (absolute->count-1)*9);
      ctx_list_remove (&mrg->absolutes, absolute);
      free (absolute);
    }
  }
}

void  mrg_set_line_height (Mrg *mrg, float line_height);
float mrg_line_height (Mrg *mrg);


static void mrg_path_fill_stroke (Mrg *mrg)
{
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *fill_color = ctx_color_new ();
  CtxColor *stroke_color = ctx_color_new ();

  ctx_get_color (ctx, SQZ_fill_color, fill_color);
  ctx_get_color (ctx, SQZ_stroke_color, stroke_color);

  if (!ctx_color_is_transparent (fill_color))
  {
    mrg_ctx_set_source_color (ctx, fill_color);
    if (PROP(stroke_width) > 0.001f && !ctx_color_is_transparent (stroke_color))
      ctx_preserve (ctx);
    ctx_fill (ctx);
  }

  if (PROP(stroke_width) > 0.001f && !ctx_color_is_transparent (stroke_color))
  {
    ctx_line_width (ctx, PROP(stroke_width));
    mrg_ctx_set_source_color (ctx, stroke_color);
    ctx_stroke (ctx);
  }
  ctx_color_free (fill_color);
  ctx_color_free (stroke_color);
}

void _mrg_border_top (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);

  if (PROP(border_top_width) < 0.01f)
    return;

  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width), 0);
    ctx_rel_line_to (ctx, -PROP(border_right_width), PROP(border_top_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_right) + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}

void _mrg_border_bottom (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_bottom_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *cr = mrg_ctx (mrg);
  if (PROP(border_top_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (cr, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (cr);
    ctx_begin_path (cr);
    ctx_move_to (cr, x, y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (cr, width + PROP(padding_right) + PROP(border_right_width), 0);
    ctx_rel_line_to (cr, -PROP(border_right_width), PROP(border_top_width));
    ctx_rel_line_to (cr, - (width + PROP(padding_right)), 0);

    mrg_ctx_set_source_color (cr, color);
    ctx_fill (cr);
  ctx_restore (cr);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_bottom_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_top_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width), 0);
    ctx_rel_line_to (ctx, 0, PROP(border_top_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_bottom_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(border_left_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}


void _mrg_border_top_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_top_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x,
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, PROP(border_top_width));
    ctx_rel_line_to (ctx, -width, 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  float border_bottom_width = PROP(border_bottom_width);
  if ((border_bottom_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
    ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, (border_bottom_width));
    ctx_rel_line_to (ctx, - width, 0);
    ctx_rel_line_to (ctx, 0, -(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }

  ctx_color_free (color);
}
void _mrg_border_left (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_left_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_left_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, PROP(border_left_width), PROP(border_top_width));
    ctx_rel_line_to (ctx, 0, height + PROP(padding_top) + PROP(padding_bottom) );
    ctx_rel_line_to (ctx, -PROP(border_left_width), PROP(border_bottom_width));
    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_right (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_ctx (mrg);
  if (PROP(border_right_width) < 0.01f)
    return;
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_right_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, 0, - (height + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width)));
    ctx_rel_line_to (ctx, -PROP(border_right_width), PROP(border_top_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

static void mrg_box (Mrg *mrg, int x, int y, int width, int height)
{
  _mrg_border_top (mrg, x, y, width, height);
  _mrg_border_left (mrg, x, y, width, height);
  _mrg_border_right (mrg, x, y, width, height);
  _mrg_border_bottom (mrg, x, y, width, height);
}

static void mrg_box_fill (Mrg *mrg, CtxStyle *style, float x, float y, float width, float height)
{
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_background_color, background_color);
  if (ctx_color_is_transparent (background_color))
  {
    ctx_color_free (background_color);
    return;
  }

  height = ctx_floorf (y + height) - ctx_floorf(y);
  y = ctx_floorf (y);

  ctx_save (ctx);
  {
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x,
                       y);
    ctx_rel_line_to (ctx, 0, height );
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, -(height ));

    ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
    mrg_ctx_set_source_color (ctx, background_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
  ctx_color_free (background_color);
}

/*
 *  each style state level needs to know how far down it has
 *  painted background,.. on background increment we do all of them..
 *  .. floats are problematic - maybe draw them in second layer.
 *
 */


#if 0
static void
_mrg_resolve_line_height (Mrg *mrg, void *data, int last)
{
  //MrgState *state = &mrg->states[mrg->state_no];
  //CtxStyle *style = &state->style;
  float ascent, descent;
  ctx_font_extents (mrg->ctx, &ascent, &descent, NULL);

  float val = mrg->line_max_height[mrg->line_level] * ascent;

  char name[10]="lin_";
  name[3]=mrg->line_level+2;

  ctx_resolve (mrg->ctx, name, set_line_height, &val);

  mrg->line_max_height[mrg->line_level] = 0.0f;//style->font_size;//0.0f;
  mrg->line_got_baseline[mrg->line_level]=0;
}
#endif

/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**************/

static float measure_word_width (Mrg *mrg, const char *word)
{
  return ctx_text_width (mrg->ctx, word);
}

const char * hl_punctuation[] =
{";", ",", "(", ")", "{", "}", NULL};
const char * hl_operators [] =
{"-", "+", "=", "*", "/", "return", "<", ">", ":",
 "if", "else", "break", "case", NULL};
const char * hl_types[] =
// XXX anything ending in _t ?
{"int", "uint32_t", "uint64_t", "uint8_t", "Ctx", "cairo_t", "Mrg", "float", "double",
  "char", "const", "static", "void", "NULL",
  "#include", "#define", NULL};

static int is_one_of (const char *word, const char **words)
{
  int i;
  for (i = 0; words[i]; i++)
  {
    if (!strcmp (words[i], word))
      return 1;
  }
  return 0;
}

static int is_a_number (const char *word)
{
  int yep = 1;
  int i;
  for (i = 0; word[i]; i++)
  {
    if ((word[i] < '0' || word[i] > '9') && word[i] != '.')
      yep = 0;
  }
  return yep;
}

/* the syntax highlighting is done with static globals; deep in the text
 * rendering, this permits the editing code to recognize which string is
 * edited and directly work with pointer arithmetic on that instead of
 * marked up xml for the highlighting - it limits the syntax highlighting
 * context ability
 */
enum {
  MRG_HL_NEUTRAL      = 0,
  MRG_HL_NEXT_NEUTRAL = 1,
  MRG_HL_STRING       = 2,
  MRG_HL_STRING_ESC   = 3,
  MRG_HL_QSTRING      = 4,
  MRG_HL_QSTRING_ESC  = 5,
  MRG_HL_SLASH        = 6,
  MRG_HL_LINECOMMENT  = 7,
  MRG_HL_COMMENT      = 8,
  MRG_HL_COMMENT_STAR = 9,
};

static int hl_state_c = MRG_HL_NEUTRAL;

static void mrg_hl_token (Ctx *cr, const char *word)
{
  switch (hl_state_c)
  {
    case MRG_HL_NEUTRAL:
      if (!strcmp (word, "\""))
      {
        hl_state_c = MRG_HL_STRING;
      }
      else if (!strcmp (word, "'"))
      {
        hl_state_c = MRG_HL_QSTRING;
      }
      else if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_SLASH;
      }
      break;
    case MRG_HL_SLASH:
      if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_LINECOMMENT;
      } else if (!strcmp (word, "*"))
      {
        hl_state_c = MRG_HL_COMMENT;
      } else
      {
        hl_state_c = MRG_HL_NEUTRAL;
      }
      break;
    case MRG_HL_LINECOMMENT:
      if (!strcmp (word, "\n"))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      break;
    case MRG_HL_COMMENT:
      if (!strcmp (word, "*"))
      {
        hl_state_c = MRG_HL_COMMENT_STAR;
      }
      break;
    case MRG_HL_COMMENT_STAR:
      if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_NEUTRAL;
      }
      else
      {
        hl_state_c = MRG_HL_COMMENT;
      }
      break;
    case MRG_HL_STRING:
      if (!strcmp (word, "\""))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      else if (!strcmp (word, "\\"))
      {
        hl_state_c = MRG_HL_STRING_ESC;
      }
      break;
    case MRG_HL_STRING_ESC:
      hl_state_c = MRG_HL_STRING;
      break;
    case MRG_HL_QSTRING:
      if (!strcmp (word, "'"))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      else if (!strcmp (word, "\\"))
      {
        hl_state_c = MRG_HL_QSTRING_ESC;
      }
      break;
    case MRG_HL_QSTRING_ESC:
      hl_state_c = MRG_HL_QSTRING;
      break;
    case MRG_HL_NEXT_NEUTRAL:
      hl_state_c = MRG_HL_NEUTRAL;
      break;
  }

  switch (hl_state_c)
  {
    case MRG_HL_NEUTRAL:
      if (is_a_number (word))
        ctx_rgb (cr, 0.5, 0.0, 0.0);
      else if (is_one_of (word, hl_punctuation))
        ctx_rgb (cr, 0.4, 0.4, 0.4);
      else if (is_one_of (word, hl_operators))
        ctx_rgb (cr, 0, 0.5, 0);
      else if (is_one_of (word, hl_types))
        ctx_rgb (cr, 0.2, 0.2, 0.5);
      else 
        ctx_rgb (cr, 0, 0, 0);
      break;
    case MRG_HL_STRING:
    case MRG_HL_QSTRING:
        ctx_rgb (cr, 1, 0, 0.5);
      break;
    case MRG_HL_COMMENT:
    case MRG_HL_COMMENT_STAR:
    case MRG_HL_LINECOMMENT:
        ctx_rgb (cr, 0.4, 0.4, 1);
      break;
  }

  ctx_text (cr, word);
}

/* hook syntax highlighter in here..  */
void mrg_hl_text (Ctx *cr, const char *text)
{
  int i;
  CtxString *word = ctx_string_new ("");
  for (i = 0; i < text[i]; i++)
  {
    switch (text[i])
    {
      case ';':
      case '-':
      case '\'':
      case '>':
      case '<':
      case '=':
      case '+':
      case ' ':
      case ':':
      case '"':
      case '*':
      case '/':
      case '\\':
      case '[':
      case ']':
      case ')':
      case ',':
      case '(':
        if (word->length)
        {
          mrg_hl_token (cr, word->str);
          ctx_string_set (word, "");
        }
        ctx_string_append_byte (word, text[i]);
        mrg_hl_token (cr, word->str);
        ctx_string_set (word, "");
        break;
      default:
        ctx_rgb (cr, 0,0,0);
        ctx_string_append_byte (word, text[i]);
        break;
    }
  }
  if (word->length)
    mrg_hl_token (cr, word->str);

  ctx_string_free (word, 1);
}

void ctx_listen (Ctx     *ctx,
                 CtxEventType  types,
                 CtxCb    cb,
                 void*    data1,
                 void*    data2);



/* x and y in cairo user units ; returns x advance in user units  */
float mrg_draw_string (Mrg *mrg, CtxStyle *style, 
                       const char *string,
                       int utf8_len)
{
  //float x = mrg->x;
  float y = mrg->y;
  float new_x, old_x;
  char *temp_string = NULL;
  Ctx *cr = mrg_ctx (mrg);

  ctx_current_point (cr, &old_x, NULL);

  if (utf8_len < 0)
    utf8_len = ctx_utf8_strlen (string);

  if (ctx_utf8_strlen (string) != utf8_len)
  {
    const char *t;
    int i;

    temp_string = strdup (string);
    for (i = 0, t = temp_string ;i < utf8_len && *t; i++)
    {
      t += ctx_utf8_len (*t);
    }
    *(char *)t = 0;
    string = temp_string;
  }
#if 0
  if (mrg_is_terminal (mrg) && mrg_em (mrg) <= CPX * 4 / mrg->ddpx)
  {
    const char *t;
    int i;

    /* XXX: include transforms */
    int offset;
    double u = x , v = y;
    cairo_matrix_t matrix;
    cairo_get_matrix (mrg_ctx (mrg), &matrix);
    cairo_matrix_transform_point (&matrix, &u, &v);

    //u = ctx_floorf(u);
    //v = ctx_floorf(v);
    
    offset = (int)(v/CPX) * ((int)(mrg->width/CPX) * 4) + (int)(u/CPX) * 4;

    old_x = x;
    for (i = 0, t = string; *t; i++)
    {
      if ( v >= 0 && u >= 0 &&
          (int)u/CPX < (int)(mrg->width/CPX) &&
          (int)v/CPX < (int)(mrg->height/CPX))
      {
        int styleno = offset/4;
        memcpy (&mrg->glyphs[offset], t, ctx_utf8_len (*t));
        mrg->styles[styleno] = mrg->state->fg +
                               mrg->state->bg * 8 +
                               (mrg->state->style.text_decoration & 
                                (CTX_TEXT_DECORATION_BOLD|CTX_TEXT_DECORATION_DIM|CTX_TEXT_DECORATION_UNDERLINE|CTX_TEXT_DECORATION_REVERSE)) * 64;;
      }
      t += ctx_utf8_len (*t);
      offset += 4;
      x += CPX / mrg->ddpx;
    }
    new_x = x;
  }
  else 
#endif
  //if (mrg->in_paint)
  {
    ctx_font_size (cr, style->font_size);

#if 0
    if (PROP(text_stroke_width) > 0.01)
    {
      CtxColor *color = ctx_color_new ();
      ctx_get_color (cr, SQZ_text_stroke_color, color);
      mrg_ctx_set_source_color (cr, color);
      ctx_begin_path (cr);
      ctx_move_to   (cr, x, y - _mrg_text_shift (mrg));
      ctx_line_width (cr, PROP(text_stroke_width));
      ctx_line_join (cr, CTX_JOIN_ROUND);
      ctx_text_stroke (cr, string);
      ctx_color_free (color);
    }
#endif

    {
    CtxColor *color = ctx_color_new ();
    ctx_get_color (cr, SQZ_color, color);
    mrg_ctx_set_source_color (cr, color);
    ctx_color_free (color);
    }
    //ctx_move_to   (cr, x, y - _mrg_text_shift (mrg));
    ctx_current_point (cr, &old_x, NULL);

    /* when syntax highlighting,.. should do it as a coloring
     * directly here..
     */

    const char *syntax_highlight = PROPS(syntax_highlight);
    if (!syntax_highlight) syntax_highlight = "";

    if (syntax_highlight[0] == 0)
      ctx_text (cr, string);
    else if (!strcmp (syntax_highlight, "C"))
      mrg_hl_text (cr, string);
    else
      ctx_text (cr, string);

    ctx_current_point (cr, &new_x, NULL);

    if (style->text_decoration & CTX_TEXT_DECORATION_UNDERLINE)
      {
        ctx_rel_move_to (cr, -(new_x-old_x), 0);
        ctx_rel_line_to (cr, new_x-old_x, 0);
        ctx_stroke (cr);
      }
    if (style->text_decoration & CTX_TEXT_DECORATION_LINETHROUGH)
      {
        ctx_move_to (cr, old_x, y - style->font_size / 2);
        ctx_line_to (cr, new_x, y - style->font_size / 2);
        ctx_stroke (cr);
      }
    if (style->text_decoration & CTX_TEXT_DECORATION_OVERLINE)
      {
        ctx_move_to (cr, old_x, y - style->font_size);
        ctx_line_to (cr, new_x, y - style->font_size);
        ctx_stroke (cr);
      }
    //ctx_move_to (cr, new_x, y);
  }
#if 0
  else
  {
    ctx_font_size (cr, style->font_size);
    new_x = old_x + ctx_text_width (cr, string);
  }
#endif

  if (mrg->text_listen_active)
  {
    float em = mrg_em (mrg);
    int no = mrg->text_listen_count-1;
    float x, y;

    ctx_current_point (cr, &x, &y);

    ctx_begin_path (cr);
    ctx_rectangle (cr,
        old_x, y - em, new_x - old_x + 1, em * mrg->state->style.line_height);
    ctx_listen (cr,
                mrg->text_listen_types[no],
                mrg->text_listen_cb[no],
                mrg->text_listen_data1[no],
                mrg->text_listen_data2[no]);
    ctx_begin_path (cr);
    ctx_move_to (cr, x, y);
  }

  if (temp_string)
    free (temp_string);

  return new_x - old_x;
}

float mrg_addstr (Mrg *mrg, const char *string, int utf8_length);

float paint_span_bg_final (Mrg   *mrg, float x, float y,
                           float  width)
{
  CtxStyle *style = ctx_style (mrg);
  Ctx *cr = mrg_ctx (mrg);
  if (style->display != CTX_DISPLAY_INLINE)
    return 0.0;
  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (cr, SQZ_background_color, background_color);

  if (!ctx_color_is_transparent (background_color))
  {
    ctx_save (cr);
    ctx_rectangle (cr, x, y + mrg_em (mrg),
                       width + PROP(padding_right),
                       mrg_em (mrg) * style->line_height);
    mrg_ctx_set_source_color (cr, background_color);
    ctx_fill (cr);
    ctx_restore (cr);
  }

  _mrg_border_top_r (mrg, x, y , width, mrg_em (mrg));
  _mrg_border_bottom_r (mrg, x,y, width, mrg_em (mrg));
  _mrg_border_right (mrg, x, y ,width, mrg_em (mrg));

  ctx_color_free (background_color);
  return PROP(padding_right) + PROP(border_right_width);
}

float paint_span_bg (Mrg   *mrg, float x, float y,
                     float  width)
{
  CtxStyle *style = ctx_style (mrg);
  Ctx *cr = mrg_ctx (mrg);
  if (!cr)
    return 0.0;
  float left_pad = 0.0;
  float left_border = 0.0;
  if (style->display != CTX_DISPLAY_INLINE)
    return 0.0;

  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (cr, SQZ_background_color, background_color);

  if (!mrg->state->span_bg_started)
  {
    left_pad = PROP(padding_left);
    left_border = PROP(border_left_width);
    mrg->state->span_bg_started = 1;
  }

  if (!ctx_color_is_transparent (background_color))
  {
    ctx_save (cr);
    ctx_rectangle (cr, x + left_border, y,
                         width + left_pad,
                         mrg_em (mrg) * style->line_height);
    mrg_ctx_set_source_color (cr, background_color);
    ctx_fill (cr);
    ctx_restore (cr);
  }

  if (left_pad || left_border)
  {
    _mrg_border_left (mrg, x + left_pad + left_border, y, width, mrg_em (mrg));
    _mrg_border_top_l (mrg, x + left_pad + left_border, y, width , mrg_em (mrg));
    _mrg_border_bottom_l (mrg, x + left_pad + left_border, y, width , mrg_em (mrg));
  }
  else
  {
    _mrg_border_top_m (mrg, x, y, width, mrg_em (mrg));
    _mrg_border_bottom_m (mrg, x, y, width, mrg_em (mrg));
  }

  ctx_color_free (background_color);

  return left_pad + left_border;
}

float
mrg_addstr (Mrg *mrg, const char *string, int utf8_length)
{
  float x = mrg->x;
  float y = mrg->y;
  CtxStyle *style = ctx_style (mrg);
  float wwidth = measure_word_width (mrg, string);
  float left_pad;
  left_pad = paint_span_bg (mrg, x, y, wwidth); // TODO avoid doing this for out of bounds

  mrg->line_max_height[mrg->line_level] =
      ctx_maxf (mrg->line_max_height[mrg->line_level],
                style->font_size);
  {
    float tx = x;
    float ty = y;
    ctx_user_to_device (mrg_ctx (mrg), &tx, &ty);
    if (ty > ctx_height (mrg->ctx) * 2 ||
        tx > ctx_width (mrg->ctx)* 2 ||
        tx < -ctx_width (mrg->ctx) * 2 ||
        ty < -ctx_height (mrg->ctx) * 2)
    {
      /* bailing early*/
    }
    else
    {

     if (mrg->line_got_baseline[mrg->line_level] == 0)
     {
        ctx_move_to (mrg->ctx, mrg->x, mrg->y);

        char name[10]="lin_";
        name[3]=mrg->line_level+2;

        ctx_deferred_rel_move_to (mrg->ctx, name, 0.0, 0.0);//mrg_em (mrg));
        mrg->line_got_baseline[mrg->line_level] = 1;
      }

      if (left_pad != 0.0f)
      {
        ctx_rel_move_to (mrg->ctx, left_pad, 0.0f);
      }
      mrg_draw_string (mrg, &mrg->state->style, string, utf8_length);
    }
  }

  return wwidth + left_pad;
}

/******** end of core text-drawing primitives **********/

#if 0
void mrg_xy (Mrg *mrg, float x, float y)
{
  mrg->x = x * mrg_em (mrg);
  mrg->y = y * mrg_em (mrg);
}
#endif

void mrg_set_xy (Mrg *mrg, float x, float y)
{
  mrg->x = x;
  mrg->y = y;
  mrg->state->overflowed = 0;
}

float mrg_x (Mrg *mrg)
{
  return mrg->x;
}

float mrg_y (Mrg *mrg)
{
  return mrg->y;
}

void mrg_set_wrap_skip_lines (Mrg *mrg, int skip_lines);
void mrg_set_wrap_max_lines  (Mrg *mrg, int max_lines);

void mrg_set_wrap_skip_lines (Mrg *mrg, int skip_lines)
{
    mrg->state->skip_lines = skip_lines;
}

void mrg_set_wrap_max_lines  (Mrg *mrg, int max_lines)
{
    mrg->state->max_lines = max_lines;
}

static void _mrg_spaces (Mrg *mrg, int count)
{
  while (count--)
    {
     if (mrg->state->style.print_symbols)
        mrg->x+=mrg_addstr (mrg, "␣", -1);
     else
     {
        float diff = mrg_addstr (mrg, " ", 1);

#if 0
        if (mrg_is_terminal (mrg) && mrg_em (mrg) <= CPX * 4 / mrg->ddpx)
        {
        }
        else
#endif
        {
          if (mrg->state->style.text_decoration & CTX_TEXT_DECORATION_REVERSE)
          {
            Ctx *cr = mrg_ctx (mrg);
            ctx_rectangle (cr, mrg->x + diff*0.1, mrg->y + mrg_em(mrg)*0.2, diff*0.8, -mrg_em (mrg)*1.1);
            ctx_rgb (cr, 1,1,1);
            ctx_fill (cr);
          }
        }
        mrg->x += diff;
     }
    }
}

#define EMIT_NL() \
    do {wraps++; \
    if (wraps >= max_lines)\
      return wraps;\
    if (skip_lines-- <=0)\
      {\
         if (print) { if (gotspace)\
             _mrg_spaces (mrg, 1);\
         if (cursor_start == pos -1 && cursor_start>0 && mrg->text_edited)\
           {\
             itk_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             itk_end (mrg, NULL);\
           }\
         else\
           _mrg_nl (mrg);\
         } else _mrg_nl (mrg);\
      }\
    if (skip_lines<=0)\
      mrg_set_xy (mrg, _mrg_dynamic_edge_left(mrg), mrg_y (mrg));}while(0)

#define EMIT_NL2() \
    do {\
    if (skip_lines-- <=0)\
      {\
         if (print) {if (gotspace)\
             _mrg_spaces (mrg, 1);\
         if (cursor_start == *pos -1 && cursor_start>0 && mrg->text_edited)\
           {\
             itk_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             itk_end (mrg, NULL);\
           }\
         else\
           _mrg_nl (mrg);\
         } else _mrg_nl (mrg);\
      }\
    if (skip_lines<=0)\
      mrg_set_xy (mrg, _mrg_dynamic_edge_left(mrg), mrg_y (mrg));}while(0)



static void mrg_get_edit_state (Mrg *mrg, 
     float *x, float *y, float *s, float *e,
     float *em_size)
{
  if (x) *x = mrg->e_x;
  if (y) *y = mrg->e_y;
  if (s) *s = mrg->e_ws;
  if (e) *e = mrg->e_we;
  if (em_size) *em_size = mrg->e_em;
}


static void emit_word (Mrg *mrg,
                       int  print,
                       const char *data,
                       const char *word,
                       int         max_lines,
                       int         skip_lines,
                       int         cursor_start,
                       int        *pos,
                       int        *wraps,
                       int        *wl,
                       int         c,
                       int         gotspace)
{
    float len = ctx_utf8_strlen (word);
    float wwidth = measure_word_width (mrg, word);

    if (mrg->x + wwidth >= _mrg_dynamic_edge_right (mrg))
    {
      if (mrg->x > mrg_edge_left(mrg) || *wraps != 0)
      {
        EMIT_NL2();
      }
    }
    if (mrg->x != mrg_edge_left(mrg) && gotspace)
      { 
        if ((skip_lines<=0)) 
          { 
            if (cursor_start == *pos-1 && cursor_start>=0 && mrg->text_edited)
            { 
              if (print) { 
               itk_start (mrg, ".cursor", NULL);
               _mrg_spaces (mrg, 1); 
               itk_end (mrg, NULL);
              } else { 
               mrg->x += measure_word_width (mrg, " ");
              }
            }
            else 
              {
                if (print){
                  if (mrg->state->style.print_symbols)
                    {
                      itk_start (mrg, "dim", NULL);
                      mrg->x += mrg_addstr (mrg, "␣", -1);
                      itk_end (mrg, NULL);
                    }
                  else
                    _mrg_spaces (mrg, 1);
                } else {
                  if (mrg->state->style.print_symbols)
                  {
                    mrg->x += measure_word_width (mrg, "␣");
                  }
                  else
                  {
                    mrg->x += measure_word_width (mrg, " ");
                  }
                }
              } 
          }
      } 
    if ((skip_lines<=0)) {
      if (print){if (cursor_start >= *pos && *pos + len > cursor_start && mrg->text_edited)
        { 
#if 0  // XXX: there is a bug in mrg_addstr it doesn't respect the length argument 
          mrg->x += mrg_addstr (mrg,  word, cursor_start - *pos);
          itk_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg,  mrg_utf8_skip (word, cursor_start - *pos), 1);
          itk_end (mrg, NULL);
          mrg->x += mrg_addstr (mrg,  mrg_utf8_skip (word, cursor_start - *pos + 1), len - (cursor_start - *pos) - 1);
#else

          char *dup, *dup2, *dup3;

          dup = strdup (word);
          dup2 = strdup (ctx_utf8_skip (dup, cursor_start - *pos));
          dup3 = strdup (ctx_utf8_skip (dup, cursor_start - *pos + 1));
          *((char*)ctx_utf8_skip (dup,  cursor_start - *pos)) = 0;
          *((char*)ctx_utf8_skip (dup2, 1)) = 0;

          mrg->x += mrg_addstr (mrg,  dup, -1);
          itk_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg,  dup2, -1);
          itk_end (mrg, NULL);
          mrg->x += mrg_addstr (mrg,  dup3, -1);

          free (dup);
          free (dup2);
          free (dup3);
#endif
        }
      else
        {
          mrg->x += mrg_addstr (mrg,  word, len); 
        }
      } else {
          mrg->x += wwidth;
      }
    }
    *pos += len;
    *wl = 0;

}

static int itk_print_wrap (Mrg        *mrg,
                           int         print,
                           const char *data, int length,
                           int         max_lines,
                           int         skip_lines,
                           int         cursor_start,
                           float     *retx,
                           float     *rety)
{
#define MAX_WORDL 1024
  char word[MAX_WORDL+1]="";
  int wl = 0;
  int c;
  int wraps = 0;
  int pos;
  int gotspace = 0;

  if (mrg->state->overflowed)
  {
    return 0;
  }

  float space_width = measure_word_width (mrg, " ");

  pos = 0;

  if (max_lines <= 0)
    max_lines = 4096;
  if (retx)
    *retx = -1;

  if (mrg->text_edited && print)
    {
      mrg->e_x = mrg->x;
      mrg->e_y = mrg->y;
      mrg->e_ws = mrg_edge_left(mrg);
      mrg->e_we = mrg_edge_right(mrg);
      mrg->e_em = mrg_em (mrg);
    }

  ctx_font_size (mrg_ctx (mrg), ctx_style(mrg)->font_size);

  for (c = 0 ; c < length && data[c] && ! mrg->state->overflowed; c++)
    switch (data[c])
      {
        case '\n':
          if (wl)
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (mrg->state->style.print_symbols && print)
          {
            itk_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg,  "¶", -1);\
            itk_end (mrg, NULL);
          }
          EMIT_NL();
          gotspace = 0;
          break;
        case '\t': // XXX: this collapses tabs to a single space
        case ' ':
          if (wl == 0)
            {
              if (cursor_start == pos-1 && cursor_start>=0 && mrg->text_edited)
                {
                  if (print)
                  {
                    itk_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    itk_end (mrg, NULL);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg,  " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      itk_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg,  "␣", -1);
                      itk_end (mrg, NULL);
                    }
                  else
                    {
                      mrg->x+=mrg_addstr (mrg,  " ", -1);

                    }
                }
            }
          else
            {
              emit_word (mrg, print, data, word,
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (retx && *retx < 0 && pos >= cursor_start)
            {
              float tailwidth;
              const char *rest = &word[ctx_utf8_strlen (word) - (pos-cursor_start)];
#if 0
              if (mrg_is_terminal (mrg))
                tailwidth = (pos-cursor_start -1) * CPX / mrg->ddpx;
              else
#endif
                tailwidth = measure_word_width (mrg, rest);
              *retx = mrg->x - tailwidth;
              *rety = mrg->y;
              return pos;
            }
          gotspace = 1;
          break;
        default:
          word[wl++]= data[c];
	  wl = ctx_mini (wl, MAX_WORDL-1);
          word[wl]  = '\0';
          break;
      }
  if (wl) /* orphaned word for last line. */
    {
      emit_word (mrg, print, data, word, 
                 max_lines, skip_lines,
                 cursor_start,
                 &pos, &wraps, &wl, c, gotspace);
    }
   /* cursor at end */
   if (cursor_start == pos && cursor_start>=0 && mrg->text_edited)
    {
      if (print)
      {
        if (c && data[c-1]==' ')
          mrg->x += space_width;
        itk_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        itk_end (mrg, NULL);
      }
      else
        mrg->x += space_width;
    }
  if (retx && *retx < 0 && pos >= cursor_start)
    {
       *retx = mrg->x; 
       *rety = mrg->y;
      return pos;
    }
  return wraps;
}

int itk_print_get_xy (Mrg *mrg, const char *string, int no, float *x, float *y)
{
  int ret;
  if (!string)
    return 0;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      ret = itk_print_wrap (mrg, 0, string, strlen (string), mrg->state->max_lines,
                             mrg->state->skip_lines, no, x, y);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }
  if (y) *y = mrg->y;
  if (x) *x = mrg->x + no; // XXX: only correct for nct/monospace

  return 0;
}

typedef struct _MrgGlyph MrgGlyph;

struct _MrgGlyph{
  unsigned long index; /*  done this way, the remnants of layout; before feeding
                        *  glyphs positions in cairo, similar to how pango would do
                        *  can be reused for computing the caret nav efficiently.
                        */
  float x;
  float y;
  int   no;
};

static int itk_print_wrap2 (Mrg        *mrg,
                           int         print,
                           const char *data, int length,
                           int         max_lines,
                           int         skip_lines,
                           CtxList   **list)
{
  char word[400]="";
  int wl = 0;
  int c;
  int wraps = 0;
  int pos;
  int gotspace = 0;
  int cursor_start = -1;

  MrgGlyph *g = calloc (sizeof (MrgGlyph), 1);
  g->x = length;
  g->y = 42;
  g->index = 44;
  g->no = 2;
  ctx_list_append (list, g);

  if (mrg->state->overflowed)
  {
    return 0;
  }

  pos = 0;

  if (max_lines <= 0)
    max_lines = 4096;

  if (mrg->text_edited && print)
    {
      mrg->e_x = mrg->x;
      mrg->e_y = mrg->y;
      mrg->e_ws = mrg_edge_left(mrg);
      mrg->e_we = mrg_edge_right(mrg);
      mrg->e_em = mrg_em (mrg);
#if 0
      if (mrg->scaled_font)
        cairo_scaled_font_destroy (mrg->scaled_font);
#endif
      ctx_font_size (mrg_ctx (mrg), ctx_style(mrg)->font_size);
#if 0
      mrg->scaled_font = cairo_get_scaled_font (mrg_ctx (mrg));
      cairo_scaled_font_reference (mrg->scaled_font);
#endif
    }

  for (c = 0 ; c < length && data[c] && ! mrg->state->overflowed; c++)
    switch (data[c])
      {
        case '\n':
          if (wl)
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (mrg->state->style.print_symbols && print)
          {
            itk_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg,  "¶", -1);\
            itk_end (mrg, NULL);
          }
          EMIT_NL();
          gotspace = 0;
          break;
        case ' ':
          if (wl == 0)
            {
              if (cursor_start == pos-1 && cursor_start>=0 && mrg->text_edited)
                {
                  if (print)
                  {
                    itk_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    itk_end (mrg, NULL);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg,  " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      itk_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg,  "␣", -1);
                      itk_end (mrg, NULL);
                    }
                  else
                    {
                      mrg->x+=mrg_addstr (mrg,  " ", -1);
                    }
                }
            }
          else
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;
          
#if 0
          if (retx && *retx < 0 && pos >= cursor_start)
            {
              float tailwidth;
              const char *rest = &word[ctx_utf8_strlen (word) - (pos-cursor_start)];
              if (mrg_is_terminal (mrg))
                tailwidth = (pos-cursor_start -1) * CPX / mrg->ddpx;
              else
                tailwidth = measure_word_width (mrg, rest);
              *retx = mrg->x - tailwidth;
              *rety = mrg->y;
              return pos;
            }
#endif
          gotspace = 1;
          break;
        default:
          word[wl++]= data[c];
          word[wl]  = '\0';
          break;
      }
  if (wl) /* orphaned word for last line. */
    {
      emit_word (mrg, print, data, word, 
                 max_lines, skip_lines,
                 cursor_start, 
                 &pos, &wraps, &wl, c, gotspace);
    }
   /* cursor at end */
   if (cursor_start == pos && cursor_start>=0 && mrg->text_edited)
    {
      if (print)
      {
        itk_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        itk_end (mrg, NULL);
      }
      else
        mrg->x += measure_word_width (mrg, " ");
    }
#if 0
  if (retx && *retx < 0 && pos >= cursor_start)
    {
       *retx = mrg->x; 
       *rety = mrg->y;
      return pos;
    }
#endif
  return wraps;
}

CtxList *itk_print_get_coords (Mrg *mrg, const char *string)
{
  CtxList *ret = NULL;
  if (!string)
    return ret;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      itk_print_wrap2 (mrg, 0, string, strlen (string), mrg->state->max_lines,
                       mrg->state->skip_lines, &ret);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }

  return ret;
}

#include <math.h>

int itk_print (Mrg *mrg, const char *string)
{
  float ret;
  CtxStyle *style = ctx_style (mrg);
  mrg->unresolved_line = 1;

#ifdef SNAP
  float em = mrg_em (mrg);  /* XXX: a global body-line spacing 
                               snap is better grid design */
  mrg->x = ceil (mrg->x / em) * em;
  mrg->y = ceil (mrg->y / em) * em;
#endif

  if (mrg->text_edited)
    ctx_string_append_str (mrg->edited_str, string);

  if (style->display == CTX_DISPLAY_NONE)
    return 0.0;

  if (!string)
    return 0;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
   return itk_print_wrap (mrg, 1, string, strlen (string), mrg->state->max_lines, mrg->state->skip_lines, mrg->cursor_pos, NULL, NULL);

  ret  = mrg_addstr (mrg,  string, ctx_utf8_strlen (string));
  mrg->x += ret;
  return ret;
}

void _mrg_text_prepare (Mrg *mrg)
{
  hl_state_c = MRG_HL_NEUTRAL;
}

void _mrg_text_init (Mrg *mrg)
{
  // XXX: this should be done in a prepre,.. not an init?
  //
  mrg->state->style.line_height = 1.0;
  mrg->state->style.print_symbols = 0;
}

void  mrg_text_listen_done (Mrg *mrg)
{
  mrg->text_listen_active = 0;
}

void  mrg_text_listen_full (Mrg *mrg, CtxEventType types,
                            CtxCb cb, void *data1, void *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                      void    *finalize_data)
{
  int no = mrg->text_listen_count;
  if (cb == NULL)
  {
    mrg_text_listen_done (mrg);
    return;
  }
  if (no + 1 >= CTX_MAX_TEXT_LISTEN)
  {
    fprintf (stderr, "mrg text listen overflow\n");
    return;
  }

  mrg->text_listen_types[no] = types;
  mrg->text_listen_cb[no] = cb;
  mrg->text_listen_data1[no] = data1;
  mrg->text_listen_data2[no] = data2;
  mrg->text_listen_finalize[no] = finalize;
  mrg->text_listen_finalize_data[no] = finalize_data;
  mrg->text_listen_count++;
  mrg->text_listen_active = 1;
}

void  mrg_text_listen (Mrg *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2)
{
  mrg_text_listen_full (mrg, types, cb, data1, data2, NULL, NULL);
}


static void cmd_home (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_end (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str);
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_backspace (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  char *new;
  const char *rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);
  const char *mark = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos-1);

  if (mrg->cursor_pos <= 0)
    {
      mrg->cursor_pos = 0;
    }
  else
    {
      new = malloc (strlen (mrg->edited_str->str) + 1);
      memcpy (new, mrg->edited_str->str, ((mark - mrg->edited_str->str)));
      memcpy (new + ((mark - mrg->edited_str->str)), rest, strlen (rest));
      new [strlen (mrg->edited_str->str)-(rest-mark)] = 0;
      mrg->update_string (new, mrg->update_string_user_data);
      ctx_string_set (mrg->edited_str, new);
      free (new);
      mrg->cursor_pos--;
    }
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_delete (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  char *new;
  const char *rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos+1);
  const char *mark = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);

  new = malloc (strlen (mrg->edited_str->str) + 1);
  memcpy (new, mrg->edited_str->str, ((mark - mrg->edited_str->str)));
  memcpy (new + ((mark - mrg->edited_str->str)), rest, strlen (rest));
  new [strlen (mrg->edited_str->str)-(rest-mark)] = 0;

  mrg->update_string (new, mrg->update_string_user_data);
  ctx_string_set (mrg->edited_str, new);
  free (new);
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_down (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx, cy;
  cx = cy = 0;
 
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);
  itk_set_edge_left (mrg, e_s - PROP (padding_left));
  itk_set_edge_right (mrg, e_e + PROP (padding_right));
  mrg_set_xy (mrg, e_x, e_y);
  itk_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

  {
    int no;
    int best = mrg->cursor_pos;
    float best_score = 10000000000.0;
    float best_y = cy;
    int strl = ctx_utf8_strlen (mrg->edited_str->str);
    for (no = mrg->cursor_pos + 1; no < mrg->cursor_pos + 256 && no < strl; no++)
    {
      float x = 0, y = 0;
      float attempt_score = 0.0;
      mrg_set_xy (mrg, e_x, e_y);
      itk_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

      if (y > cy && best_y == cy)
        best_y = y;

      if (y > cy)
        attempt_score = (y - best_y);
      else
        attempt_score = 1000.0;

      attempt_score += fabs(cx-x) / 10000000.0;

      if (attempt_score <= best_score)
      {
        best_score = attempt_score;
        best = no;
      }
    }
    if (best_y == cy)
    {
      mrg->cursor_pos = strl;
#if 0
      ctx_key_press (mrg, 0, "down-nudge", 0);
#endif
      mrg_queue_draw (mrg, NULL);
      return;
    }
    mrg->cursor_pos = best;
  }

  if (mrg->cursor_pos >= ctx_utf8_strlen (mrg->edited_str->str))
    mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str) - 1;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_up (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx = 0.0f, cy = 0.0f;
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);

  itk_set_edge_left  (mrg, e_s - PROP(padding_left));
  itk_set_edge_right (mrg, e_e + PROP(padding_right));

  mrg_set_xy (mrg, e_x, e_y);
  itk_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

  /* XXX: abstract the finding of best cursor pos for x coord to a function */
  {
    int no;
    int best = mrg->cursor_pos;
    float best_y = cy;
    float best_score = 1000000000000.0;
    for (no = mrg->cursor_pos - 1; no>= mrg->cursor_pos - 256 && no > 0; no--)
    {
      float x = 0, y = 0;
      float attempt_score = 0.0;
      mrg_set_xy (mrg, e_x, e_y);
      itk_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

      if (y < cy && best_y == cy)
        best_y = y;

      if (y < cy)
        attempt_score = (best_y - y);
      else
        attempt_score = 1000.0;

      attempt_score += fabs(cx-x) / 10000000.0;

      if (attempt_score < best_score)
      {
        best_score = attempt_score;
        best = no;
      }
    }
    mrg->cursor_pos = best;
    if (best_y == cy)
    {
      mrg->cursor_pos = 0;
      mrg_queue_draw (mrg, NULL);
      ctx_key_press (event->ctx, 0, "up-nudge", 0);
      return; // without stop propagate this should permit things registered earlier to fire
    }
  }

  if (mrg->cursor_pos < 0)
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

int mrg_get_cursor_pos (Mrg *mrg)
{
  return mrg->cursor_pos;
}

void mrg_set_cursor_pos (Mrg *mrg, int pos)
{
  mrg->cursor_pos = pos;
  mrg_queue_draw (mrg, NULL);
}

static void cmd_page_down (CtxEvent *event, void *data1, void *data2)
{
  int i;
  for (i = 0; i < 6; i++)
    cmd_down (event, data1, data2);
  ctx_event_stop_propagate (event);
}

static void cmd_page_up (CtxEvent *event, void *data1, void *data2)
{
  int i;
  for (i = 0; i < 6; i++)
    cmd_up (event, data1, data2);
  ctx_event_stop_propagate (event);
}

static void cmd_left (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos--;
  if (mrg->cursor_pos < 0)
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_right (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos++;

  /* should mrg have captured the text printed in-between to build its idea
   * of what is being edited, thus being able to do its own internal cursor
   * positioning with that cache?
   */

  if (mrg->cursor_pos > ctx_utf8_strlen (mrg->edited_str->str))
    mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str);

  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}


/* the added utf8 bits go to edited_str as well, so that successive edits do work out
 *
 */

static void add_utf8 (Mrg *mrg, const char *string)
{
  char *new;
  const char *rest;
  /* XXX: this is the code the should be turned into a callback/event
   * to digest for the user of the framework, with a reasonable default
   * for using it from C with a string
   */

  rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);

  new = malloc (strlen (mrg->edited_str->str) + strlen (string) + 1);
  memcpy (new, mrg->edited_str->str, (rest-mrg->edited_str->str));
  memcpy (new + (rest-mrg->edited_str->str), string,  strlen (string));
  memcpy (new + (rest-mrg->edited_str->str) + strlen (string),
          rest, strlen (rest));
  new [strlen (string) + strlen (mrg->edited_str->str)] = 0;
  mrg->update_string (new, mrg->update_string_user_data);
  ctx_string_set (mrg->edited_str, new);
  free (new);
  mrg_queue_draw (mrg, NULL);
  mrg->cursor_pos++;
}

static void cmd_unhandled (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  if (!strcmp (event->string, "space"))
  {
    add_utf8 (mrg, " ");
    ctx_event_stop_propagate (event);
  }

  if (ctx_utf8_strlen (event->string) != 1)
    return;

  add_utf8 (mrg, event->string);
  ctx_event_stop_propagate (event);
}

#if 0
static void cmd_space (CtxEvent *event, void *data1, void *data2)
{
  if (!ctx_utf8_strlen (event->key_name) == 1)
    return 0;

  add_utf8 (event->mrg, " ");
  return 1;
}
#endif

static void cmd_return (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  // this check excludes terminal from working
  //if (!(ctx_utf8_strlen (event->key_name) == 1))
  //  return;

  add_utf8 (mrg, "\n");
  ctx_event_stop_propagate (event);
}

static void cmd_escape (CtxEvent *event, void *data, void *data2)
{
#if 0
  mrg_edit_string (event->mrg, NULL, NULL, NULL);
#endif
}

void mrg_text_edit_bindings (Mrg *mrg)
{
  ctx_add_key_binding (mrg->ctx, "escape",    NULL, "stop editing",    cmd_escape,      mrg);
  ctx_add_key_binding (mrg->ctx, "return",    NULL, "add newline",     cmd_return,    mrg);
  ctx_add_key_binding (mrg->ctx, "home",      NULL, "cursor to start", cmd_home, mrg);
  ctx_add_key_binding (mrg->ctx, "end",       NULL, "cursor to end",   cmd_end,    mrg);
  ctx_add_key_binding (mrg->ctx, "left",      NULL, "cursor left",     cmd_left,    mrg);
  ctx_add_key_binding (mrg->ctx, "right",     NULL, "cursor right",    cmd_right,  mrg);
  ctx_add_key_binding (mrg->ctx, "up",        NULL, "cursor up",       cmd_up,        mrg);
  ctx_add_key_binding (mrg->ctx, "down",      NULL, "cursor down",     cmd_down,    mrg);
  ctx_add_key_binding (mrg->ctx, "page-up",   NULL, "cursor up",       cmd_page_up,     mrg);
  ctx_add_key_binding (mrg->ctx, "page-down", NULL, "cursor down",     cmd_page_down, mrg);
  ctx_add_key_binding (mrg->ctx, "backspace", NULL, "remove preceding character", cmd_backspace, mrg);
  ctx_add_key_binding (mrg->ctx, "delete",    NULL, "remove character under cursor", cmd_delete, mrg);
  ctx_add_key_binding (mrg->ctx, "any", NULL, "add if key name is 1 char long", cmd_unhandled, mrg);
}

#if 1
void mrg_edit_string (Mrg *mrg, char **string,
                      void (*update_string)(Mrg *mrg,
                        char **string_loc,
                        const char *new_string,
                        void  *user_data),
                      void *user_data)
{
  if (mrg->edited == string)
    return;
  mrg->edited = string;
  mrg->update_string = (void*)update_string;
  mrg->update_string_user_data = user_data;
  if (string)
    mrg->cursor_pos = ctx_utf8_strlen (*string);
  else
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
}
#endif

void
itk_printf (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  itk_print (mrg, buffer);
  free (buffer);
}

void  itk_set_font_size (Mrg *mrg, float font_size)
{
  mrg->font_size = font_size;
  //itk_set_stylef (mrg, "font-size:%fpx;", font_size);
}

void _mrg_block_edit (Mrg *mrg)
{
  mrg->text_edit_blocked = 1;
}
void _mrg_unblock_edit (Mrg *mrg)
{
  mrg->text_edit_blocked = 0;
}

void mrg_edit_start_full (Mrg *mrg,
                          MrgNewText  update_string,
                          void *user_data,
                          CtxDestroyNotify destroy,
                          void *destroy_data)
{
  if (mrg->update_string_destroy_notify)
  {
    mrg->update_string_destroy_notify (mrg->update_string_destroy_data);
  }
  mrg->got_edit                     = 1;
  mrg->text_edited                  = 1;
  mrg->update_string                = update_string;
  mrg->update_string_user_data      = user_data;
  mrg->update_string_destroy_notify = destroy;
  mrg->update_string_destroy_data   = destroy_data;
}

void  mrg_edit_start (Mrg *mrg,
                      MrgNewText  update_string,
                      void *user_data)
{
  return mrg_edit_start_full (mrg, update_string, user_data, NULL, NULL);
}

void  mrg_edit_end (Mrg *mrg)
{
  mrg->text_edited = 0;
  mrg_text_edit_bindings (mrg);
}


#if 0
static void ctx_css_add_class (Mrg *mrg, const char *class_name)
{
  int i;
  CtxStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->classes[i]; i++);
  node->classes[i] = mrg_intern_string (class_name);
}

static void ctx_css_add_pseudo_class (Mrg *mrg, const char *pseudo_class)
{
  int i;
  CtxStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->pseudo[i]; i++);
  node->pseudo[i] = mrg_intern_string (pseudo_class);
}
#endif

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data)
{
  mrg->state->wrap_edge_left = wrap_edge_left;
  mrg->state->wrap_edge_right = wrap_edge_right;
  mrg->state->wrap_edge_data = wrap_edge_data;
}

static void update_rect_geo (Ctx *ctx, void *userdata, const char *name, int count,
                             float *x, float *y, float *w, float *h)
{
  CtxFloatRectangle *geo = userdata;
  *w = geo->width;
  *h = geo->height;
}


void _mrg_layout_post (Mrg *mrg, CtxFloatRectangle *ret_rect)
{
  Ctx *ctx         = mrg->ctx;
  float vmarg      = 0;
  CtxStyle *style  = ctx_style (mrg);
  float height     = PROP(height);
  float width      = PROP(width);
  int returned_dim = 0;
  float ascent, descent;
  ctx_font_extents (mrg->ctx, &ascent, &descent, NULL);
  
  if (mrg->state->flow_root)
    clear_both (mrg);

  if (is_block_item (style))
  {
    if (style->display == CTX_DISPLAY_INLINE_BLOCK)
    {
      if (height == 0)
        height = mrg_y (mrg) - (mrg->state->block_start_y);
      
      mrg->line_max_height[mrg->line_level-1] =
        ctx_maxf (mrg->line_max_height[mrg->line_level-1],
                  height);
      
    }
    else
    {
      if (mrg->line_got_baseline[mrg->line_level])
        _mrg_nl (mrg);
    }
    mrg->line_level--;
    //mrg->line_got_baseline [mrg->line_level] = 0;
  }

  /* remember data to store about float, XXX: perhaps better to store
   * straight into parent state?
   */
  if (style->float_)
  {
    CtxFloatData *float_data = &mrg->float_data[mrg->floats];
    // XXX protect against overflow
    mrg->floats++;

    float_data->type = style->float_;
    float_data->x = 
       mrg->state->block_start_x - PROP(padding_left) - PROP(border_left_width) - PROP(margin_left);
    float_data->y = 
         mrg->state->block_start_y - mrg_em(mrg) - PROP(padding_top) - PROP(border_top_width)
      - PROP(margin_top);

    float_data->width = 
         mrg_edge_right (mrg) - mrg_edge_left (mrg)
     //+ PROP(border_left_width) 
     //+ PROP(border_right_width)  
#if 0
     /*+ PROP(padding_left) +*/ + PROP(border_left_width) + PROP(margin_left)
     /*+ PROP(padding_right) +*/ + PROP(border_right_width) + PROP(margin_right)
#endif
     ;

    float_data->height = 
       mrg_y (mrg) - (mrg->state->block_start_y)
         + PROP(margin_bottom) + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width);
  }


  if (style->display == CTX_DISPLAY_INLINE_BLOCK)
  {
    CtxFloatRectangle _geo;
    CtxFloatRectangle *geo = &_geo;
    memset (geo, 0, sizeof (_geo));

    if (width == 0)
    {
      width = mrg_x (mrg) - (mrg->state->block_start_x) + PROP(padding_right);
    }
    geo->width = width;

    if (height == 0)
      height = mrg_y (mrg) - (mrg->state->block_start_y);
    geo->height = height;

    char name[10]="ele_";
    name[3]=mrg->state_no+2;

    ctx_resolve (mrg->ctx, name, update_rect_geo, geo);
    mrg_box (mrg,
        mrg->state->block_start_x,
        mrg->state->block_start_y,
        geo->width,
        geo->height);
    if (ret_rect)
    {
       ret_rect->x = mrg->state->block_start_x;
       ret_rect->y = mrg->state->block_start_y;
       ret_rect->width = geo->width;
       ret_rect->height = geo->height;
       returned_dim = 1;
    }

    {
      CtxMatrix transform;
      ctx_get_matrix (mrg_ctx (mrg), &transform);
      float x = ctx_pointer_x (ctx);
      float y = ctx_pointer_y (ctx);
      ctx_matrix_invert (&transform);
      ctx_matrix_apply_transform (&transform, &x, &y);
    }

    //mrg_edge_right (mrg) - mrg_edge_left (mrg), mrg_y (mrg) - (mrg->state->block_start_y - mrg_em(mrg)));

      mrg_set_xy (mrg, 
          mrg_x (mrg) + width,
          mrg_y (mrg));
  }
  else if (is_block_item (style))
  {
    CtxFloatRectangle _geo;
    CtxFloatRectangle *geo = &_geo;
    memset (geo, 0, sizeof (_geo));

    geo->width = width;
    if (width == 0)
    {
#if 0
      if (mrg_y (mrg) == (ctx->state->block_start_y))
        geo->width = mrg_x (mrg) - (ctx->state->block_start_x);
      else
        geo->width = mrg->state->edge_right  - (ctx->state->block_start_x);
#endif
      if (style->float_)
      {
        geo->width = mrg_x (mrg) - (mrg->state->block_start_x);
      }
      else
      {
        geo->width = _mrg_dynamic_edge_right(mrg)-
                      _mrg_dynamic_edge_left (mrg);
      }
    }

    if (height == 0)
      height = mrg_y (mrg) - (mrg->state->block_start_y);
    geo->height = height;

    char name[10]="ele_";
    name[3]=mrg->state_no+2;


    mrg_box (mrg,
        mrg->state->block_start_x,
        mrg->state->block_start_y,
        geo->width,
        geo->height);
    if (ret_rect)
    {
       ret_rect->x = mrg->state->block_start_x;
       ret_rect->y = mrg->state->block_start_y;
       ret_rect->width = geo->width;
       ret_rect->height = geo->height;
       returned_dim = 1;
    }

    geo->width += PROP(padding_right) + PROP(padding_left);
    geo->height += PROP(padding_top) + PROP(padding_bottom);
    ctx_resolve (mrg->ctx, name, update_rect_geo, geo);

    {
      CtxMatrix transform;
      ctx_get_matrix (mrg_ctx (mrg), &transform);
      float x = ctx_pointer_x (ctx);
      float y = ctx_pointer_y (ctx);
      ctx_matrix_invert (&transform);
      ctx_matrix_apply_transform (&transform, &x, &y);
    }

    //mrg_edge_right (mrg) - mrg_edge_left (mrg), mrg_y (mrg) - (mrg->state->block_start_y - mrg_em(mrg)));

    if (!style->float_ && (style->display == CTX_DISPLAY_BLOCK ||
                           style->display == CTX_DISPLAY_FLOW_ROOT ||
		           style->display == CTX_DISPLAY_LIST_ITEM))
    {
      vmarg = PROP(margin_bottom);

      mrg_set_xy (mrg, 
          mrg_edge_left (mrg),
          mrg_y (mrg) + vmarg + PROP(border_bottom_width) + PROP(padding_bottom));
    }
  }
  else if (style->display == CTX_DISPLAY_INLINE)
  {
    float x0     = mrg->state->original_x;
    float y0     = mrg->state->original_y;
    float width  = mrg->x - x0;
    float height = mrg->y - y0;

    if (ret_rect)
    {
       ret_rect->x = x0;
       ret_rect->y = y0;
       ret_rect->width = width;
       ret_rect->height = height;

       returned_dim = 1;
    }
    mrg_box (mrg, x0, y0, width, height);

    mrg->x += paint_span_bg_final (mrg, mrg->x, mrg->y, 0);
  }


  /* restore insert position when having been out-of-context */
  if (style->float_ ||
      style->position == CTX_POSITION_ABSOLUTE ||
      style->position == CTX_POSITION_FIXED)
  {
    mrg_set_xy (mrg, mrg->state->original_x,
                     mrg->state->original_y);
  }

#if 0
  /* restore state to parent */
  mrg->state_no--;
  if (mrg->state_no<0)
     mrg->state_no=0;
  mrg->state = &mrg->states[mrg->state_no];
#endif

  if (mrg->state->style_id)
  {
    free (mrg->state->style_id);
    mrg->state->style_id = NULL;
  }
  /* restore relative shift */
  if (style->position == CTX_POSITION_RELATIVE)
  {
    //ctx_translate (mrg_ctx (mrg), -PROP(left), -PROP(top)); // not really
    //                                                        // needed we'll
    //                                                        // restore..
    mrg->relative_x -= PROP(left);
    mrg->relative_y -= PROP(top);
  }

  ctx_restore (mrg_ctx (mrg));

  if (style->position == CTX_POSITION_ABSOLUTE ||
      style->position == CTX_POSITION_FIXED)
  {
    int start_offset = mrg->state->drawlist_start_offset;
    int end_offset;
    const CtxEntry *entries = ctx_get_drawlist (mrg->ctx, &end_offset);
    int count = end_offset - start_offset;

    MrgAbsolute *absolute = calloc (sizeof (MrgAbsolute) + count * 9, 1);
    absolute->z_index = style->z_index;
    absolute->top    = PROP(top);
    absolute->left   = PROP(left);
    if (style->position == CTX_POSITION_FIXED)
      absolute->fixed = 1;
    absolute->relative_x = mrg->relative_x;
    absolute->relative_y = mrg->relative_y;
    absolute->entries    = (CtxEntry*) (absolute + 1);
    absolute->count      = count;
    memcpy (absolute->entries, entries + start_offset, count * 9);

    ctx_list_prepend (&mrg->absolutes, absolute);

    ctx_drawlist_force_count (mrg->ctx, mrg->state->drawlist_start_offset);
  }



  mrg->state_no--;
  if (mrg->state_no < 0)
  {
    fprintf (stderr, "unbalanced itk_start/itk_end, enderflow %i\n", mrg->state_no);
    mrg->state_no = 0;
  }
  mrg->state = &mrg->states[mrg->state_no];

  mrg->state->vmarg = vmarg;

  if (ret_rect && !returned_dim)
    fprintf (stderr, "didnt return dim!\n");

  mrg->unresolved_line = 0;
}

enum {
  HTML_ATT_UNKNOWN = 0,
  HTML_ATT_STYLE,
  HTML_ATT_CLASS,
  HTML_ATT_ID,
  HTML_ATT_HREF,
  HTML_ATT_REL,
  HTML_ATT_SRC
};

typedef struct MrgEntity {
   uint32_t    name;
   const char *value;
} MrgEntity;

static MrgEntity entities[]={
  {SQZ_shy,    ""},   // soft hyphen,. should be made use of in wrapping..
  {SQZ_nbsp,   " "},  //
  {SQZ_lt,     "<"},
  {SQZ_gt,     ">"},
  {SQZ_trade,  "™"},
  {SQZ_copy,   "©"},
  {SQZ_middot, "·"},
  {SQZ_bull,   "•"},
  {SQZ_Oslash, "Ø"},
  {SQZ_oslash, "ø"},
  {SQZ_hellip, "…"},
  {SQZ_aring,  "å"},
  {SQZ_Aring,  "Å"},
  {SQZ_aelig,  "æ"},
  {SQZ_AElig,  "Æ"},
  {SQZ_Aelig,  "Æ"},
  {SQZ_laquo,  "«"},
  {SQZ_raquo,  "»"},

  /*the above were added as encountered, the rest in anticipation  */

  {SQZ_reg,    "®"},
  {SQZ_deg,    "°"},
  {SQZ_plusmn, "±"},
  {SQZ_sup2,   "²"},
  {SQZ_sup3,   "³"},
  {SQZ_sup1,   "¹"},
  {SQZ_ordm,   "º"},
  {SQZ_para,   "¶"},
  {SQZ_cedil,  "¸"},
  {SQZ_bull,   "·"},
  {SQZ_amp,    "&"},
  {SQZ_mdash,  "–"},
  {SQZ_apos,   "'"},
  {SQZ_quot,   "\""},
  {SQZ_iexcl,  "¡"},
  {SQZ_cent,   "¢"},
  {SQZ_pound,  "£"},
  {SQZ_euro,   "€"},
  {SQZ_yen,    "¥"},
  {SQZ_curren, "¤"},
  {SQZ_sect,   "§"},
  {SQZ_phi,    "Φ"},
  {SQZ_omega,  "Ω"},
  {SQZ_alpha,  "α"},

  /* XXX: incomplete */

  {0, NULL}
};

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

static void
mrg_parse_transform (Mrg *mrg, CtxMatrix *matrix, const char *str)
{
  if (!strncmp (str, "matrix", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0.0,};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s &&  numbers < 11; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }

    if (number[0] == 0.0f) {};
#if 0
    matrix->m[0][0] = number[0];
    matrix->m[0][1] = number[1];
    matrix->m[1][0] = number[2];
    matrix->m[1][1] = number[3];
    matrix->m[2][0] = number[4];
    matrix->m[2][1] = number[5];
#else
#if 0
    matrix->m[0][0] = number[0];
    matrix->m[1][0] = number[1];
    matrix->m[0][1] = number[2];
    matrix->m[1][1] = number[3];
    matrix->m[0][2] = number[4];
    matrix->m[1][2] = number[5];
#endif
#endif
  }
  else if (!strncmp (str, "scale", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0,0};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    if (numbers <= 1)
      ctx_matrix_scale (matrix, number[0], number[0]);
    else
      ctx_matrix_scale (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "translate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_translate (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "rotate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_rotate (matrix, number[0] / 360.0 * 2 * M_PI);
  }
  else
  {
    fprintf (stderr, "unhandled transform: %s\n", str);
    ctx_matrix_identity (matrix);
  }
}

int
mrg_parse_svg_path (Mrg *mrg, const char *str)
{
  /* this function is the seed of the ctx parser */
  Ctx *ctx = mrg_ctx (mrg);
  char  command = 'm';
  char *s;
  int numbers = 0;
  double number[12];
  double pcx, pcy, cx, cy;

  if (!str)
    return -1;
  //ctx_move_to (ctx, 0, 0);
  //ctx_begin_path (ctx);
  cx = 0; cy = 0;
  pcx = cx; pcy = cy;

  s = (void*)str;
again:
  numbers = 0;

  for (; s && *s; s++)
  {
    switch (*s)
    {
      case 'z':
      case 'Z':
        pcx = cx; pcy = cy;
        ctx_close_path (ctx);
        break;
      case 'm':
      case 'a':
      case 's':
      case 'S':
      case 'M':
      case 'c':
      case 'C':
      case 'l':
      case 'L':
      case 'h':
      case 'H':
      case 'v':
      case 'V':
         command = *s;
         break;

      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      if (*s == '-')
      {
        number[numbers] = -strtod (s+1, &s);
        s--;
      }
      else
      {
        number[numbers] = strtod (s, &s);
        s--;
      }
      numbers++;

      switch (command)
      {
        case 'a':
          /* fallthrough */
        case 'A':
          if (numbers == 7)
          {
            /// XXX: NYI
            s++;
            goto again;
          }
          /* fallthrough */
        case 'm':
          if (numbers == 2)
          {
            ctx_rel_move_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'h':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, number[0], 0.0);
	    cx += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'v':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, 0.0, number[0]);
	    cy += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'l':
          if (numbers == 2)
          {
            ctx_rel_line_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'c':
          if (numbers == 6)
          {
            ctx_rel_curve_to (ctx, number[0], number[1],
                                    number[2], number[3],
                                    number[4], number[5]);
            pcx = cx + number[2];
	    pcy = cy + number[3];
	    cx += number[4];
	    cy += number[5];
            s++;
            goto again;
          }
          break;
	case 's':
          if (numbers == 4)
          {
            ctx_curve_to (ctx, 2 * cx - pcx, 2 * cy - pcy,
                                number[0] + cx, number[1] + cy,
                                number[2] + cx, number[3] + cy);
	    pcx = number[0] + cx;
	    pcy = number[1] + cy;
	    cx += number[2];
	    cy += number[3];
            s++;
            goto again;
          }
          break;
        case 'M':
          if (numbers == 2)
          {
            ctx_move_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'H':
          if (numbers == 1)
          {
            ctx_line_to (ctx, number[0], cy);
	    cx = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'V':
          if (numbers == 1)
          {
            ctx_line_to (ctx, cx, number[0]);
	    cy = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'L':
          if (numbers == 2)
          {
            ctx_line_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'C':
          if (numbers == 6)
          {
            ctx_curve_to (ctx, number[0], number[1],
                                number[2], number[3],
                                number[4], number[5]);
	    pcx = number[2];
	    pcy = number[3];
	    cx = number[4];
	    cy = number[5];
            s++;
            goto again;
          }
          break;
        case 'S':
          if (numbers == 4)
          {
            float ax = 2 * cx - pcx;
            float ay = 2 * cy - pcy;
            ctx_curve_to (ctx, ax, ay,
                                number[0], number[1],
                                number[2], number[3]);
	    pcx = number[0];
	    pcy = number[1];
	    cx = number[2];
	    cy = number[3];
            s++;
            goto again;
          }
          break;
        default:
          fprintf (stderr, "_%c", *s);
          break;
      }
      break;
      default:
        break;
    }
  }
  return 0;
}

static void
mrg_parse_polygon (Mrg *mrg, const char *str)
{
  Ctx *ctx = mrg_ctx (mrg);
  char *s;
  int numbers = 0;
  int started = 0;
  double number[12];

  if (!str)
    return;
  //ctx_move_to (ctx, 0, 0);

  s = (void*)str;
again:
  numbers = 0;

  for (; *s; s++)
  {
    switch (*s)
    {
      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      number[numbers] = _ctx_parse_float (s, &s);
      s--;
      numbers++;

      if (numbers == 2)
      {
        if (started)
          ctx_line_to (ctx, number[0], number[1]);
        else
        {
          ctx_move_to (ctx, number[0], number[1]);
          started = 1;
        }
        s++;
        goto again;
      }
      default:
        break;
    }
  }
}

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data);

int mrg_get_contents (Mrg         *mrg,
                      const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length);


void  mrg_text_listen (Mrg *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2);

void  mrg_text_listen_full (Mrg *mrg, CtxEventType types,
                            CtxCb cb, void *data1, void *data2,
          void (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
          void  *finalize_data);
void  mrg_text_listen_done (Mrg *mrg);

char *_mrg_resolve_uri (const char *base_uri, const char *uri);
typedef struct _MrgImage MrgImage;
struct _MrgImage
{
  char *uri;
  char *path;
  int width;
  int height;
  CtxBuffer *surface;
};

static CtxList *images = NULL;

static MrgImage *_mrg_image (Mrg *mrg, const char *path)
{
  char *uri =  _mrg_resolve_uri (mrg->uri_base, path);

  for (CtxList *l = images; l; l = l->next)
  {
    MrgImage *image = l->data;
    if (!strcmp (path, image->uri))
    {
       return image;
    }
  }

  int w = 0, h = 0;
#if 0
  char *p = strchr (uri, ':');
  if (p)
  {
    if (*p) p++;
    if (*p) p++;
    if (*p) p++;
  }
  else p = uri;
  ctx_texture_load (mrg->ctx, p, &w, &h, NULL);
#else
  ctx_texture_load (mrg->ctx, uri, &w, &h, NULL);
#endif

  if (w)
  {
    MrgImage *image = calloc (sizeof (MrgImage), 1);
    image->width = w;
    image->height = h;
    image->uri = strdup (path);
    image->path = strdup (uri);
    ctx_list_prepend (&images, image);
    free (uri);
    return _mrg_image (mrg, path);
  }
  free (uri);
  return NULL;
}

int mrg_query_image (Mrg        *mrg,
                     const char *path,
                     int        *width,
                     int        *height)
{
  MrgImage *image = _mrg_image (mrg, path);
  if (image)
  {
    *width = image->width;
    *height = image->height;
    return 1;
  }
  return 0;
}

void mrg_image (Mrg *mrg, float x0, float y0, float width, float height, float opacity, const char *path, int *used_width, int *used_height)
{
  MrgImage *image = _mrg_image (mrg, path);
  if (image)
  {
    ctx_draw_image (mrg->ctx, image->path, x0, y0, width, height);
  }


}

enum
{
  URI_STATE_IN_PROTOCOL = 0,
  URI_STATE_IN_HOST,
  URI_STATE_IN_PORT,
  URI_STATE_E_S1,
  URI_STATE_E_S2,
  URI_STATE_IN_PATH,
  URI_STATE_IN_FRAGMENT,
};

int split_uri (char *uri,
               char **protocol,
               char **host,
               char **port,
               char **path,
               char **fragment)
{
  char *p;
  *protocol =
  *host =
  *port =
  *path =
  *fragment = NULL;

  if (strstr (uri, "//") || strchr(uri, ':'))
  {
    int mr = URI_STATE_IN_PROTOCOL;

    if (protocol)
      *protocol = uri;

    if (uri[0] == '/' &&
        uri[1] == '/')
    {
      mr = URI_STATE_E_S1;
      *protocol = NULL;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
     {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              mr = URI_STATE_IN_HOST;
              if (path) *path = p;
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
         break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
    }
  }
  else
  {

    int mr = URI_STATE_IN_HOST;
    if (protocol)
      *protocol = NULL;

    if (uri[0]=='/')
    {
      if (host)
        *host = NULL;
      if (port)
        *port = NULL;
      *uri = '\0';
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri+1;
    }
    else
    {
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
      {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
         switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
   }
  }
  if (*protocol && (*protocol)[0] == 0)
    *protocol = NULL;
  return 0;
}

char *_mrg_resolve_uri (const char *base_uri, const char *uri)
{
  char *ret;
  char *uri_dup = strdup (uri);

  if (!base_uri)
    return uri_dup;

  char *base_dup = strdup (base_uri);

  char *protocol = NULL;
  char *host = NULL;
  char *port = NULL;
  char *path = NULL;
  char *fragment = NULL;
  char *base_protocol = NULL;
  char *base_host = NULL;
  char *base_port = NULL;
  char *base_path = NULL;
  char *base_fragment = NULL;

  //int retlen = 0;
  int samehost = 0;

  split_uri (uri_dup, &protocol, &host, &port, &path, &fragment);
  split_uri (base_dup, &base_protocol, &base_host, &base_port, &base_path, &base_fragment);

  if (!protocol)
    protocol = base_protocol;
  if (!host)
  {
    host = base_host;
    port = base_port;
    samehost = 1;
  }
  ret = malloc (
      (path?strlen (path):0)
      + (fragment?strlen (fragment):0) +
      (host?strlen (host):0) + 640);
  if (protocol)
  {
    if (uri[0] == '/' && uri[1] != '/')
      sprintf (ret, "%s://%s%s%s%s", protocol, host, port?":":"", port?port:"", uri);
    else if (uri[0] == '.' && uri[1] == '.' && uri[2] == '/' &&
             uri[3] == '.' && uri[4] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "c%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[6], fragment?"#":"", fragment?fragment:"");
    }
    else if (uri[0] == '.' && uri[1] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[3], fragment?"#":"", fragment?fragment:"");
    }
    else
   {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else if (ctx_strchr (base_path, '/'))
        ctx_strchr (base_path, '/')[1] = 0;
      else
        base_path[0] = 0;

      if (host)
      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");
      else
      sprintf (ret, "%s:%s%s%s%s", protocol, samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");

    }
  }
  else
  {
    if (uri[0] == '/')
      sprintf (ret, "/%s", path);
    else
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      sprintf (ret, "/%s%s", base_path, path);
    }
  }

  free (uri_dup);
  free (base_dup);
  return ret;
}

static float
_ctx_str_get_float (const char *string, int no)
{
  float ret = 0.0f;
  int number_no = 0;
  const char *s = string;
  if (!string) return ret;

  while (*s == ' ')s++;

  while (*s && number_no < no)
  {
     while ( *s && ((*s >= '0' && *s <= '9') || (*s=='.') || (*s=='-'))) s ++;
     number_no ++;
     while (*s == ' ')s++;
  }
  if (*s)
    return atof (s);

  return ret;
}

void itk_xml_render (Mrg *mrg,
                     char *uri_base,
                     void (*link_cb) (CtxEvent *event, void *href, void *link_data),
                     void *link_data,
                     void *(finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                     void *finalize_data,
                     char *html_)
{
  MrgXml *xmltok;
  uint32_t tag[CTX_MAX_STATE_DEPTH];
  int pos             = 0;
  int type            = t_none;
  static int depth    = 0;
  int in_style        = 0;
  int should_be_empty = 0;
  int tagpos          = 0;

  if (mrg->uri_base)
	  free (mrg->uri_base);
  mrg->uri_base = NULL;
  if (uri_base)
    mrg->uri_base = strdup (uri_base);

  CtxString *style = ctx_string_new ("");
  int whitespaces = 0;
  uint32_t att = 0;


////////////////////////////////////////////////////

  whitespaces = 0;
  att = 0;
#if 1
  xmltok = xmltok_buf_new (html_);

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);
    switch (type)
    {
      case t_word:
      case t_whitespace:
        if (in_style)
        {
          ctx_stylesheet_add (mrg, data, uri_base, CTX_STYLE_XML, NULL);
        }
        break;
      case t_endtag:
	{
        int i;
        for (i = 0; data[i]; i++)
          data[i] = tolower (data[i]);
        in_style = !strcmp (data, "style");
	if (in_style)
	{
          if (mrg->css_parse_state)
                  free (mrg->css_parse_state);
          mrg->css_parse_state = NULL;
	}
	}
        break;
      default:
        break;
    }
  }

  xmltok_free (xmltok);
#endif


////////////////////////////////////////////////////


  type = t_none;
  whitespaces = 0;
  att = 0;
  in_style = 0;
  xmltok = xmltok_buf_new (html_);

  {
    int no = mrg->text_listen_count;
    mrg->text_listen_data1[no] = link_data;
    mrg->text_listen_data2[no] = html_;
    mrg->text_listen_finalize[no] = (void*)finalize;
    mrg->text_listen_finalize_data[no] = finalize_data;
    mrg->text_listen_count++;
  }

  _mrg_set_wrap_edge_vfuncs (mrg, wrap_edge_left, wrap_edge_right, mrg);
  mrg->state = &mrg->states[0];

  //itk_start (mrg, "fjo", NULL);
  //ctx_stylesheet_add (mrg, style_sheets->str, uri_base, CTX_STYLE_XML, NULL);

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);

    if (type == t_tag ||
        type == t_att ||
        type == t_endtag ||
        type == t_closeemptytag ||
        type == t_closetag)
    {
      int i;
      for (i = 0; data[i]; i++)
        data[i] = tolower (data[i]);
    }

    switch (type)
    {
      case t_entity:
        {
          int i;
          int dealt_with = 0;
          if (data[0]=='#')
          {
            int c = atoi (&data[1]);
            itk_printf (mrg, "%c", c);
          }
          else
          {
            uint32_t hash = ctx_strhash (data);
          for (i = 0; entities[i].name && !dealt_with; i++)
            if (hash == entities[i].name)
            {
              itk_print (mrg, entities[i].value);
              dealt_with = 1;
            }
          }

          if (!dealt_with){
            itk_start (mrg, "dim", (void*)((size_t)pos));
            itk_print (mrg, data);
            itk_end (mrg, NULL);
          }
        }
        break;
      case t_word:
        if (in_style)
        {
          //ctx_stylesheet_add (mrg, data, uri_base, CTX_STYLE_XML, NULL);
        }
        else
        {
          itk_print (mrg, data);
        }
        whitespaces = 0;
        break;

      case t_whitespace:
        if (in_style)
        {
          //ctx_stylesheet_add (mrg, data, uri_base, CTX_STYLE_XML, NULL);
        }
        else
        {
          switch (ctx_style (mrg)->white_space)
          {
            case CTX_WHITE_SPACE_PRE: /* handles as pre-wrap for now */
            case CTX_WHITE_SPACE_PRE_WRAP:
              itk_print (mrg, data);
              break;
            case CTX_WHITE_SPACE_PRE_LINE:
              switch (*data)
              {
                case ' ':
                  whitespaces ++;
                  if (whitespaces == 1)
                    itk_print (mrg, " ");
                  break;
                case '\n':
                  whitespaces = 0;
                  break;
              }
              break;
            case CTX_WHITE_SPACE_NOWRAP: /* XXX: handled like normal, this is bad.. */
            case CTX_WHITE_SPACE_NORMAL: 
              whitespaces ++;
              if (whitespaces == 1)
	      {
		int save = mrg->unresolved_line;
                itk_print (mrg, " ");
		mrg->unresolved_line = save;
	      }
              break;
          }
        }
        break;
      case t_tag:
        //htmlctx->attributes = 0;
        //ctx_save (mrg->ctx);
        tagpos = pos;
        ctx_string_clear (style);
        ctx_set_string (mrg->ctx, SQZ_style, "");

	if (!strcmp (data, "html"))
	{
	}
        break;
      case t_att:
        //if (htmlctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
        //  strncpy (htmlctx->attribute[htmlctx->attributes], data, MRG_XML_MAX_ATTRIBUTE_LEN-1);
        att = ctx_strhash (data);
        break;
      case t_val:
        //if (htmlctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
        //  strncpy (htmlctx->value[htmlctx->attributes++], data, MRG_XML_MAX_VALUE_LEN-1);
        ctx_set_string (mrg->ctx, att, data);
        {
            uint32_t style_attribute[] ={
              SQZ_fill_rule,
              SQZ_font_size,
              SQZ_font_family,
              SQZ_fill_color,
              SQZ_fill,
              SQZ_stroke_width,
              SQZ_stroke_color,
              SQZ_stroke_linecap,
              SQZ_stroke_miterlimit,
              SQZ_stroke_linejoin,
              SQZ_stroke,
              // SQZ_viewBox,  // SQZ_version
              SQZ_color,
              SQZ_background_color,
              SQZ_background,
              0};
            char *style_attribute_names[] ={
              "fill_rule",
              "font_size",
              "font_family",
              "fill_color",
              "fill",
              "stroke_width",
              "stroke_color",
              "stroke_linecap",
              "stroke_miterlimit",
              "stroke_linejoin",
              "stroke",
              //"viewBox",
              "color",
              "background_color",
              "background",
              0};

              int j;
              for (j = 0; style_attribute[j]; j++)
                if (att == style_attribute[j])
                {
                  ctx_string_append_printf (style, "%s: %s;",
                      style_attribute_names[j], data);
                  break;
                }
          }
        break;
      case t_endtag:
        {
          uint32_t data_hash = ctx_strhash (data);
#if 0
	  int prev_is_self_closing = 0;

	  if (depth) switch (tag[depth-1])
	  {
            case SQZ_p:
            case SQZ_li:
            case SQZ_dt:
            case SQZ_dd:
            case SQZ_option:
            case SQZ_thead:
            case SQZ_tbody:
            case SQZ_head:
            case SQZ_tfoot:
            case SQZ_colgroup:
            case SQZ_th:
	      prev_is_self_closing = 1;
	      break;
	  }

	  if (prev_is_self_closing)
	  {
             int is_block = 0;
	     switch (data_hash)
	     {
	        case SQZ_tr:
	        case SQZ_li:
	        case SQZ_p:
	        case SQZ_div:
	        //case SQZ_td:
		  is_block = 1;
		  break;
	     }
	     if (is_block)
	     {
               itk_end (mrg, NULL);
               depth--;
	     }

	  }

#else

        if (depth && (data_hash == SQZ_tr && tag[depth-1] == SQZ_td))
        {
          itk_end (mrg, NULL);
          depth--;
          itk_end (mrg, NULL);
          depth--;
        }
        if (depth && (data_hash == SQZ_tr && tag[depth-1] == SQZ_td))
        {
          itk_end (mrg, NULL);
          depth--;
          itk_end (mrg, NULL);
          depth--;
        }
        else if (depth && ((data_hash == SQZ_dd && tag[depth-1] == SQZ_dt) ||
                      (data_hash == SQZ_li && tag[depth-1] == SQZ_li) ||
                      (data_hash == SQZ_dt && tag[depth-1] == SQZ_dd) ||
                      (data_hash == SQZ_td && tag[depth-1] == SQZ_td) ||
                      (data_hash == SQZ_tr && tag[depth-1] == SQZ_tr) ||
                      (data_hash == SQZ_dd && tag[depth-1] == SQZ_dd) ||
                      (data_hash == SQZ_p &&  tag[depth-1] == SQZ_p)))
        {
          itk_end (mrg, NULL);
          depth--;
        }
#endif

        tag[depth] = data_hash;
        depth ++;
	depth = ctx_mini(depth, CTX_MAX_STATE_DEPTH-1);

        {
          char combined[512]="";
          char *klass = (char*)PROPS(class);
          /* XXX: spaces in class should be turned into .s making
           * it possible to use multiple classes
           */
          const char *id = PROPS(id);

          const char *pseudo = "";

          if (klass)
          {
            klass = strdup (klass);
            if(1){
              int i;
              for (i = 0; klass[i]; i++)
                if (klass[i] == ' ')
                  klass[i]='.';
            }
          }

	  combined[511]=0;
          snprintf (combined, 511, "%s%s%s%s%s%s",
              data,
              klass?".":"",
              klass?klass:"",
              id?"#":"",
              id?id:"", pseudo);

	  //fprintf (stderr,"[%s]", combined);
          if (klass)
            free (klass);
            /* collect XML attributes and convert into CSS declarations */
          ctx_string_append_str (style, PROPS(style));
          itk_start_with_style (mrg, combined, (void*)((size_t)tagpos), style->str);
        }

        if (data_hash == SQZ_g)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              mrg_parse_transform (mrg, &matrix, transform);
              ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
        }

        else if (data_hash == SQZ_svg)
        {
          const char *vbox = PROPS(viewbox);
          if (vbox)
          {
            float x = _ctx_str_get_float (vbox, 0);
            float y = _ctx_str_get_float (vbox, 1);
            float width = _ctx_str_get_float (vbox, 2);
            float height = _ctx_str_get_float (vbox, 3);
            //fprintf (stderr, "viewBox:%s   %f %f %f %f\n", vbox, x, y, width, height);
            ctx_view_box (mrg->ctx, x, y, width, height);
          }
        }

        else if (data_hash == SQZ_polygon)
        {
          mrg_parse_polygon (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == SQZ_path)
        {
          mrg_parse_svg_path (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == SQZ_rect)
        {
          float width  = PROP(width);
          float height = PROP(height);
          float x      = PROP(x);
          float y      = PROP(y);

          ctx_rectangle (mrg_ctx (mrg), x, y, width, height);
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == SQZ_text)
        {
          mrg->x = PROP(x);
          mrg->y = PROP(y);
        }

        if (data_hash == SQZ_a)
        {
          if (link_cb && ctx_is_set_now (mrg->ctx, SQZ_href))
            mrg_text_listen_full (mrg, CTX_CLICK, link_cb, _mrg_resolve_uri (uri_base, ctx_get_string (mrg->ctx, SQZ_href)), link_data, (void*)free, NULL); //XXX: free is not invoked according to valgrind
        }

        else if (data_hash == SQZ_style)
        {
          in_style = 1;
#if 1
          if (mrg->css_parse_state)
                  free (mrg->css_parse_state);
          mrg->css_parse_state = NULL;
#endif
        }
        else
          in_style = 0;

        should_be_empty = 0;

        if (data_hash == SQZ_link)
        {
          const char *rel;
          if ((rel=PROPS(rel)) && !strcmp (rel, "stylesheet") && ctx_is_set_now (mrg->ctx, SQZ_href))
          {
            char *contents;
            long length;
            mrg_get_contents (mrg, uri_base, ctx_get_string (mrg->ctx, SQZ_href), &contents, &length);
            if (contents)
            {
              ctx_stylesheet_add (mrg, contents, uri_base, CTX_STYLE_XML, NULL);
              free (contents);
            }
          }
        }

        if (data_hash == SQZ_img && ctx_is_set_now (mrg->ctx, SQZ_src))
        {
          int img_width, img_height;
          const char *src = ctx_get_string (mrg->ctx, SQZ_src);

          if (mrg_query_image (mrg, src, &img_width, &img_height))
          {
            float width  = PROP(width);
            float height = PROP(height);

            if (width < 1)
            {
               width = img_width;
            }
            if (height < 1)
            {
               height = img_height *1.0 / img_width * width;
            }

            mrg->y += height;

            mrg_image (mrg,
            mrg->x,
            mrg->y - height,
            width,
            height,
            1.0f,
            src, NULL, NULL);

            mrg->x += width;
          }
          else
          {
            itk_printf (mrg, "![%s]", src);
          }
        }
#if 1
        switch (data_hash)
        {
          case SQZ_link:
          case SQZ_meta:
          case SQZ_input:
          case SQZ_img:
          case SQZ_br:
          case SQZ_hr:
            should_be_empty = 1;
            itk_end (mrg, NULL);
            depth--;
        }
#endif
        }
        break;

      case t_closeemptytag:
      case t_closetag:
        if (!should_be_empty)
        {
          uint32_t data_hash = ctx_strhash (data);
          if (!strcmp (data, "a"))
          {
            mrg_text_listen_done (mrg);
          }
          in_style = 0;
          itk_end (mrg, NULL);
          //ctx_restore (mrg->ctx);
          depth--;

          if (depth<0)depth=0; // XXX
#if 1
          if (tag[depth] != data_hash)
          {
            if (tag[depth] == SQZ_p)
            {
              itk_end (mrg, NULL);
              depth --;
            } else 
            if (depth > 0 && tag[depth-1] == data_hash)
            {
              itk_end (mrg, NULL);
              depth --;
            }
            else if (depth > 1 && tag[depth-2] == data_hash)
            {
              int i;
              for (i = 0; i < 2; i ++)
              {
                depth --;
                itk_end (mrg, NULL);
                //nctx_restore (mrg->ctx);
              }
            }
#if 0
            else if (depth > 2 && tag[depth-3] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wass open\n", pos, data, tag[depth]);

              for (i = 0; i < 3; i ++)
              {
                depth --;
                itk_end (mrg, NULL);
               // ctx_restore (mrg->ctx);
              }
            }
            else if (depth > 3 && tag[depth-3] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wasss open\n", pos, data, tag[depth]);

              for (i = 0; i < 4; i ++)
              {
                depth --;
                itk_end (mrg, NULL);
                //ctx_restore (mrg->ctx);
              }
            }
            else if (depth > 4 && tag[depth-5] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wassss open\n", pos, data, tag[depth]);

              for (i = 0; i < 5; i ++)
              {
                depth --;
                itk_end (mrg, NULL);
              }
            }
#endif
            else
            {
              if (data_hash == SQZ_table && tag[depth] == SQZ_td)
              {
                depth--;
                itk_end (mrg, NULL);
                depth--;
                itk_end (mrg, NULL);
              }
              else if (data_hash == SQZ_table && tag[depth] == SQZ_tr)
              {
                depth--;
                itk_end (mrg, NULL);
              }
            }
          }
#endif
        }
        break;
    }
  }
  //itk_end (mrg,  NULL);

  xmltok_free (xmltok);

  if (depth!=0){
    fprintf (stderr, "html parsing unbalanced, %i open tags.. \n", depth);
    while (depth > 0)
    {
      fprintf (stderr, " %s ", ctx_str_decode (tag[depth-1]));
      itk_end (mrg, NULL);
      depth--;
    }
    fprintf (stderr, "\n");
  }

  ctx_string_free (style, 1);

  if (mrg->absolute_ctx)
  {
     ctx_render_ctx (mrg->absolute_ctx, mrg->ctx);
     ctx_destroy (mrg->absolute_ctx);
  }
  if (mrg->fixed_ctx)
  {
     ctx_render_ctx (mrg->fixed_ctx, mrg->ctx);
     ctx_destroy (mrg->fixed_ctx);
  }
}

void itk_xml_renderf (Mrg *mrg,
                      char *uri_base,
                      void (*link_cb) (CtxEvent *event, void *href, void *link_data),
                      void *link_data,
                      char *format,
                      ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  itk_xml_render (mrg, uri_base, link_cb, link_data, NULL, NULL, buffer);
  free (buffer);
}
void itk_css_init (Mrg *mrg, Ctx *ctx, int width, int height);

void itk_print_xml (Mrg *mrg, const char *xml)
{
  itk_xml_render (mrg, NULL, NULL, NULL, NULL, NULL, (char*)xml);
}

void
itk_printf_xml (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  itk_print_xml (mrg, buffer);
  free (buffer);
}

void mrg_set_size (Mrg *mrg, int width, int height)
{
  if (ctx_width (mrg->ctx) != width ||
      ctx_height (mrg->ctx) != height)
  {
    ctx_set_size (mrg->ctx, width, height);
    mrg_queue_draw (mrg, NULL);
  }
}

int
_mrg_file_get_contents (const char  *path,
                        char       **contents,
                        long        *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;

  file = fopen (path,"rb");

  if (!file)
    return -1;

  if (!strncmp (path, "/proc", 4))
  {
    buffer = calloc(2048, 1);
    *contents = buffer;
    *length = fread (buffer, 1, 2047, file);
    buffer[*length] = 0;
    return 0;
  }
  else
  {
    fseek (file, 0, SEEK_END);
    *length = size = remaining = ftell (file);
    rewind (file);
    buffer = malloc(size + 8);
  }

  if (!buffer)
    {
      fclose(file);
      return -1;
    }

  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = buffer;
  buffer[size] = 0;
  return 0;
}

#if 0
static int
_mr_get_contents (const char  *referer,
                 const char  *uri,
                 char       **contents,
                 long        *length)
{
  char *uri_dup;
  char *protocol = NULL;
  char *host = NULL;
  char *port = NULL;
  char *path = NULL;
  char *fragment = NULL;
  uint32_t protocol_hash;
#if 0
  if (!strncmp (uri, "mrg:", 4))
  {
    return _mrg_internal_get_contents (referer, uri, contents, length);
  }
#endif

  uri_dup = strdup (uri);
  split_uri (uri_dup, &protocol, &host, &port, &path, &fragment);
  protocol_hash = protocol?ctx_strhash (protocol):0;
#if 0
  if (protocol && protocol_hash == SQZ_http)
  {
    int len;
    char *pathdup = malloc (strlen (path) + 2);
    pathdup[0] = '/';
    strcpy (&pathdup[1], path);
   // fprintf (stderr, "%s %i\n",host, port?atoi(port):80);
    char *cont = _mrg_http (NULL, host, port?atoi(port):80, pathdup, &len);
    *contents = cont;
    *length = len;
    //fprintf (stderr, "%s\n", cont);
    //
    fprintf (stderr, "%i\n", len);
    free (uri_dup);
    return 0;
  } else
#endif
  if (protocol && protocol_hash == SQZ_file)
  {
    char *path2 = malloc (strlen (path) + 2);
    int ret;
    sprintf (path2, "/%s", path);
    ret = ctx_get_contents (path2, (uint8_t**)contents, length);

    free (path2);
    free (uri_dup);
    fprintf (stderr, "a%i\n", (int)*length);
    return ret;
  }
  else
  {
    char *c = NULL;
    long  l = 0;
    int ret;
    free (uri_dup);
    ret = _mrg_file_get_contents (uri, &c, &l);
    if (contents) *contents = c;
    if (length) *length = l;
    return ret;
  }

  return -1;
}
#endif


typedef struct _CacheEntry {
  char *uri;
  long  length;
  char *contents;
} CacheEntry;

static CtxList *cache = NULL;


#ifdef ITK_HAVE_FS

/* we define it here, but it is actually only used from within ctx
 * when ITK_HAVE_FS is also defined for that compilation
 */
int itk_static_get_contents (const char *path, char **contents, long *length)
{
   if (!strncmp (path, "itk:", 4)) path += 4;
   if (path[0] == '/') path++;
   for (int i = 0; itk_fs[i].uri; i++)
   {
     if (!strcmp (path, itk_fs[i].uri))
     {
	*contents = malloc(itk_fs[i].length);
	memcpy (*contents, itk_fs[i].data, itk_fs[i].length);
	/// XXX: eeek why need a copy?
	*length = itk_fs[i].length;
        return 0;
     }
   }
   return -1;
}
#endif

/* caching uri fetcher
 */
int
mrg_get_contents_default (const char  *referer,
                          const char  *input_uri,
                          char       **contents,
                          long        *length,
                          void        *ignored_user_data)
{
  Mrg *mrg = ignored_user_data;
  char *uri =  _mrg_resolve_uri (mrg->uri_base, input_uri);
#if 0 // without caching
  int ret = 0;
  ctx_get_contents (uri, (uint8_t**)contents, length);
  free (uri);
  return *contents!=NULL;
#else
  /* should resolve before mrg_get_contents  */
  //fprintf (stderr, "%s %s\n", uri, input_uri);

  for (CtxList *i = cache; i; i = i->next)
  {
    CacheEntry *entry = i->data;
    if (!strcmp (entry->uri, uri))
    {
      if (!entry->contents)
	{
	  *contents = NULL;
          if (length) *length = 0;
  free (uri);
          return -1;
	}

      *contents = malloc (entry->length + 1);
      memcpy (*contents, entry->contents, entry->length);
      (*contents)[entry->length]=0;
      free (uri);
      if (length)
	*length = entry->length;
      if (*length)
      {
        return 0;
      }
      else
      {
        return -1;
      }
    }
  }

  {
    CacheEntry *entry = calloc (sizeof (CacheEntry), 1);
    char *c = NULL;
    long  l = 0;

    entry->uri = uri;

    ctx_get_contents (uri, (unsigned char**)&c, &l);
   
    if (c){
      entry->contents = c;
      entry->length = l;
    } else
    {
      entry->contents = NULL;
      entry->length = 0;
    }
    ctx_list_prepend (&cache, entry);

#if MRG_URI_LOG
    if (c)
      fprintf (stderr, "%li\t%s\n", l, uri);
    else
      fprintf (stderr, "FAIL\t%s\n", uri);
#endif
  }

  int ret = mrg_get_contents_default (referer, uri, contents, length, ignored_user_data);
  return ret;
#endif
}

void itk_css_init (Mrg *mrg, Ctx *ctx, int width, int height)
{
  //memset (mrg, 0, sizeof (Mrg));
  mrg->do_clip = 1;
  mrg->ctx = mrg->document_ctx = ctx;
  _ctx_events_init (mrg->ctx);
  mrg->state_no = 0;
  mrg->line_level=0;
  mrg->line_max_height[0]=0.0f;
  mrg->state = &mrg->states[mrg->state_no];
  mrg->state->float_base = 0;

  mrg->floats = 0;
#if 0
  mrg->state->children = 0;
  mrg->state->overflowed = 0;
  mrg->state->span_bg_started = 0;
#endif
  //memset (mrg->state, 0, sizeof (MrgState));
  //memset (mrg->states, 0, sizeof (mrg->states));
  /* XXX: is there a better place to set the default text color to black? */
#if 0
  mrg->state->style.color.red =
  mrg->state->style.color.green =
  mrg->state->style.color.blue = 0;
  mrg->state->style.color.alpha = 1;
#endif
  if(1){
    CtxColor *color = ctx_color_new ();
    ctx_color_set_rgba (ctx_get_state (mrg->ctx), color, 1, 1, 1, 1);
    ctx_set_color (mrg->ctx, SQZ_color, color);
    ctx_color_free (color);
  }

  mrg->ddpx = 1;
  if (getenv ("MRG_DDPX"))
  {
    mrg->ddpx = strtod (getenv ("MRG_DDPX"), NULL);
  }
  mrg_set_size (mrg, width, height);
  _mrg_text_init (mrg);

  mrg->style = ctx_string_new ("");

  mrg_set_mrg_get_contents (mrg, mrg_get_contents_default, mrg);
  mrg->style_global = ctx_string_new ("");


  if(0){
    const char *global_css_uri = "mrg:theme.css";

    if (getenv ("MRG_CSS"))
      global_css_uri = getenv ("MRG_CSS");

    char *contents;
    long length;
    mrg_get_contents (mrg, NULL, global_css_uri, &contents, &length);
    if (contents)
    {
      ctx_string_set (mrg->style_global, contents);
      free (contents);
    }
  }

  itk_stylesheet_clear (mrg);
  _mrg_clear_text_closures (mrg);
}

Mrg *mrg_new (Ctx *ctx, int width, int height)
{
  Mrg *mrg;

  mrg = calloc (sizeof (Mrg), 1);
  mrg->do_clip = 1;
  itk_css_init (mrg, ctx, width, height);
  ctx_style_defaults (mrg);

#if 0
  printf ("%f %i %i\n", mrg->state->style.font_size, mrg_width(mrg), mrg_height(mrg));
  printf ("sizeof(Mrg) %li (was: 1142496)\n", sizeof(Mrg));
  printf ("sizeof(MrgState) %li\n", sizeof(MrgState));
  printf ("sizeof(CtxStyle) %li\n", sizeof(CtxStyle));
  printf ("sizeof(MrgHtml) %li\n", sizeof(MrgHtml));
  printf ("sizeof(CtxCssParseState) %li\n", sizeof(CtxCssParseState));
#endif

  return mrg;
}

void mrg_destroy (Mrg *mrg)
{
  if (mrg->edited_str)
    ctx_string_free (mrg->edited_str, 1);
  mrg->edited_str = NULL;
  free (mrg);
}
#endif
