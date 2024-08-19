#if 0
#include "local.conf"
#include "ctx.h"

#endif

#if CTX_CSS
//#include "squoze/squoze.h"

//#include "static.inc"

//#include "itk.h"   // for completeness, itk wants to be built in the ctx
                   // compilation unit to be influenced by the ctx config



#define CTX_MAX_STYLE_DEPTH  CTX_MAX_STATES
#define CTX_MAX_STATE_DEPTH  CTX_MAX_STATES

#if 0
#define CTX_MAX_FLOATS           16
#define CTX_MAX_SELECTOR_LENGTH  64
#define CTX_MAX_CSS_STRINGLEN    512
#define CTX_MAX_CSS_RULELEN      32   // XXX doesnt have overflow protection
#define CTX_MAX_CSS_RULES        128

/* other important maximums */
#define CTX_MAX_TEXT_LISTEN      256
#define CTX_XML_INBUF_SIZE       1024
#else

#define CTX_MAX_FLOATS           3
#define CTX_MAX_SELECTOR_LENGTH  64
#define CTX_MAX_CSS_STRINGLEN    256
#define CTX_MAX_CSS_RULELEN      32   // XXX doesnt have overflow protection
#define CTX_MAX_CSS_RULES        64

/* other important maximums */
#define CTX_MAX_TEXT_LISTEN      16
#define CTX_XML_INBUF_SIZE       256 
#endif

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

typedef struct _Css          Css;


void mrg_clear (Css *mrg);

static void mrg_queue_draw (Css *mrg, CtxIntRectangle *rect)
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

typedef void (*CssNewText)       (const char *new_text, void *data);
typedef void (*UiRenderFun)      (Css *mrg, void *ui_data);

typedef struct _Css Css;

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
} CssCursor;


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
  CssCursor           cursor:6;
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

typedef struct CssState {
  CtxStyleNode style_node;

  float        original_x;
  float        original_y;
  float        block_start_x;
  float        block_start_y;
  float        ptly;
  float        vmarg;
  int          flow_root; // is
  int          float_base;

  float      (*wrap_edge_left)  (Css *mrg, void *data);
  float      (*wrap_edge_right) (Css *mrg, void *data);
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
  unsigned int overflowed:1;
  unsigned int span_bg_started:1;

  int          drawlist_start_offset;
} CssState;

typedef struct _CssAbsolute CssAbsolute;

struct _CssAbsolute {
  int       z_index;
  int       fixed;
  float     top;
  float     left;
  float     relative_x;
  float     relative_y;
  CtxEntry *entries;
  int       count; 
};


struct _Css {
  Ctx             *ctx;
  Ctx            *document_ctx;
  Ctx            *fixed_ctx;
  Ctx            *absolute_ctx;
  float            rem;
  float            ddpx;
  int              in_svg;
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
  CssState        *state;
  CssState         states[CTX_MAX_STATE_DEPTH];
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
  CssNewText       update_string;
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

  // the following used to be the original Css struct

  int (*ui_fun)(Css *itk, void *data);
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
  CssPanel *panel;
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

float css_panel_scroll (Css *itk);

static Ctx *mrg_ctx (Css *mrg)
{
  return mrg->ctx;
}


/* XXX: stopping sibling grabs should be an addtion to stop propagation,
 * this would permit multiple events to co-register, and use that
 * to signal each other,.. or perhaps more coordination is needed
 */
void _mrg_clear_text_closures (Css *mrg)
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
mrg_get_contents (Css         *mrg,
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

typedef struct _Css    Css;
typedef struct _CssXml CssXml;

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

CssXml *xmltok_new     (FILE *file_in);
CssXml *xmltok_buf_new (char *membuf);
void    xmltok_free    (CssXml *t);
int     xmltok_lineno  (CssXml *t);
int     xmltok_get     (CssXml *t, char **data, int *pos);

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

struct _CssXml
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
nextchar (CssXml *t)
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
xmltok_get (CssXml *t, char **data, int *pos)
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

CssXml *
xmltok_new (FILE * file_in)
{
  CssXml *ret;

  ret = calloc (1, sizeof (CssXml));
  ret->file_in = file_in;
  ret->state = s_start;
  ret->curtag = ctx_string_new ("");
  ret->curdata = ctx_string_new ("");
  ret->inbuf = calloc (1, CTX_XML_INBUF_SIZE);
  return ret;
}

CssXml *
xmltok_buf_new (char *membuf)
{
  CssXml *ret;

  ret = calloc (1, sizeof (CssXml));
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
xmltok_free (CssXml *t)
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
xmltok_lineno (CssXml *t)
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

void mrg_clear (Css *mrg)
{
  ctx_events_clear (mrg->ctx);
  _mrg_clear_text_closures (mrg);
}

void css_set_edge_right (Css *mrg, float val);
void css_set_edge_left (Css *mrg, float val);
void css_set_edge_top (Css *mrg, float val);
void css_set_edge_bottom (Css *mrg, float val);
float mrg_edge_right (Css *mrg);
float mrg_edge_left (Css *mrg);
float mrg_y (Css *mrg);
float mrg_x (Css *mrg);

float mrg_em (Css *mrg);
void mrg_set_xy (Css *mrg, float x, float y);

static float _mrg_dynamic_edge_right2 (Css *mrg, CssState *state)
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

static float _mrg_dynamic_edge_left2 (Css *mrg, CssState *state)
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

static float _mrg_parent_dynamic_edge_left (Css *mrg)
{
  CssState *state = mrg->state;
  if (mrg->state_no)
    state = &mrg->states[mrg->state_no-1];
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float _mrg_parent_dynamic_edge_right (Css *mrg)
{
  CssState *state = mrg->state;
  if (mrg->state_no)
    state = &mrg->states[mrg->state_no-1];
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static float _mrg_dynamic_edge_left (Css *mrg)
{
  if (mrg->state->wrap_edge_left)
    return mrg->state->wrap_edge_left (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_left;
}
float mrg_width (Css *mrg)
{
  if (!mrg) return 640;
  return ctx_width (mrg->ctx) / mrg->ddpx;
}

float mrg_height (Css *mrg)
{
  if (!mrg) return 480;
  return ctx_height (mrg->ctx) / mrg->ddpx;
}

static float _mrg_dynamic_edge_right (Css *mrg)
{
  if (mrg->state->wrap_edge_right)
    return mrg->state->wrap_edge_right (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_right;
}

static float wrap_edge_left (Css *mrg, void *data)
{
  CssState *state = mrg->state;
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float wrap_edge_right (Css *mrg, void *data)
{
  CssState *state = mrg->state;
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static void clear_left (Css *mrg)
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

static void clear_right (Css *mrg)
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
CtxStyle *ctx_style (Css *mrg)
{
  return &mrg->state->style;
}

static void clear_both (Css *mrg)
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

static float mrg_edge_bottom  (Css *mrg)
{
  return mrg->state->edge_bottom;
}

static float mrg_edge_top  (Css *mrg)
{
  return mrg->state->edge_top;
}

float mrg_edge_left  (Css *mrg)
{
  return mrg->state->edge_left;
}

float mrg_edge_right (Css *mrg)
{
  return mrg->state->edge_right;
}

float _mrg_dynamic_edge_right (Css *mrg);
float _mrg_dynamic_edge_left (Css *mrg);

void css_set_edge_top (Css *itk, float edge)
{
  Css *mrg = itk;
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

void  css_set_edge_left (Css *itk, float val)
{
  itk->state->edge_left = val;
  itk->edge_left = val;
  itk->edge_right = itk->width + itk->edge_left;
}

void css_set_edge_bottom (Css *itk, float edge)
{
   itk->state->edge_bottom = edge;
   itk->edge_bottom = edge;
   itk->height = itk->edge_bottom - itk->edge_top;
}

void css_set_edge_right (Css *itk, float edge)
{
   itk->state->edge_right = edge;
   itk->edge_right = edge;
   itk->width      = itk->edge_right - itk->edge_left;
}

float mrg_rem (Css *mrg)
{
  return mrg->rem;
}


void css_start            (Css *mrg, const char *class_name, void *id_ptr);
void css_start_with_style (Css        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style);
void css_start_with_stylef (Css *mrg, const char *style_id, void *id_ptr,
                            const char *format, ...);

static void ctx_parse_style_id (Css          *mrg,
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

void _ctx_initial_style (Css *mrg)
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
  //SET_PROP(width, 42);

  SET_PROPS(class,"");
  SET_PROPS(id,"");

  //ctx_set_float (mrg->ctx, SQZ_stroke_width, 4.0);

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

static void ctx_css_parse_selector (Css *mrg, const char *selector, CtxStyleEntry *entry)
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

static void ctx_stylesheet_add_selector (Css *mrg, const char *selector, const char *css, int priority)
{
  CtxStyleEntry *entry = calloc (1, sizeof (CtxStyleEntry));
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


static void _ctx_stylesheet_add (CtxCssParseState *ps, Css *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  const char *p;
  if (!ps)
    ps = mrg->css_parse_state = calloc (1, sizeof (CtxCssParseState));

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

void ctx_stylesheet_add (Css *mrg, const char *css, const char *uri_base,
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

void css_css_default (Css *mrg)
{
  char *error = NULL;
  ctx_stylesheet_add (mrg, html_css, NULL, CTX_STYLE_INTERNAL, &error);
  if (error)
  {
    fprintf (stderr, "Css css parsing error: %s\n", error);
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

void css_stylesheet_clear (Css *mrg)
{
  if (mrg->stylesheet)
    ctx_list_free (&mrg->stylesheet);
  css_css_default (mrg);
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

int _ctx_child_no (Css *mrg)
{
  return mrg->states[mrg->state_no-1].children;
}

static inline int match_nodes (Css *mrg, CtxStyleNode *sel_node, CtxStyleNode *subject)
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

static int ctx_selector_vs_ancestry (Css *mrg,
                                     CtxStyleEntry *entry,
                                     CtxStyleNode **ancestry,
                                     int a_depth)
{
  int s = entry->sel_len - 1;
  //a_depth = mrg->state_no + 1;
#if 1

#if 0
  for (int i = 1; i < a_depth; i++)
  {
	  if (ancestry[i] != &mrg->states[i].style_node)
	  {
	    fprintf (stderr, "%i %p!=%p %i \n", i, ancestry[i],
	  		  &mrg->states[i].style_node, a_depth);
	    fprintf (stderr, "%p\n", &mrg->states[mrg->state_no].style_node);
	    fprintf (stderr, "%p\n", &mrg->states[mrg->state_no+1].style_node);
	    fprintf (stderr, "%p\n", &mrg->states[mrg->state_no-1].style_node);
	  }
  }
#endif

  if (entry->parsed[s].direct_descendant == 0)
  {
    /* right most part of selector must match */
    if (a_depth == 1)
    {
      // XXX it is an oddity how we need to use this instead of mrg->states[0].style_node directly..
      if (!match_nodes (mrg, &entry->parsed[s], ancestry[a_depth-1]))
      //if (!match_nodes (mrg, &entry->parsed[s], &mrg->states[0].style_node))
        return 0;
    }
    else
    {
      if (!match_nodes (mrg, &entry->parsed[s], &mrg->states[a_depth-1].style_node))
        return 0;
    }

    s--;
    a_depth--;
  }

  if (s < 0 || a_depth < 0)
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
        if (s >0 && ai >0 && match_nodes (mrg, &entry->parsed[s], &mrg->states[ai].style_node) &&
            match_nodes (mrg, &entry->parsed[s-1], &mrg->states[ai-1].style_node))
          found_node = 1;
      }
      ai--;
      s-=2;
    }
    else
    {
      for (ai = a_depth-1; ai >= 0 && !found_node; ai--)
      {
        if (match_nodes (mrg, &entry->parsed[s], &mrg->states[ai].style_node))
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

static int ctx_css_selector_match (Css *mrg, CtxStyleEntry *entry, CtxStyleNode **ancestry, int a_depth)
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

static char *_ctx_css_compute_style (Css *mrg, CtxStyleNode **ancestry, int a_depth)
{
  CtxList *l;
  CtxList *matches = NULL;
  int totlen = 2;
  char *ret = NULL;

  for (l = mrg->stylesheet; l; l = l->next)
  {
    CtxStyleEntry *entry = l->data;
    int score  = ctx_css_selector_match (mrg, entry, ancestry, a_depth);
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

static void _ctx_get_ancestry (Css *mrg, CtxStyleNode **ancestry)
{
  if (mrg->state_no>0)
      ancestry[0] = &mrg->states[0].style_node;
}

char *_ctx_stylesheet_collate_style (Css *mrg)
{
  CtxStyleNode *ancestry[1];
  _ctx_get_ancestry (mrg, ancestry);
  char *ret = _ctx_css_compute_style (mrg, ancestry, mrg->state_no + 1);
  return ret;
}

void  mrg_set_line_height (Css *mrg, float line_height)
{
  ctx_style (mrg)->line_height = line_height;
}

float mrg_line_height (Css *mrg)
{
  return ctx_style (mrg)->line_height;
}

void  mrg_set_rem         (Css *mrg, float em)
{
  mrg->rem = em;
}

float mrg_em (Css *mrg)
{
  return mrg->state->style.font_size;
}

void  mrg_set_em (Css *mrg, float em)
{
  mrg->state->style.font_size = em;
}

void ctx_css_set (Css *mrg, const char *css)
{
  ctx_string_set (mrg->style, css);
}

void ctx_css_add (Css *mrg, const char *css)
{
  ctx_string_append_str (mrg->style, css);
}

void css_stylesheet_clear (Css *mrg);
void ctx_stylesheet_add (Css *mrg, const char *css, const char *uri,
                         int priority,
                         char **error);

void ctx_css_set (Css *mrg, const char *css);
void ctx_css_add (Css *mrg, const char *css);

static inline float mrg_parse_px_x (Css *mrg, const char *str, char **endptr)
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

static inline float mrg_parse_px_y (Css *mrg, const char *str, char **endptr)
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

static inline int mrg_parse_pxs (Css *mrg, const char *str, float *vals)
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


static inline void ctx_css_handle_property_pass0 (Css *mrg, uint32_t key,
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

static void ctx_css_handle_property_pass1 (Css *mrg, uint32_t key,
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
    case SQZ_width:     // handled in pass1m
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
      // XXX : keep track of what we have set - so we at least do not
      // keep re-setting it..
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
    case SQZ_text_anchor:
      switch (val_hash)
      {
        case SQZ_start:   s->text_align = CTX_TEXT_ALIGN_START;   break;
        case SQZ_end:     s->text_align = CTX_TEXT_ALIGN_END;     break;
        case SQZ_left:    s->text_align = CTX_TEXT_ALIGN_LEFT;    break;
        case SQZ_right:   s->text_align = CTX_TEXT_ALIGN_RIGHT;   break;
        case SQZ_justify: s->text_align = CTX_TEXT_ALIGN_JUSTIFY; break;
        case SQZ_middle:
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

static void ctx_css_handle_property_pass1med (Css *mrg, uint32_t key,
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

static void css_parse_properties (Css *mrg, const char *style,
  void (*handle_property) (Css *mrg, uint32_t key,
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
#if 0
	  case '-':
            name[name_l++]='_';
            name[name_l]=0;
            break;
#endif
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
#if 0
            for (int i = 0; name[i];i++)
              if (name[i]=='-')name[i]='_';
#endif
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
  {
#if 0
    for (int i = 0; name[i];i++)
      if (name[i]=='-')
	name[i]='_';
#endif
    handle_property (mrg, ctx_strhash (name), string);
  }
}


static void ctx_css_handle_property_pass2 (Css *mrg, uint32_t key,
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


static float deco_width (Css *mrg)
{
  return PROP (padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width);
}

void css_set_style (Css *mrg, const char *style)
{
  CtxStyle *s;

  css_parse_properties (mrg, style, ctx_css_handle_property_pass0);
  css_parse_properties (mrg, style, ctx_css_handle_property_pass1);
  css_parse_properties (mrg, style, ctx_css_handle_property_pass1med);

  s = ctx_style (mrg);

  if (!mrg->in_svg)
  {
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

  }

  css_parse_properties (mrg, style, ctx_css_handle_property_pass2);
}

void _css_set_style_properties (Css *mrg, const char *style_properties)
{
  _ctx_initial_style (mrg);

  if (style_properties)
  {
    css_set_style (mrg, style_properties);
  }
}

void
css_set_stylef (Css *mrg, const char *format, ...)
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
  css_set_style (mrg, buffer);
  free (buffer);
}

void ctx_style_defaults (Css *mrg)
{
  Ctx *ctx = mrg->ctx;
  float em = 32;
  mrg_set_em (mrg,  em);
  mrg_set_rem (mrg, em);
  css_set_edge_left (mrg, 0);
  css_set_edge_right (mrg, mrg_width (mrg));
  css_set_edge_bottom (mrg, mrg_height (mrg));
  css_set_edge_top (mrg, 0);
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

  css_stylesheet_clear (mrg);
  _ctx_initial_style (mrg);

  if (mrg->style_global->length)
  {
    ctx_stylesheet_add (mrg, mrg->style_global->str, NULL, CTX_STYLE_GLOBAL, NULL);
  }

  if (mrg->style->length)
    ctx_stylesheet_add (mrg, mrg->style->str, NULL, CTX_STYLE_GLOBAL, NULL);
}

void mrg_set_mrg_get_contents (Css *mrg,
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
mrg_get_contents (Css         *mrg,
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

int css_print (Css *mrg, const char *string);
static int is_block_item (CtxStyle *style)
{
  return ((style->display == CTX_DISPLAY_BLOCK
           ||style->float_
           ||style->display == CTX_DISPLAY_LIST_ITEM
           ||style->display == CTX_DISPLAY_FLOW_ROOT
           ||style->display == CTX_DISPLAY_INLINE_BLOCK
	   ));
}

float mrg_ddpx (Css *mrg)
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

static void _mrg_nl (Css *mrg)
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

void _mrg_layout_pre (Css *mrg)
{
  CtxStyle *style;
  float dynamic_edge_left, dynamic_edge_right;


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

  // extra box dimensions once, reducing overhead of property storage
  //
  float padding_left = PROP(padding_left);
  float margin_left = PROP(margin_left);
  float border_left_width = PROP(border_left_width);
  float padding_right = PROP(padding_right);
  float margin_right = PROP(margin_right);
  float border_right_width = PROP(border_right_width);
  float padding_top = PROP(padding_top);
  float margin_top = PROP(margin_top);
  float border_top_width = PROP(border_top_width);
  float padding_bottom = PROP(padding_bottom);
  //float margin_bottom = PROP(margin_bottom);
  float border_bottom_width = PROP(border_bottom_width);

  float left = PROP(left);
  float top = PROP(top);
  float width = PROP(width);
  float height = PROP(height);

  if (style->display == CTX_DISPLAY_BLOCK ||
      style->display == CTX_DISPLAY_FLOW_ROOT ||
      style->display == CTX_DISPLAY_LIST_ITEM)
  {
    if (padding_left + margin_left + border_left_width != 0)
    {
      css_set_edge_left (mrg, mrg_edge_left (mrg) +
        padding_left + margin_left + border_left_width);
    }
    if (padding_right + margin_right + border_right_width
        != 0)
    {
      css_set_edge_right (mrg, mrg_edge_right (mrg) -
        (padding_right + margin_right + border_right_width));
    }


    /* collapsing of vertical margins */
    float actual_top = margin_top;
    if (actual_top >= mrg->state->vmarg)
      actual_top = actual_top - mrg->state->vmarg;
    else
      actual_top = 0;

    css_set_edge_top (mrg, mrg_y (mrg) + border_top_width + padding_top + actual_top);

    mrg->state->block_start_x = mrg_edge_left (mrg);
    mrg->state->block_start_y = mrg_y (mrg);
 // mrg->floats = 0;
  }
  else if (style->display == CTX_DISPLAY_INLINE_BLOCK)
  {


    float left_margin_pad_and_border = padding_left + margin_left + border_left_width;
    float right_margin_pad_and_border = padding_right + margin_right + border_right_width;

    if (mrg_x (mrg) + width + left_margin_pad_and_border + right_margin_pad_and_border
		    >= dynamic_edge_right)
       _mrg_nl (mrg);

    if (left_margin_pad_and_border != 0.0f)
    {
      css_set_edge_left (mrg, mrg_x (mrg) +
        padding_left + margin_left + border_left_width);
    }


#if 0
    if (right_margin_pad_and_border != 0.0f)
    {
      css_set_edge_right (mrg, mrg_edge_right (mrg) - right_margin_pad_and_border);
    }
#else
      css_set_edge_right (mrg, mrg_x (mrg) + width);
#endif

    css_set_edge_top (mrg, mrg_y (mrg) + border_top_width);// + actual_top);

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
    css_print (mrg, "•");
    mrg->x = x;
    mrg->line_max_height[mrg->line_level] = 0.0f;
    mrg->line_got_baseline[mrg->line_level]=0;
  }

  switch (style->position)
  {
    case CTX_POSITION_RELATIVE:
      /* XXX: deal with style->right and style->bottom */
      ctx_translate (mrg_ctx (mrg), left, top);
      mrg->relative_x += left;
      mrg->relative_y += top;
      /* fallthrough */

    case CTX_POSITION_STATIC:

      if (style->float_ == CTX_FLOAT_RIGHT)
      {
        if (width == 0.0)
        {
          width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + padding_right + padding_left + border_left_width + border_right_width);


        if (width + margin_left + margin_right >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (mrg);
          css_set_edge_left (mrg, mrg_edge_right (mrg) - width);
          css_set_edge_right (mrg, mrg_edge_right (mrg) - (margin_right + padding_right + border_right_width));

        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + margin_left + margin_right)
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(mrg);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(mrg);
        }

        css_set_edge_left (mrg, dynamic_edge_right - width);
        css_set_edge_right (mrg, dynamic_edge_right - (margin_right + padding_right + border_right_width));

        }

        css_set_edge_top (mrg, mrg_y (mrg) + (PROP(margin_top))); // - mrg->state->vmarg));

        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
        //mrg->floats = 0;

      } else if (style->float_ == CTX_FLOAT_LEFT)
      {
        float left;
        float widt = width;

        if (widt == 0.0)
        {
          widt = 4 * mrg_em (mrg);//mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        widt = (widt + padding_right + padding_left + border_left_width + border_right_width);

        if (widt + margin_left + margin_right >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (mrg);
          left = mrg_edge_left (mrg) + padding_left + border_left_width + margin_left;
        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < widt + margin_left + margin_right)
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(mrg);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(mrg);
        }
          left = dynamic_edge_left;// + padding_left + border_left_width + margin_left;
        }

        css_set_edge_left (mrg, left);
        css_set_edge_right (mrg,  left + widt +
            padding_left /* + border_right_width*/);
        css_set_edge_top (mrg, mrg_y (mrg) + (margin_top));// - mrg->state->vmarg));
                        //));//- mrg->state->vmarg));
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);// + padding_top + border_top_width;
        //mrg->floats = 0;

        /* change cursor point after floating something left; if pushed far
         * down, the new correct
         */
#if 0
        if(0)	
        mrg_set_xy (mrg, mrg->state->original_x = left + width + padding_left + border_right_width + padding_right + margin_right + margin_left + border_left_width,
            mrg_y (mrg) + padding_top + border_top_width);
#endif
      } /* XXX: maybe spot for */
      else if (1)
      {
         if (width)
           css_set_edge_right (mrg, mrg->state->block_start_x  + width);
      }
      break;
    case CTX_POSITION_ABSOLUTE:
      ctx_get_drawlist (mrg->ctx, &mrg->state->drawlist_start_offset);
      mrg->state->drawlist_start_offset--;
      {
        if (left == 0.0f) // XXX 0.0 should also be a valid value!
	  left = mrg->x;
        if (top == 0.0f)  // XXX 0.0 should also be a valid value!
	  top = mrg->y;

        //mrg->floats = 0;
        css_set_edge_left (mrg, left + margin_left + border_left_width + padding_left);
        css_set_edge_right (mrg, left + width);
        css_set_edge_top (mrg, top + margin_top + border_top_width + padding_top);
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
      }
      break;
    case CTX_POSITION_FIXED:
      ctx_get_drawlist (mrg->ctx, &mrg->state->drawlist_start_offset);
      mrg->state->drawlist_start_offset--;
      {
        if (!width)
        {
          width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

	ctx_translate (mrg_ctx(mrg), 0, css_panel_scroll (mrg));
        ctx_scale (mrg_ctx(mrg), mrg_ddpx (mrg), mrg_ddpx (mrg));
        //mrg->floats = 0;

        css_set_edge_left (mrg, left + margin_left + border_left_width + padding_left);
        css_set_edge_right (mrg, left + margin_left + border_left_width + padding_left + width);
        css_set_edge_top (mrg, top + margin_top + border_top_width + padding_top);
        mrg->state->block_start_x = mrg_x (mrg);
        mrg->state->block_start_y = mrg_y (mrg);
      }
      break;
  }

  if (is_block_item (style))
  {
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
            mrg->state->block_start_x - padding_left - border_left_width,
            mrg->state->block_start_y - mrg_em(mrg) - padding_top - border_top_width,
            width + border_right_width + border_left_width + padding_left + padding_right, //mrg_edge_right (mrg) - mrg_edge_left (mrg) + padding_left + padding_right + border_left_width + border_right_width,
            height + padding_top + padding_bottom + border_top_width + border_bottom_width);
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
         mrg->state->block_start_x - padding_left,
         mrg->state->block_start_y - padding_top,
         width + padding_left + padding_right,
         height + padding_top + padding_bottom);
    }
    else
    {
      ctx_deferred_rectangle (mrg->ctx, name,
         mrg_x (mrg), mrg_y (mrg),
         width  + padding_left + padding_right,
         height + padding_top + padding_bottom);
    }
    ctx_fill (mrg->ctx);
  }
  ctx_color_free (background_color);
  }
}
void _mrg_layout_post (Css *mrg, CtxFloatRectangle *ret_rect);

void css_set_style (Css *mrg, const char *style);

void css_start_with_style (Css        *mrg,
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
      css_set_style (mrg, collated_style);
      free (collated_style);
    }
  }
  if (style)
  {
    css_set_style (mrg, style);
  }
  _mrg_layout_pre (mrg);
}

void
css_start_with_stylef (Css *mrg, const char *style_id, void *id_ptr,
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
  css_start_with_style (mrg, style_id, id_ptr, buffer);
  free (buffer);
}

void css_start (Css *mrg, const char *style_id, void *id_ptr)
{
  css_start_with_style (mrg, style_id, id_ptr, NULL);
}

static int compare_zindex (const void *a, const void *b, void *d)
{
  const CssAbsolute *ma = a;
  const CssAbsolute *mb = b;
  return mb->z_index- ma->z_index;
}

void css_end (Css *mrg, CtxFloatRectangle *ret_rect)
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
      CssAbsolute *absolute = mrg->absolutes->data;
      ctx_save (mrg->ctx);
      ctx_translate (mrg->ctx, absolute->relative_x, absolute->relative_y);
      ctx_append_drawlist (mrg->ctx, absolute->entries+1, (absolute->count-1)*9);
      ctx_list_remove (&mrg->absolutes, absolute);
      free (absolute);
    }
  }
}

void  mrg_set_line_height (Css *mrg, float line_height);
float mrg_line_height (Css *mrg);


typedef struct _ItkCssDef ItkCssDef;

struct _ItkCssDef {
  uint32_t   id;
  CtxString *str;
  ItkCssDef *next;
};

static CtxString *css_svg_add_def (ItkCssDef **defs, uint32_t id)
{
  ItkCssDef *iter = *defs;
  while (iter)
  {
    if (iter->id == id)
      return iter->str;
    iter = iter->next;
  }
  iter = ctx_calloc (1, sizeof (ItkCssDef));
  iter->str = ctx_string_new ("");
  iter->id = id;
  iter->next = *defs;
  *defs = iter;
  return iter->str;
}

static void mrg_path_fill_stroke (Css *mrg, ItkCssDef **defs)
{
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *fill_color = ctx_color_new ();
  CtxColor *stroke_color = ctx_color_new ();

  const char *fill = ctx_get_string (mrg->ctx, SQZ_fill);
  const char *stroke = ctx_get_string (mrg->ctx, SQZ_stroke);


  ctx_get_color (ctx, SQZ_stroke_color, stroke_color);

  int has_stroke = (PROP(stroke_width) > 0.001f &&
		    (!ctx_color_is_transparent (stroke_color)
		     || (stroke != NULL)));

  if (fill && fill[0] == 'u' && strstr(fill, "url("))
  {
    char *id = strchr(fill, '#');
    if (id)
    {
      id ++;
    }
    if (id[strlen(id)-1]==')')
      id[strlen(id)-1]=0;
    if (id[strlen(id)-1]=='\'')
      id[strlen(id)-1]=0;
    if (id[strlen(id)-1]=='"')
      id[strlen(id)-1]=0;
    CtxString *str = css_svg_add_def (defs, ctx_strhash(id));
    ctx_parse (ctx, str->str);

    if (has_stroke)
      ctx_preserve (ctx);
    ctx_fill (ctx);
  }
  else
  {

  ctx_get_color (ctx, SQZ_fill_color, fill_color);

  if (!ctx_color_is_transparent (fill_color))
  {
    mrg_ctx_set_source_color (ctx, fill_color);

    if (has_stroke)
      ctx_preserve (ctx);
    ctx_fill (ctx);
  }

  }
  ctx_color_free (fill_color);

  if (has_stroke)
  {
    ctx_line_width (ctx, PROP(stroke_width));
    if (stroke && stroke[0] == 'u' && strstr(stroke, "url("))
    {
      char *id = strchr(stroke, '#');
      if (id)
      {
        id ++;
      }
      if (id[strlen(id)-1]==')')
        id[strlen(id)-1]=0;
      if (id[strlen(id)-1]=='\'')
        id[strlen(id)-1]=0;
      if (id[strlen(id)-1]=='"')
        id[strlen(id)-1]=0;
      CtxString *str = css_svg_add_def (defs, ctx_strhash(id));
      ctx_parse (ctx, str->str);
    }
    else
    {
      mrg_ctx_set_source_color (ctx, stroke_color);
    }
    ctx_stroke (ctx);
  }
  ctx_color_free (stroke_color);
}

void _mrg_border_top (Css *mrg, int x, int y, int width, int height)
{
  float border_top_width = PROP(border_top_width);
  if (border_top_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - border_top_width);
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width), 0);
    ctx_rel_line_to (ctx, -PROP(border_right_width), border_top_width);
    ctx_rel_line_to (ctx, - (width + PROP(padding_right) + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}

void _mrg_border_bottom (Css *mrg, int x, int y, int width, int height)
{
  float border_bottom_width = PROP(border_bottom_width);
  if (border_bottom_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), border_bottom_width);
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -border_bottom_width);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_r (Css *mrg, int x, int y, int width, int height)
{
  float border_top_width = PROP(border_top_width);
  if (border_top_width < 0.01f)
    return;
  Ctx *cr = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (cr, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (cr);
    ctx_begin_path (cr);
    ctx_move_to (cr, x, y - PROP(padding_top) - border_top_width);
    ctx_rel_line_to (cr, width + PROP(padding_right) + PROP(border_right_width), 0);
    ctx_rel_line_to (cr, -PROP(border_right_width), border_top_width);
    ctx_rel_line_to (cr, - (width + PROP(padding_right)), 0);

    mrg_ctx_set_source_color (cr, color);
    ctx_fill (cr);
  ctx_restore (cr);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_r (Css *mrg, int x, int y, int width, int height)
{
  float border_bottom_width = PROP(border_bottom_width);
  if (border_bottom_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), border_bottom_width);
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -border_bottom_width);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_l (Css *mrg, int x, int y, int width, int height)
{
  float border_top_width = PROP(border_top_width);
  if (border_top_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - border_top_width);
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width), 0);
    ctx_rel_line_to (ctx, 0, border_top_width);
    ctx_rel_line_to (ctx, - (width + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_l (Css *mrg, int x, int y, int width, int height)
{
  float border_bottom_width = PROP(border_bottom_width);
  if (border_bottom_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, border_bottom_width);
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(border_left_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -border_bottom_width);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}


void _mrg_border_top_m (Css *mrg, int x, int y, int width, int height)
{
  float border_top_width = PROP(border_top_width);
  if (border_top_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_top_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x,
                       y - PROP(padding_top) - border_top_width);
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, border_top_width);
    ctx_rel_line_to (ctx, -width, 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_m (Css *mrg, int x, int y, int width, int height)
{
  float border_bottom_width = PROP(border_bottom_width);
  if ((border_bottom_width) < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_bottom_color, color);

  if (!ctx_color_is_transparent (color))
  {
    ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, border_bottom_width);
    ctx_rel_line_to (ctx, - width, 0);
    ctx_rel_line_to (ctx, 0, -border_bottom_width);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }

  ctx_color_free (color);
}
void _mrg_border_left (Css *mrg, int x, int y, int width, int height)
{
  float border_left_width = PROP(border_left_width);
  if (border_left_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_left_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - border_left_width,
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, border_left_width, PROP(border_top_width));
    ctx_rel_line_to (ctx, 0, height + PROP(padding_top) + PROP(padding_bottom) );
    ctx_rel_line_to (ctx, -border_left_width, PROP(border_bottom_width));
    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_right (Css *mrg, int x, int y, int width, int height)
{
  float border_right_width = PROP(border_right_width);
  if (border_right_width < 0.01f)
    return;
  Ctx *ctx = mrg_ctx (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, SQZ_border_right_color, color);

  if (!ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, border_right_width, PROP(border_bottom_width));
    ctx_rel_line_to (ctx, 0, - (height + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width)));
    ctx_rel_line_to (ctx, -border_right_width, PROP(border_top_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

static void mrg_box (Css *mrg, int x, int y, int width, int height)
{
  _mrg_border_top (mrg, x, y, width, height);
  _mrg_border_left (mrg, x, y, width, height);
  _mrg_border_right (mrg, x, y, width, height);
  _mrg_border_bottom (mrg, x, y, width, height);
}

#if 0
static void mrg_box_fill (Css *mrg, CtxStyle *style, float x, float y, float width, float height)
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
#endif

/*
 *  each style state level needs to know how far down it has
 *  painted background,.. on background increment we do all of them..
 *  .. floats are problematic - maybe draw them in second layer.
 *
 */


#if 0
static void
_mrg_resolve_line_height (Css *mrg, void *data, int last)
{
  //CssState *state = &mrg->states[mrg->state_no];
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

static float measure_word_width (Css *mrg, const char *word)
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
{"int", "uint32_t", "uint64_t", "uint8_t", "Ctx", "cairo_t", "Css", "float", "double",
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
float mrg_draw_string (Css *mrg, CtxStyle *style, 
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

float mrg_addstr (Css *mrg, const char *string, int utf8_length);

float paint_span_bg_final (Css   *mrg, float x, float y,
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

float paint_span_bg (Css   *mrg, float x, float y,
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
mrg_addstr (Css *mrg, const char *string, int utf8_length)
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
void mrg_xy (Css *mrg, float x, float y)
{
  mrg->x = x * mrg_em (mrg);
  mrg->y = y * mrg_em (mrg);
}
#endif

void mrg_set_xy (Css *mrg, float x, float y)
{
  mrg->x = x;
  mrg->y = y;
  mrg->state->overflowed = 0;
}

float mrg_x (Css *mrg)
{
  return mrg->x;
}

float mrg_y (Css *mrg)
{
  return mrg->y;
}

void mrg_set_wrap_skip_lines (Css *mrg, int skip_lines);
void mrg_set_wrap_max_lines  (Css *mrg, int max_lines);

void mrg_set_wrap_skip_lines (Css *mrg, int skip_lines)
{
    mrg->state->skip_lines = skip_lines;
}

void mrg_set_wrap_max_lines  (Css *mrg, int max_lines)
{
    mrg->state->max_lines = max_lines;
}

static void _mrg_spaces (Css *mrg, int count)
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
             css_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             css_end (mrg, NULL);\
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
             css_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             css_end (mrg, NULL);\
           }\
         else\
           _mrg_nl (mrg);\
         } else _mrg_nl (mrg);\
      }\
    if (skip_lines<=0)\
      mrg_set_xy (mrg, _mrg_dynamic_edge_left(mrg), mrg_y (mrg));}while(0)



static void mrg_get_edit_state (Css *mrg, 
     float *x, float *y, float *s, float *e,
     float *em_size)
{
  if (x) *x = mrg->e_x;
  if (y) *y = mrg->e_y;
  if (s) *s = mrg->e_ws;
  if (e) *e = mrg->e_we;
  if (em_size) *em_size = mrg->e_em;
}


static void emit_word (Css *mrg,
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
               css_start (mrg, ".cursor", NULL);
               _mrg_spaces (mrg, 1); 
               css_end (mrg, NULL);
              } else { 
               mrg->x += measure_word_width (mrg, " ");
              }
            }
            else 
              {
                if (print){
                  if (mrg->state->style.print_symbols)
                    {
                      css_start (mrg, "dim", NULL);
                      mrg->x += mrg_addstr (mrg, "␣", -1);
                      css_end (mrg, NULL);
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
          css_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg,  mrg_utf8_skip (word, cursor_start - *pos), 1);
          css_end (mrg, NULL);
          mrg->x += mrg_addstr (mrg,  mrg_utf8_skip (word, cursor_start - *pos + 1), len - (cursor_start - *pos) - 1);
#else

          char *dup, *dup2, *dup3;

          dup = strdup (word);
          dup2 = strdup (ctx_utf8_skip (dup, cursor_start - *pos));
          dup3 = strdup (ctx_utf8_skip (dup, cursor_start - *pos + 1));
          *((char*)ctx_utf8_skip (dup,  cursor_start - *pos)) = 0;
          *((char*)ctx_utf8_skip (dup2, 1)) = 0;

          mrg->x += mrg_addstr (mrg,  dup, -1);
          css_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg,  dup2, -1);
          css_end (mrg, NULL);
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

static int css_print_wrap (Css        *mrg,
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
            css_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg,  "¶", -1);\
            css_end (mrg, NULL);
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
                    css_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    css_end (mrg, NULL);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg,  " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      css_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg,  "␣", -1);
                      css_end (mrg, NULL);
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
        css_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        css_end (mrg, NULL);
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

int css_print_get_xy (Css *mrg, const char *string, int no, float *x, float *y)
{
  int ret;
  if (!string)
    return 0;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      ret = css_print_wrap (mrg, 0, string, strlen (string), mrg->state->max_lines,
                             mrg->state->skip_lines, no, x, y);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }
  if (y) *y = mrg->y;
  if (x) *x = mrg->x + no; // XXX: only correct for nct/monospace

  return 0;
}

typedef struct _CssGlyph CssGlyph;

struct _CssGlyph{
  unsigned long index; /*  done this way, the remnants of layout; before feeding
                        *  glyphs positions in cairo, similar to how pango would do
                        *  can be reused for computing the caret nav efficiently.
                        */
  float x;
  float y;
  int   no;
};

static int css_print_wrap2 (Css        *mrg,
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

  CssGlyph *g = calloc (1, sizeof (CssGlyph));
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
            css_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg,  "¶", -1);\
            css_end (mrg, NULL);
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
                    css_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    css_end (mrg, NULL);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg,  " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      css_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg,  "␣", -1);
                      css_end (mrg, NULL);
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
        css_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        css_end (mrg, NULL);
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

CtxList *css_print_get_coords (Css *mrg, const char *string)
{
  CtxList *ret = NULL;
  if (!string)
    return ret;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      css_print_wrap2 (mrg, 0, string, strlen (string), mrg->state->max_lines,
                       mrg->state->skip_lines, &ret);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }

  return ret;
}

#include <math.h>

int css_print (Css *mrg, const char *string)
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
   return css_print_wrap (mrg, 1, string, strlen (string), mrg->state->max_lines, mrg->state->skip_lines, mrg->cursor_pos, NULL, NULL);

  ret  = mrg_addstr (mrg,  string, ctx_utf8_strlen (string));
  mrg->x += ret;
  return ret;
}

void _mrg_text_prepare (Css *mrg)
{
  hl_state_c = MRG_HL_NEUTRAL;
}

void _mrg_text_init (Css *mrg)
{
  // XXX: this should be done in a prepre,.. not an init?
  //
  mrg->state->style.line_height = 1.0;
  mrg->state->style.print_symbols = 0;
}

void  mrg_text_listen_done (Css *mrg)
{
  mrg->text_listen_active = 0;
}

void  mrg_text_listen_full (Css *mrg, CtxEventType types,
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

void  mrg_text_listen (Css *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2)
{
  mrg_text_listen_full (mrg, types, cb, data1, data2, NULL, NULL);
}


static void cmd_home (CtxEvent *event, void *data1, void *data2)
{
  Css *mrg = data1;
  mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_end (CtxEvent *event, void *data1, void *data2)
{
  Css *mrg = data1;
  mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str);
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_backspace (CtxEvent *event, void *data1, void *data2)
{
  Css *mrg = data1;
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
  Css *mrg = data1;
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
  Css *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx, cy;
  cx = cy = 0;
 
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);
  css_set_edge_left (mrg, e_s - PROP (padding_left));
  css_set_edge_right (mrg, e_e + PROP (padding_right));
  mrg_set_xy (mrg, e_x, e_y);
  css_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

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
      css_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

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
  Css *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx = 0.0f, cy = 0.0f;
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);

  css_set_edge_left  (mrg, e_s - PROP(padding_left));
  css_set_edge_right (mrg, e_e + PROP(padding_right));

  mrg_set_xy (mrg, e_x, e_y);
  css_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

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
      css_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

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

int mrg_get_cursor_pos (Css *mrg)
{
  return mrg->cursor_pos;
}

void mrg_set_cursor_pos (Css *mrg, int pos)
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
  Css *mrg = data1;
  mrg->cursor_pos--;
  if (mrg->cursor_pos < 0)
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_right (CtxEvent *event, void *data1, void *data2)
{
  Css *mrg = data1;
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

static void add_utf8 (Css *mrg, const char *string)
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
  Css *mrg = data1;
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
  Css *mrg = data1;
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

void mrg_text_edit_bindings (Css *mrg)
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
void mrg_edit_string (Css *mrg, char **string,
                      void (*update_string)(Css *mrg,
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
css_printf (Css *mrg, const char *format, ...)
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
  css_print (mrg, buffer);
  free (buffer);
}

void  css_set_font_size (Css *mrg, float font_size)
{
  mrg->font_size = font_size;
  //css_set_stylef (mrg, "font-size:%fpx;", font_size);
}

void _mrg_block_edit (Css *mrg)
{
  mrg->text_edit_blocked = 1;
}
void _mrg_unblock_edit (Css *mrg)
{
  mrg->text_edit_blocked = 0;
}

void mrg_edit_start_full (Css *mrg,
                          CssNewText  update_string,
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

void  mrg_edit_start (Css *mrg,
                      CssNewText  update_string,
                      void *user_data)
{
  return mrg_edit_start_full (mrg, update_string, user_data, NULL, NULL);
}

void  mrg_edit_end (Css *mrg)
{
  mrg->text_edited = 0;
  mrg_text_edit_bindings (mrg);
}


#if 0
static void ctx_css_add_class (Css *mrg, const char *class_name)
{
  int i;
  CtxStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->classes[i]; i++);
  node->classes[i] = mrg_intern_string (class_name);
}

static void ctx_css_add_pseudo_class (Css *mrg, const char *pseudo_class)
{
  int i;
  CtxStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->pseudo[i]; i++);
  node->pseudo[i] = mrg_intern_string (pseudo_class);
}
#endif

void _mrg_set_wrap_edge_vfuncs (Css *mrg,
    float (*wrap_edge_left)  (Css *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Css *mrg, void *wrap_edge_data),
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


void _mrg_layout_post (Css *mrg, CtxFloatRectangle *ret_rect)
{
  Ctx *ctx         = mrg->ctx;
  float vmarg      = 0;
  CtxStyle *style  = ctx_style (mrg);
  float height     = PROP(height);
  float width      = PROP(width);

  float padding_left = PROP(padding_left);
  float margin_left = PROP(margin_left);
  float border_left_width = PROP(border_left_width);
  float padding_right = PROP(padding_right);
  //float margin_right = PROP(margin_right);
  //float border_right_width = PROP(border_right_width);
  float padding_top = PROP(padding_top);
  float margin_top = PROP(margin_top);
  float border_top_width = PROP(border_top_width);
  float padding_bottom = PROP(padding_bottom);
  float margin_bottom = PROP(margin_bottom);
  float border_bottom_width = PROP(border_bottom_width);
  float left = PROP(left);
  float top = PROP(top);

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
       mrg->state->block_start_x - padding_left - border_left_width - margin_left;
    float_data->y = 
         mrg->state->block_start_y - mrg_em(mrg) - padding_top - border_top_width
      - margin_top;

    float_data->width = 
         mrg_edge_right (mrg) - mrg_edge_left (mrg)
     //+ border_left_width 
     //+ border_right_width
#if 0
     /*+ padding_left +*/ + border_left_width + margin_left
     /*+ padding_right +*/ + border_right_width + margin_right
#endif
     ;

    float_data->height = 
       mrg_y (mrg) - (mrg->state->block_start_y)
         + margin_bottom + padding_top + padding_bottom + border_top_width + border_bottom_width;
  }


  if (style->display == CTX_DISPLAY_INLINE_BLOCK)
  {
    CtxFloatRectangle _geo;
    CtxFloatRectangle *geo = &_geo;
    memset (geo, 0, sizeof (_geo));

    if (width == 0)
    {
      width = mrg_x (mrg) - (mrg->state->block_start_x) + padding_right;
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

    geo->width += padding_right + padding_left;
    geo->height += padding_top + padding_bottom;
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
      vmarg = margin_bottom;

      mrg_set_xy (mrg, 
          mrg_edge_left (mrg),
          mrg_y (mrg) + vmarg + border_bottom_width + padding_bottom);
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
    //ctx_translate (mrg_ctx (mrg), -left, -top); // not really
    //                                            // needed we'll
    //                                                        // restore..
    mrg->relative_x -= left;
    mrg->relative_y -= top;
  }

  ctx_restore (mrg_ctx (mrg));

  if (style->position == CTX_POSITION_ABSOLUTE ||
      style->position == CTX_POSITION_FIXED)
  {
    int start_offset = mrg->state->drawlist_start_offset;
    int end_offset;
    const CtxEntry *entries = ctx_get_drawlist (mrg->ctx, &end_offset);
    int count = end_offset - start_offset;

    CssAbsolute *absolute = calloc (1, sizeof (CssAbsolute) + count * 9);
    absolute->z_index = style->z_index;
    absolute->top    = top;
    absolute->left   = left;
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
    fprintf (stderr, "unbalanced css_start/css_end, enderflow %i\n", mrg->state_no);
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

typedef struct CssEntity {
   uint32_t    name;
   const char *value;
} CssEntity;

static CssEntity entities[]={
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

static int 
mrg_parse_transform (Css *mrg, CtxMatrix *matrix, const char *str_in)
{
  // TODO : parse combined transforms

  const char *str = str_in;

  do {

  if (!strncmp (str, "matrix", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0.0,};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return 0;
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

    matrix->m[0][0] = number[0];
    matrix->m[0][1] = number[2];
    matrix->m[0][2] = number[4];
    matrix->m[1][0] = number[1];
    matrix->m[1][1] = number[3];
    matrix->m[1][2] = number[5];
  }
  else if (!strncmp (str, "scale", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0,0};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return 0;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
	if (numbers<11)
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
      return 0;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
	if (numbers < 11)
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
      return 0;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
	if (numbers < 11)
          numbers++;
      }
    }
    if (numbers == 3)
    {
      ctx_matrix_translate (matrix, -number[1], -number[2]);
      ctx_matrix_rotate (matrix, number[0] / 360.0 * 2 * M_PI);
      ctx_matrix_translate (matrix, number[1], number[2]);
    }
    else
    ctx_matrix_rotate (matrix, number[0] / 360.0 * 2 * M_PI);
  }
  else
  {
    //fprintf (stderr, "unhandled transform: %s\n", str);
    ctx_matrix_identity (matrix);
    return 0;
  }
    str = strchr (str, ')');
    if (str) {
	str++;
        while (*str == ' ')str++;
    }
  }
  while (strchr (str, '('));
  return 1;
}

int
mrg_parse_svg_path (Css *mrg, const char *str)
{
  /* this function is the seed of the ctx parser */
  char  command = 'm';
  char *s;
  int numbers = 0;
  double number[12];
  double pcx, pcy, cx, cy;

  if (!str)
    return -1;

  Ctx *ctx = mrg_ctx (mrg);
  ctx_parse (ctx, str);
  return 0;
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
      if (numbers < 11)
        numbers++;

      switch (command)
      {
        case 'a':
          if (numbers == 9)
          {
            /// XXX: NYI
            s++;
            goto again;
          }
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
mrg_parse_polygon (Css *mrg, const char *str)
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
      if (numbers<11)
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

#if 0
static void
mrg_parse_ellipse (Css *mrg, const char *str)
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
      if (numbers<11)
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
#endif


void _mrg_set_wrap_edge_vfuncs (Css *mrg,
    float (*wrap_edge_left)  (Css *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Css *mrg, void *wrap_edge_data),
    void *wrap_edge_data);

int mrg_get_contents (Css         *mrg,
                      const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length);


void  mrg_text_listen (Css *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2);

void  mrg_text_listen_full (Css *mrg, CtxEventType types,
                            CtxCb cb, void *data1, void *data2,
          void (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
          void  *finalize_data);
void  mrg_text_listen_done (Css *mrg);

char *_mrg_resolve_uri (const char *base_uri, const char *uri);
typedef struct _CssImage CssImage;
struct _CssImage
{
  char *uri;
  char *path;
  int width;
  int height;
  CtxBuffer *surface;
};

static CtxList *images = NULL;

static CssImage *_mrg_image (Css *mrg, const char *path)
{
  char *uri =  _mrg_resolve_uri (mrg->uri_base, path);

  for (CtxList *l = images; l; l = l->next)
  {
    CssImage *image = l->data;
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
    CssImage *image = calloc (1, sizeof (CssImage));
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

int mrg_query_image (Css        *mrg,
                     const char *path,
                     int        *width,
                     int        *height)
{
  CssImage *image = _mrg_image (mrg, path);
  if (image)
  {
    *width = image->width;
    *height = image->height;
    return 1;
  }
  return 0;
}

void mrg_image (Css *mrg, float x0, float y0, float width, float height, float opacity, const char *path, int *used_width, int *used_height)
{
  CssImage *image = _mrg_image (mrg, path);
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


void css_xml_render (Css *mrg,
                     char *uri_base,
                     void (*link_cb) (CtxEvent *event, void *href, void *link_data),
                     void *link_data,
                     void *(finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                     void *finalize_data,
                     char *html_)
{
  CssXml *xmltok;
  CtxString *svg_text = ctx_string_new ("");
  uint32_t tag[CTX_MAX_STATE_DEPTH];
  int pos             = 0;
  int type            = t_none;
  static int depth    = 0;
  int in_style        = 0;
  int in_defs         = 0;
  int should_be_empty = 0;
  int tagpos          = 0;
  ItkCssDef *defs = NULL;

  if (mrg->uri_base)
	  free (mrg->uri_base);
  mrg->uri_base = NULL;
  if (uri_base)
    mrg->uri_base = strdup (uri_base);

  CtxString *style = ctx_string_new ("");
  int whitespaces = 0;
  uint32_t att = 0;


////////////////////////////////////////////////////
  CtxString *str = NULL;

  whitespaces = 0;
  att = 0;
  pos = 0;
#if 1
  xmltok = xmltok_buf_new (html_);

  ctx_save (mrg->ctx);
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
      case t_att:
#if 0
	for (int i = 0; data[i]; i++)
	  if (data[i]=='-')data[i]='_';
#endif
        att = ctx_strhash (data);
        break;
      case t_val:
	if (in_defs)
	{
          ctx_set_string (mrg->ctx, att, data);
	}
	break;
      case t_endtag:
	{
        //int i;
        uint32_t data_hash = ctx_strhash (data);
     // for (i = 0; data[i]; i++)
     //   data[i] = tolower (data[i]);
        in_style = (data_hash == SQZ_style);

        if (data_hash == SQZ_defs)
	{
	  in_defs = 1;
	}

	if (in_defs)
	{

	  switch (data_hash)
	  {
	    case SQZ_stop:
            {
	       const char *offset     = ctx_get_string (mrg->ctx, SQZ_offset);
	       const char *stop_color = ctx_get_string (mrg->ctx, SQZ_stop_color);
	       const char *stop_opacity = ctx_get_string (mrg->ctx, SQZ_stop_opacity);

	       float off = 0.0;
	       float rgba[4] = {0,0,0,1.0f};
	       if (!stop_color)
		 break;

	       if (!strcmp (stop_color, "red"))
	       {
		 rgba[0] = 1.0f;
	       }
	       else if (!strcmp (stop_color, "gold"))
	       {
		 rgba[0] = 1.0f;
		 rgba[1] = 0.7f;
	       }
	       else
	       {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, stop_color);
                  ctx_color_get_rgba (ctx_get_state (mrg->ctx), color, rgba);
	       }

	       if (stop_opacity)
	       {
	         if (strchr(stop_opacity, '%'))
		   rgba[3] *= (atof (stop_opacity) / 100.0f);
	         else 
		   rgba[3] *= (atof (stop_opacity));
	       }

	       if (offset)
	       {
	         if (strchr(offset, '%'))
		   off = atof (offset) / 100.0f;
	         else 
		   off = atof (offset);
	       }

	       if (str)
	      ctx_string_append_printf (str, "addStop %.3f %.3f %.3f %.3f %.3f\n",
		   off, rgba[0], rgba[1], rgba[2], rgba[3]);
	    }
	    break;
	    case SQZ_radialGradient:
	    {
	      const char *id = ctx_get_string (mrg->ctx, SQZ_id);
#define GRAD_PROP_STR(name, def_val) \
	      const char *name = def_val;\
	      if (ctx_is_set(mrg->ctx, SQZ_##name)) name = PROPS(name);
#define GRAD_PROP_X(name, def_val) \
	      float name; const char *str_##name = def_val;\
	      if (ctx_is_set(mrg->ctx, SQZ_##name)) str_##name = PROPS(name);\
	      name = mrg_parse_px_x (mrg, str_##name, NULL);
#define GRAD_PROP_Y(name, def_val) \
	      float name; const char *str_##name = def_val;\
	      if (ctx_is_set(mrg->ctx, SQZ_##name)) str_##name = PROPS(name);\
	      name = mrg_parse_px_y (mrg, str_##name, NULL);
	      

              // TODO : gradientUnits='userSpaceOnUse'  
              // TODO : gradientUnits='objectBoundingBox'  (default)
              // SQZ_gradientUnits
              // SQZ_gradientTransform
	      // SQZ_spreadMethod  =  pad, reflect, repeat
	      //
	      // SQZ_fy,
	      // SQZ_fx,
	      // SQZ_fr,
	      if (id)
	      {
	      GRAD_PROP_STR(gradientUnits, "userSpaceOnUse");
	      GRAD_PROP_STR(spreadMethod, "pad");
	        const char *transform = ctx_get_string (mrg->ctx, SQZ_gradientTransform);

	      GRAD_PROP_X(cx, "50%");
	      GRAD_PROP_Y(cy, "50%");
	      GRAD_PROP_Y(r,  "100%");
	      GRAD_PROP_Y(fr, "0%");
	      GRAD_PROP_X(fx, "50%"); // XXX should be inherited from cx/cy
	      GRAD_PROP_Y(fy, "50%"); // not 50% ..
	    
	      css_svg_add_def (&defs, ctx_strhash (id));

	       str = css_svg_add_def (&defs, ctx_strhash (id));
	       ctx_string_append_printf (str, " radialGradient %f %f %f %f %f %f\n",
			       cx,cy,r,fx,fy,fr);

	       ctx_string_append_printf (str, " rgba ");
	       if (transform)
	       {
                 CtxMatrix matrix;
                 if (mrg_parse_transform (mrg, &matrix, transform))
		 {
	       ctx_string_append_printf (str, " sourceTransform %f %f %f %f %f %f %f %f %f\n",
		        matrix.m[0][0], matrix.m[0][1], matrix.m[0][2],
                        matrix.m[1][0], matrix.m[1][1], matrix.m[1][2],
                        matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]
		    );
		 }
		 
	       }
	      }
	    }
	    break;
	    case SQZ_linearGradient:
	    {
	      const char *id = ctx_get_string (mrg->ctx, SQZ_id);

	      if (id)
	      {
	        GRAD_PROP_STR(gradientUnits, "userSpaceOnUse");
	        GRAD_PROP_STR(spreadMethod, "pad");
	        const char *transform = ctx_get_string (mrg->ctx, SQZ_gradientTransform);
	        GRAD_PROP_X(x1, "0%");
	        GRAD_PROP_Y(y1, "0%");
	        GRAD_PROP_X(x2, "100%");
	        GRAD_PROP_Y(y2, "0%");

	       str = css_svg_add_def (&defs, ctx_strhash (id));
	       ctx_string_append_printf (str, " linearGradient %f %f %f %f\n",
			       x1,y1,x2,y2);
	       ctx_string_append_printf (str, " rgba ");
	       if (transform)
	       {
                 CtxMatrix matrix;
                 if (mrg_parse_transform (mrg, &matrix, transform))
		 {
	            ctx_string_append_printf (str, " sourceTransform %f %f %f %f %f %f %f %f %f\n",
		        matrix.m[0][0], matrix.m[0][1], matrix.m[0][2],
                        matrix.m[1][0], matrix.m[1][1], matrix.m[1][2],
                        matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]
		    );
		 }
		 
	       }
	      }
	    }
	    break;
	  }

	}

	if (in_style)
	{
          if (mrg->css_parse_state)
                  free (mrg->css_parse_state);
          mrg->css_parse_state = NULL;
	}
	}
        break;
      case t_tag:

	if (in_defs)
	  ctx_save (mrg->ctx);
	break;

      case t_closetag:
      case t_closeemptytag:
	{
          uint32_t data_hash = ctx_strhash (data);
          if (data_hash == SQZ_defs)
	    in_defs = 0;
	  if (in_defs)
	  {
	    ctx_restore (mrg->ctx);
	  }
	}
	break;
      default:
        break;
    }
  }
  ctx_restore (mrg->ctx);
  in_defs = 0;
  in_style = 0;

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


  //css_start (mrg, "fjo", NULL);
  //ctx_stylesheet_add (mrg, style_sheets->str, uri_base, CTX_STYLE_XML, NULL);

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);

    if (type == t_tag ||
        //type == t_att ||
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
            css_printf (mrg, "%c", c);
          }
          else
          {
            uint32_t hash = ctx_strhash (data);
          for (i = 0; entities[i].name && !dealt_with; i++)
            if (hash == entities[i].name)
            {
              css_print (mrg, entities[i].value);
              dealt_with = 1;
            }
          }

          if (!dealt_with){
            css_start (mrg, "dim", (void*)((size_t)pos));
            css_print (mrg, data);
            css_end (mrg, NULL);
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
	  if (mrg->in_svg)
	  {
	    ctx_string_append_str(svg_text, data);
	  }
	  else
          css_print (mrg, data);
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
	  if (mrg->in_svg)
	  {
	    //if (whitespaces == 0)
	    {
	      ctx_string_append_str (svg_text, data);
	    }
	    whitespaces ++;
	  }
	  else
          switch (ctx_style (mrg)->white_space)
          {
            case CTX_WHITE_SPACE_PRE: /* handles as pre-wrap for now */
            case CTX_WHITE_SPACE_PRE_WRAP:
              css_print (mrg, data);
              break;
            case CTX_WHITE_SPACE_PRE_LINE:
              switch (*data)
              {
                case ' ':
                  whitespaces ++;
                  if (whitespaces == 1)
                    css_print (mrg, " ");
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
                css_print (mrg, " ");
		mrg->unresolved_line = save;
	      }
              break;
          }
        }
        break;
      case t_tag:
        //htmlctx->attributes = 0;
        //ctx_save (mrg->ctx);
	{
          uint32_t data_hash = ctx_strhash (data);
          tagpos = pos;
          ctx_string_clear (style);
          ctx_set_string (mrg->ctx, SQZ_style, "");
          ctx_set_string (mrg->ctx, SQZ_transform, "");

	  if (data_hash == SQZ_html)
	  {
  	  }
	  else if (data_hash == SQZ_defs)
	  {
	    in_defs = 1;
	  }
	  else if (data_hash == SQZ_svg)
	  {
	     mrg->in_svg++;
	  }
	}
        break;
      case t_att:
#if 0
	for (int i = 0; data[i]; i++)
	  if (data[i]=='-')data[i]='_';
#endif
        att = ctx_strhash (data);
        break;
      case t_val:
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
              SQZ_text_anchor,
              0};
            char *style_attribute_names[] ={
              "fill-rule",
              "font-size",
              "font-family",
              "fill-color",
              "fill",
              "stroke-width",
              "stroke-color",
              "stroke-linecap",
              "stroke-miterlimit",
              "stroke-linejoin",
              "stroke",
              //"viewBox",
              "color",
              "background-color",
              "background",
              "text-anchor",
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
               css_end (mrg, NULL);
               depth--;
	     }

	  }

#else

        if (depth && (data_hash == SQZ_tr && tag[depth-1] == SQZ_td))
        {
          css_end (mrg, NULL);
          depth--;
          css_end (mrg, NULL);
          depth--;
        }
        if (depth && (data_hash == SQZ_tr && tag[depth-1] == SQZ_td))
        {
          css_end (mrg, NULL);
          depth--;
          css_end (mrg, NULL);
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
          css_end (mrg, NULL);
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
          css_start_with_style (mrg, combined, (void*)((size_t)tagpos), style->str);
        }

        if (data_hash == SQZ_g)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
        }

        else if (data_hash == SQZ_svg)
        {
          const char *vbox = PROPS(viewbox);
          if (vbox && 0)
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
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
          mrg_parse_polygon (mrg, PROPS(d));
	  ctx_close_path (mrg->ctx);
          mrg_path_fill_stroke (mrg, &defs);
        }
        else if (data_hash == SQZ_polyline)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
          mrg_parse_polygon (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg, &defs);
        }

        else if (data_hash == SQZ_path)
        {
          const char *transform;
	  ctx_begin_path (mrg_ctx (mrg)); // XXX: eeeek!
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
          mrg_parse_svg_path (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg, &defs);
        }

        else if (data_hash == SQZ_line)
	{
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
	  // SQZ_x1
	  // SQZ_y1
	  // SQZ_x2
	  // SQZ_y2
	  ctx_move_to (mrg->ctx, PROP(x1), PROP(y1));
	  ctx_line_to (mrg->ctx, PROP(x2), PROP(y2));
          mrg_path_fill_stroke (mrg, &defs);
	}
        else if (data_hash == SQZ_ellipse)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
	  ctx_save (mrg->ctx);
	  ctx_translate (mrg->ctx, PROP(cx), PROP(cy));
	  ctx_scale (mrg->ctx, PROP(rx), PROP(ry));
	  ctx_arc (mrg->ctx, 0.0f, 0.0f, 1.0f, 0.0f, M_PI*2, 0);
	  ctx_restore (mrg->ctx);
          mrg_path_fill_stroke (mrg, &defs);
        }

        else if (data_hash == SQZ_circle)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
	  ctx_arc (mrg->ctx, PROP(cx), PROP(cy), PROP(r), 0.0f, M_PI*2.0f, 0);
          mrg_path_fill_stroke (mrg, &defs);
        }

        else if (data_hash == SQZ_rect && !in_defs)
        {
          float width  = PROP(width);
          float height = PROP(height);
          float x      = PROP(x);
          float y      = PROP(y);
          float rx     = PROP(rx);

          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              if (mrg_parse_transform (mrg, &matrix, transform))
                ctx_apply_matrix (mrg_ctx (mrg), &matrix);
            }
	  if (rx > 0.001f)
            ctx_round_rectangle (mrg_ctx (mrg), x, y, width, height, rx);
	  else
            ctx_rectangle (mrg_ctx (mrg), x, y, width, height);
          mrg_path_fill_stroke (mrg, &defs);
        }

        else if (data_hash == SQZ_text)
        {
	  ctx_string_set (svg_text, "");
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
            css_printf (mrg, "![%s]", src);
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
            css_end (mrg, NULL);
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
          css_end (mrg, NULL);
          //ctx_restore (mrg->ctx);
          depth--;

	if (data_hash == SQZ_defs)
	{
	  in_defs = 0;
	}
	else if (data_hash == SQZ_svg)
	{
	  mrg->in_svg--;
	}
        else if (data_hash == SQZ_text)
	{
	  ctx_move_to (mrg->ctx, PROP(x), PROP(y));

	  //fprintf (stderr, "%f %f\n", PROP(text_align), PROP(text_anchor));
	  ctx_font_size (mrg->ctx, PROP(font_size));
	  ctx_text (mrg->ctx, svg_text->str);
	}

          if (depth<0)depth=0; // XXX
#if 1
          if (tag[depth] != data_hash)
          {
            if (tag[depth] == SQZ_p)
            {
              css_end (mrg, NULL);
              depth --;
            } else 
            if (depth > 0 && tag[depth-1] == data_hash)
            {
              css_end (mrg, NULL);
              depth --;
            }
            else if (depth > 1 && tag[depth-2] == data_hash)
            {
              int i;
              for (i = 0; i < 2; i ++)
              {
                depth --;
                css_end (mrg, NULL);
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
                css_end (mrg, NULL);
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
                css_end (mrg, NULL);
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
                css_end (mrg, NULL);
              }
            }
#endif
            else
            {
              if (data_hash == SQZ_table && tag[depth] == SQZ_td)
              {
                depth--;
                css_end (mrg, NULL);
                depth--;
                css_end (mrg, NULL);
              }
              else if (data_hash == SQZ_table && tag[depth] == SQZ_tr)
              {
                depth--;
                css_end (mrg, NULL);
              }
            }
          }
#endif
        }
        break;
    }
  }
  //css_end (mrg,  NULL);

  xmltok_free (xmltok);

  if (depth!=0){
    //fprintf (stderr, "xml parsing unbalanced, %i open tags.. \n", depth);
    while (depth > 0)
    {
      //fprintf (stderr, " %s ", ctx_str_decode (tag[depth-1]));
      css_end (mrg, NULL);
      depth--;
    }
    //fprintf (stderr, "\n");
  }

  ctx_string_free (style, 1);
  ctx_string_free (svg_text, 1);

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

  while (defs)
  {
    ItkCssDef *temp =defs;
    ctx_string_free (temp->str, 1);
    defs = temp->next;
    ctx_free (temp);
  }
}

int css_xml_extent (Css *mrg, uint8_t *contents, float *width, float *height, float *vb_x, float *vb_y, float *vb_width, float *vb_height)
{
  CssXml *xmltok;
  int pos             = 0;
  int type            = t_none;

  float c_width = 0.0f;
  float c_height = 0.0f;

   int in_svg = 0;
////////////////////////////////////////////////////

  type = t_none;
  unsigned int att = 0;
  xmltok = xmltok_buf_new ((char*)contents);

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);

    if (type == t_tag ||
        //type == t_att ||
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
      case t_tag:
        //htmlctx->attributes = 0;
        //ctx_save (mrg->ctx);
	{
          uint32_t data_hash = ctx_strhash (data);

	  in_svg = 0;
	  if (data_hash == SQZ_html)
	  {
  	  }
	  else if (data_hash == SQZ_svg)
	  {
	     in_svg = 1;
	  }
	}
        break;
      case t_att:
#if 0
	for (int i = 0; data[i]; i++)
	  if (data[i]=='-')data[i]='_';
#endif
        att = ctx_strhash (data);

        break;
      case t_val:
	if (in_svg && att == SQZ_viewBox)
	{
            if(vb_x) *vb_x = _ctx_str_get_float (data, 0);
            if(vb_y) *vb_y = _ctx_str_get_float (data, 1);
            if(vb_width) *vb_width = _ctx_str_get_float (data, 2);
            if(vb_height) *vb_height = _ctx_str_get_float (data, 3);
	}
	else if (in_svg && att == SQZ_width)
	{
	    c_width = mrg_parse_px_x (mrg, data, NULL);
            if(width) *width = c_width;
	}
	else if (in_svg && att == SQZ_height)
	{
	    c_height = mrg_parse_px_y (mrg, data, NULL);
            if(height) *height = c_height;
	}
        break;
      case t_endtag:
        in_svg = 0;
        break;
    }
  }

  xmltok_free (xmltok);

  if (vb_width && (*vb_width == 0.0f)){
    if (vb_x) *vb_x = 0;
    *vb_width = c_width;
  }
  if (vb_height && (*vb_height == 0.0f)){
    if (vb_y) *vb_y = 0;
    *vb_height = c_height;
  }
  if (width && (*width == 0.0f) && vb_width)
  {
    *width = *vb_width;
  }
  if (height && (*height == 0.0f) && vb_height)
  {
    *height = *vb_height;
  }

  return 0;
}

void css_xml_renderf (Css *mrg,
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
  css_xml_render (mrg, uri_base, link_cb, link_data, NULL, NULL, buffer);
  free (buffer);
}
void css_init (Css *mrg, Ctx *ctx, int width, int height);

void css_print_xml (Css *mrg, const char *xml)
{
  css_xml_render (mrg, NULL, NULL, NULL, NULL, NULL, (char*)xml);
}

void
css_printf_xml (Css *mrg, const char *format, ...)
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
  css_print_xml (mrg, buffer);
  free (buffer);
}

void mrg_set_size (Css *mrg, int width, int height)
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
    buffer = calloc(1, 2048);
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



/* caching uri fetcher
 */
int
mrg_get_contents_default (const char  *referer,
                          const char  *input_uri,
                          char       **contents,
                          long        *length,
                          void        *ignored_user_data)
{
  Css *mrg = ignored_user_data;
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
    CacheEntry *entry = calloc (1, sizeof (CacheEntry));
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

void css_init (Css *mrg, Ctx *ctx, int width, int height)
{
  //memset (mrg, 0, sizeof (Css));
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
  //memset (mrg->state, 0, sizeof (CssState));
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

  if (!mrg->style)
    mrg->style = ctx_string_new ("");

  mrg_set_mrg_get_contents (mrg, mrg_get_contents_default, mrg);
  if (mrg->style_global)
    ctx_string_free (mrg->style_global, 1);
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

  css_stylesheet_clear (mrg);
  _mrg_clear_text_closures (mrg);
}

#if 0
Css *mrg_new (Ctx *ctx, int width, int height)
{
  Css *mrg;

  mrg = calloc (1, sizeof (Css));
  mrg->do_clip = 1;
  css_init (mrg, ctx, width, height);
  ctx_style_defaults (mrg);

#if 0
  printf ("%f %i %i\n", mrg->state->style.font_size, mrg_width(mrg), mrg_height(mrg));
  printf ("sizeof(Css) %li (was: 1142496)\n", sizeof(Css));
  printf ("sizeof(CssState) %li\n", sizeof(CssState));
  printf ("sizeof(CtxStyle) %li\n", sizeof(CtxStyle));
  printf ("sizeof(CssHtml) %li\n", sizeof(CssHtml));
  printf ("sizeof(CtxCssParseState) %li\n", sizeof(CtxCssParseState));
#endif

  return mrg;
}
#endif

void mrg_destroy (Css *mrg)
{
  if (mrg->edited_str)
    ctx_string_free (mrg->edited_str, 1);
  if (mrg->stylesheet)
    ctx_list_free (&mrg->stylesheet);
  if (mrg->style_global)
    ctx_string_free (mrg->style_global, 1);
  if (mrg->style)
    ctx_string_free (mrg->style, 1);
  if (mrg->css_parse_state)
    free (mrg->css_parse_state);
  mrg->edited_str = NULL;
  free (mrg);
}

typedef struct _UiChoice  UiChoice;
struct _UiChoice
{
  int   val;
  char *label;
};

void css_begin_menu_bar (Css *itk, const char *title)
{
  if (itk->menu_path)
    free (itk->menu_path);
  itk->menu_path = title?strdup (title):NULL;
}

void css_begin_menu (Css *itk, const char *title)
{
  char *tmp = malloc (strlen (title) + (itk->menu_path?strlen (itk->menu_path):0) + 2);
  sprintf (tmp, "%s/%s", itk->menu_path?itk->menu_path:"", title);
  if (itk->menu_path)
          free (itk->menu_path);
  itk->menu_path = tmp;
  if (css_button (itk, title))
  {
     if (itk->active_menu_path) free (itk->active_menu_path);
     itk->active_menu_path = strdup (itk->menu_path);
  }; 
}

void css_menu_item (Css *itk, const char *title)
{
  char *tmp = malloc (strlen (title) + (itk->menu_path?strlen (itk->menu_path):0) + 2);
  sprintf (tmp, "%s/%s", itk->menu_path?itk->menu_path:"", title);
  //fprintf (stderr, "[%s]\n", tmp);
  free (tmp);
}

void css_end_menu (Css *itk)
{
  if (itk->menu_path)
  {
    char *split = strrchr (itk->menu_path, '/');
    if (split) *split = 0;
  }
}

void css_end_menu_bar (Css *itk)
{
  css_newline (itk);
}

static char *css_style=NULL;

const char *css_style_string (const char *name)
{
  if (!css_style)
    return NULL;
  char *p = css_style;
  static char ret[64];
  int name_len = strlen (name);
  while (p && *p)
  {
    while (*p == ' ')p++;
    if (!strncmp (p, name, name_len))
    {
      if (p[name_len]==':')
      {
        for (int i = 2; p[name_len+i] && (p[name_len+i] != ';')
                        && (p[name_len+i] != '\n'); i++)
        {
          ret[i-2]=p[name_len+i];
          ret[i-1]=0;
        }
        return ret;
      }
    }
    else
    {
      p = strchr (p, '\n');
      if (p) p++;
    }
  }
  return NULL;
}

float css_style_float (char *name)
{
   const char *str = css_style_string (name);
   if (str)
   {
     return atof (str);
   }
   return 0.0f;
}

#if 1
void css_style_color (Ctx *ctx, const char *name)
{
   const char *str = css_style_string (name);
   if (str)
   {
     while (*str == ' ')str++;
     ctx_color (ctx, str);
     //ctx_stroke_source (ctx);
     //ctx_color (ctx, str);
   }
   else
   {
     ctx_rgb (ctx, 0, 0, 0); // XXX : this shows up in thumbnails
     //ctx_rgb_stroke (ctx, 1, 0, 1);
   }
}
#endif

static void css_style_color3 (Css *itk, const char *klass, uint32_t attr)
{           
   Ctx *ctx = itk->ctx;
   CtxStyleNode ancestor;
   CtxStyleNode *ancestry[2] = {&ancestor, NULL};
   memset(&ancestor, 0, sizeof (CtxStyleNode));
   ancestry[0]->element_hash = ctx_strhash ("div");
   ancestry[0]->classes_hash[0] = ctx_strhash (klass);
   // XXX : fix this casting hack, some stack waste is better then type mismatch?
   char *collated = _ctx_css_compute_style (itk, ancestry, 1);
   ctx_save (itk->ctx);
   css_set_style (itk, collated);
   CtxColor *color = ctx_color_new ();
   ctx_get_color (ctx, attr, color);
   ctx_restore (itk->ctx);
   mrg_ctx_set_source_color (ctx, color);
   ctx_color_free (color);
   free (collated);
}

void css_style_bg (Css *itk, const char *klass)
{
  css_style_color3 (itk, klass, SQZ_background_color);
}

void css_style_fg (Css *itk, const char *klass)
{
  css_style_color3 (itk, klass, SQZ_color);
}

float css_rel_ver_advance (Css *itk)
{
  return itk->rel_ver_advance;
}

Css *css_new (Ctx *ctx)
{
  Css *itk              = calloc (1, sizeof (Css));
  //itk->ctx              = ctx;
  //itk->panels = NULL;
  itk->focus_wraparound = 1;
  itk->scale            = 1.0;
  itk->font_size        = getenv("CSS_FONT_SIZE")?atoi(getenv("CSS_FONT_SIZE")):ctx_get_font_size(ctx);
  itk->label_width      = 0.5;
  itk->rel_vmargin      = 0.5;
  itk->rel_hmargin      = 0.5;
  itk->rel_ver_advance  = 1.2;
  itk->menu_path = strdup ("main/foo");
  itk->rel_hpad         = 0.3;
  itk->rel_vgap         = 0.2;
  itk->scroll_speed     = 1.0/8.0;
  itk->light_mode       = 1;
  ctx_queue_draw (ctx);
  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM)
  {
    itk->scale     = 1.0;
    itk->font_size = 3;
    itk->rel_vgap = 0.0;
    itk->rel_ver_advance = 1.0;
    itk->rel_hpad = 0.0;
  }
  itk->width = itk->font_size * 15;

  Css *mrg = (Css*)itk;
  css_init (mrg, ctx, ctx_width(ctx), ctx_height(ctx));
  ctx_style_defaults (mrg);

  //printf ("%f %i %i\n", mrg->state->style.font_size, mrg_width(mrg), mrg_height(mrg));

  return itk;
}

float css_em (Css *itk)
{
  return itk->font_size * itk->scale;
}

void css_destroy (Css *itk)
{
  if (itk->menu_path)
    free (itk->menu_path);
  if (itk->stylesheet)
    ctx_list_free (&itk->stylesheet);
  if (itk->style_global)
    ctx_string_free (itk->style_global, 1);
  if (itk->style)
    ctx_string_free (itk->style, 1);
  if (itk->css_parse_state)
    free (itk->css_parse_state);
 

  free (itk);
}

static inline void control_ref (CtxControl *control)
{
  control->ref_count ++;
}

static inline void control_unref (CtxControl *control)
{
  if (control->ref_count <= 0)
  {
    CtxControl *w = control;

    if (w->label)
      free (w->label);
    if (w->fallback)
      free (w->fallback);
    if (w->entry_value)
      free (w->entry_value);

    free (w);
    return;
  }
  control->ref_count--;
}

void control_finalize (void *control, void *foo, void *bar)
{
  control_unref (control);
}

void css_reset (Css *itk)
{
  Ctx *ctx = itk->ctx = itk->document_ctx;
  ctx_start_frame           (ctx);

  if (css_style)
    free (css_style);
  unsigned char *style = NULL;
#if CTX_FONTS_FROM_FILE
  //ctx_get_contents ("/tmp/itk-style", &style, NULL);
#endif
  if (style)
  {
    css_style = (void*)style;
  }
  else
  {
    css_style = strdup (
"\n"
"itk-font-size: 32.0;\n"

"titlebar-bg:          #0007;\n"
"titlebar-fg:          #999a;\n"
"titlebar-close:       #fff9;\n"
"titlebar-focused-close: #c44;\n"
"titlebar-focused-bg:  #333b;\n"
"titlebar-focused-fg:  #ffff;\n"
"\n"
"terminal-bg:         #000f;\n"
"terminal-bg-reverse: #ddde;\n"
"terminal-active-bg:         #000b;\n"
"terminal-active-bg-reverse: #dddb;\n"
"\n"
"itk-bg:             rgba(30,40,50, 1.0);\n"
"itk-fg:             rgb(225,225,225);\n"
"\n"
    );

  }

  ctx_save (ctx);
  ctx_font (ctx, "Regular");
  ctx_font_size (ctx, css_em (itk));

  itk->next_flags = CSS_FLAG_DEFAULT;
  itk->panel      = NULL;

  while (itk->old_controls)
  {
    CtxControl *control = itk->old_controls->data;
    control_unref (control);
    ctx_list_remove (&itk->old_controls, control);
  }
  itk->old_controls = itk->controls;
  itk->controls = NULL;
  while (itk->choices)
  {
    UiChoice *choice = itk->choices->data;
    ctx_list_remove (&itk->choices, choice);
    free (choice->label);
    free (choice);
  }
  itk->control_no = 0;
  css_init ((Css*)itk, ctx, ctx_width (itk->ctx), ctx_height (itk->ctx));
  //mrg_clear (itk);
  ctx_clear_bindings (itk->ctx);
  //css_stylesheet_clear ((Css*)itk);
  ctx_style_defaults ((Css*)itk);
}

CssPanel *add_panel (Css *itk, const char *label, float x, float y, float width, float height)
{
  CssPanel *panel;
  for (CtxList *l = itk->panels; l; l = l->next)
  {
    CssPanel *panel = l->data;
    if (!strcmp (panel->title, label))
      return panel;
  }
  panel = calloc (1, sizeof (CssPanel));
  panel->title = strdup (label);
  panel->x = x;
  panel->y = y;
  panel->width = width;
  panel->height = height;
  ctx_list_prepend (&itk->panels, panel);

  itk->panel = panel;
  return panel;
}

void
css_panels_reset_scroll (Css *itk)
{
  if (!itk || !itk->panels)
          return;
  for (CtxList *l = itk->panels; l; l = l->next)
  {
    CssPanel *panel = l->data;
    panel->scroll = 0.0;
    panel->do_scroll_jump = 1;
  }
}


/* adds a control - should be done before the drawing of the
 * control itself - as this call might draw a highlight in
 * the background.
 *
 * This also allocats a runtime control, used for focus handling
 * and ephemreal persistance of possible interaction states -
 * useful for accesibility.
 */
CtxControl *css_add_control (Css *itk,
                             int type,
                             const char *label,
                             float x, float y,
                             float width, float height)
{
  CtxControl *control = calloc (1, sizeof (CtxControl));
  float em = css_em (itk);
  control->flags = itk->next_flags;
  itk->next_flags = CSS_FLAG_DEFAULT;
  control->label = strdup (label);
  if (itk->next_id)
  {
    control->id = itk->next_id;
    itk->next_id = NULL;
  }

  control->type = type;
  control->ref_count=0;
  control->x = x;
  control->y = y;
  control->no = itk->control_no;
  itk->control_no++;
  control->width = width;
  control->height = height;
  ctx_list_prepend (&itk->controls, control);

  if (itk->focus_no == control->no && itk->focus_no >= 0)
  {
     if (itk->y - itk->panel->scroll < em * 2)
     {
        if (itk->panel->scroll != 0.0f)
        {
          itk->panel->scroll -= itk->scroll_speed * itk->panel->height * (itk->panel->do_scroll_jump?5:1);
          if (itk->panel->scroll<0.0)
            itk->panel->scroll=0.0;
          ctx_queue_draw (itk->ctx);
        }
     }
     else if (itk->y - itk->panel->scroll +  control->height > itk->panel->y + itk->panel->height - em * 2 && control->height < itk->panel->height - em * 2)
     {
          itk->panel->scroll += itk->scroll_speed * itk->panel->height * (itk->panel->do_scroll_jump?5:1);

        ctx_queue_draw (itk->ctx);
     }
     else
     {
       itk->panel->do_scroll_jump = 0;
     }

  }

  //ctx_rectangle (itk->ctx, x, y, width, height);
  if (itk->focus_no == control->no &&
      control->type == UI_LABEL)   // own-bg
  {
#if 1
    //css_style_bg (itk, "focused");
    //ctx_fill (itk->ctx);
#if 0
    ctx_rectangle (itk->ctx, x, y, width, height);
    css_style_color (itk->ctx, "itk-fg");
    ctx_line_width (itk->ctx, 2.0f);
    ctx_stroke (itk->ctx);
#endif
#endif
  }
  else
  {
    if (control->flags & CSS_FLAG_ACTIVE)
    if (control->type != UI_LABEL && // no-bg
        control->type != UI_BUTTON)  // own-bg
    {
     // css_style_bg (itk, "interactive");
     // ctx_fill (itk->ctx);
    }
  }

  switch (control->type)
  {
    case UI_SLIDER:
      control->ref_count++;
      break;
    default:
      break;
  }

  return control;
}

#if 0
static void css_base (Css *itk, const char *label, float x, float y, float width, float height, int focused)
{
  Ctx *ctx = itk->ctx;
  if (focused)
  {
    css_style_color (itk->ctx, "itk-focused-bg");

    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
  }
#if 0
  else
    css_style_color (itk->ctx, "itk-bg");

  if (itk->line_no >= itk->lines_drawn)
  {
    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
    itk->lines_drawn = itk->line_no -1;
  }
#endif
}
#endif

void css_newline (Css *itk)
{
  itk->y += css_em (itk) * (itk->rel_ver_advance + itk->rel_vgap);
  itk->x = itk->edge_left;
  itk->line_no++;
}

void css_seperator (Css *itk)
{
  Css *mrg = (Css*)itk;
  css_start (mrg, "hr", NULL);
  css_end (mrg, NULL);
}

void css_label (Css *itk, const char *label)
{
  Css *mrg = (Css*)itk;
  //css_start (mrg, "label", NULL);
  css_print (mrg, label);
  //css_end (mrg, NULL);
}

void css_labelf (Css *itk, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  css_label (itk, buffer);
  free (buffer);
}

static void titlebar_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CssPanel *panel = userdata;
  
#if 1
  //fprintf (stderr, "%d %f %f\n", event->delta_x, event->delta_y);
  panel->x += event->delta_x;
  panel->y += event->delta_y;
  if (panel->y < 0) panel->y = 0;
#else
  panel->x = event->x - panel->width / 2;
  panel->y = event->y;
#endif

  event->stop_propagate = 1;
  ctx_queue_draw (itk->ctx);
}

void css_titlebar (Css *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = css_em (itk);

  //CtxControl *control = css_add_control (itk, UI_TITLEBAR, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, titlebar_drag, itk->panel, itk, NULL, NULL);

  ctx_begin_path (ctx);
  itk->line_no = 0;
  itk->lines_drawn = 0;
  //css_base (itk, label, control->x, control->y, control->width - em * itk->rel_hmargin, em * itk->rel_ver_advance, itk->focus_no == control->no);
  css_label (itk, label);
  //itk->lines_drawn = 1;

  css_newline (itk);
}

void css_scroll_start (Css *itk, float height)
{
  CssPanel *panel = itk->panel;
  Ctx *ctx = itk->ctx;
  ctx_save (ctx);
  itk->panel->scroll_start_y = itk->y;
  ctx_rectangle (ctx, itk->edge_left - itk->rel_hmargin*css_em(itk), itk->y, panel->width, panel->height - (itk->y - panel->y));
  ctx_clip (ctx);
  ctx_begin_path (ctx);
  ctx_translate (ctx, 0.0, -panel->scroll);
}

// applies to next created control
void css_id (Css *itk, void *id)
{
  itk->next_id = id; 
}

// applies to next created control
void css_set_flag (Css *itk, int flag, int on)
{
  if (on)
  {
    itk->next_flags |= flag;
  }
  else
  {
    if (itk->next_flags & flag)
      itk->next_flags -= flag;
  }
}

void css_scroll_drag (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CssPanel *panel = data2;
  float scrollbar_height = panel->height - (panel->scroll_start_y - panel->y);
  float th = scrollbar_height * (scrollbar_height /  (panel->max_y-panel->scroll_start_y));
  if (th > scrollbar_height)
  {
    panel->scroll = 0;
    event->stop_propagate = 1;
    ctx_queue_draw (itk->ctx);
    return;
  }
  panel->scroll = ((event->y - panel->scroll_start_y - th / 2) / (scrollbar_height-th)) *
               (panel->max_y - panel->scroll_start_y - scrollbar_height)
          ;
  itk->focus_no = -1;
  ctx_queue_draw (itk->ctx);

  if (panel->scroll < 0) panel->scroll = 0;

  event->stop_propagate = 1;
}

void css_scroll_end (Css *itk)
{
  CssPanel *panel = itk->panel;
  Ctx *ctx = itk->ctx;
  float em = css_em (itk);
  ctx_restore (ctx);
  itk->panel->max_y = itk->y;

  float scrollbar_height = panel->height - (panel->scroll_start_y - panel->y);
  float scrollbar_width = em;

#if 1
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, panel->x + panel->width- scrollbar_width,
                      panel->scroll_start_y,
                      scrollbar_width,
                      scrollbar_height);
  ctx_listen (ctx, CTX_DRAG, css_scroll_drag, itk, panel);
  css_style_bg (itk, "scroll");
  ctx_fill (ctx);
#endif

  ctx_begin_path (ctx);
  float th = scrollbar_height * (scrollbar_height /  (panel->max_y-panel->scroll_start_y));
  if (th > scrollbar_height) th = scrollbar_height;

  ctx_rectangle (ctx, panel->x + panel->width- scrollbar_width,
                      panel->scroll_start_y +
                      (panel->scroll / (panel->max_y-panel->scroll_start_y)) * ( scrollbar_height ),
                      scrollbar_width,
                      th
                      
                      );

  css_style_fg (itk, "scroll");
  ctx_fill (ctx);


}

CssPanel *css_panel_start (Css *itk, const char *title,
                      int x, int y, int width, int height)
{
  Ctx *ctx = itk->ctx;
  CssPanel *panel = add_panel (itk, title, x, y, width, height);
  float em = css_em (itk);
  css_set_edge_left (itk, panel->x + em * itk->rel_hmargin);
  css_set_edge_top (itk, panel->y);
  css_set_xy (itk, css_edge_left (itk), panel->y);
  //itk->edge_left = itk->x = panel->x + em * itk->rel_hmargin;
  //itk->edge_top = itk->y = panel->y;

  if (panel->width != 0)
  {
    panel->width = width;
    panel->height = height;
  }
  itk->width = panel->width;
  itk->height = panel->height;
  css_set_wrap_width (itk, panel->width - em);
  //css_set_height (itk, panel->height);

  itk->panel = panel;

  css_style_fg (itk, "itk");
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, panel->x, panel->y, panel->width, panel->height);
  ctx_line_width (ctx, 2);
  ctx_stroke (ctx);

  css_style_bg (itk, "wallpaper");
  ctx_rectangle (ctx, panel->x, panel->y, panel->width, panel->height);
  ctx_fill (ctx);

  if (title[0])
    css_titlebar (itk, title);

  css_scroll_start (itk, panel->height - (itk->y - panel->y));
  css_start (itk, "div", NULL);
  return panel;
}

void css_panel_resize_drag (CtxEvent *event, void *data, void *data2)
{
  CssPanel *panel = data;
  panel->width += event->delta_x;
  panel->height += event->delta_y;
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

void css_panel_end (Css *itk)
{
  Ctx *ctx = itk->ctx;
  CssPanel *panel = itk->panel;
  float em = css_em (itk);
  css_end ((Css*)itk, NULL);
  css_scroll_end (itk);

  ctx_rectangle (ctx, panel->x + panel->width - em,
                      panel->y + panel->height - em,
                      em,
                      em);
  ctx_listen (ctx, CTX_DRAG, css_panel_resize_drag, panel, itk);
  css_style_fg (itk, "wallpaper");
  ctx_begin_path (ctx);
  ctx_move_to (ctx, panel->x + panel->width,
                    panel->y + panel->height);
#if 1
  ctx_rel_line_to (ctx, -em, 0);
  ctx_rel_line_to (ctx, em, -em);
  ctx_rel_line_to (ctx, 0, em);
#endif
  ctx_fill (ctx);

  itk->panel = NULL;
}

static void css_float_constrain (CtxControl *control, float *val)
{
  float new_val = *val;
  if (new_val < control->min) new_val = control->min;
  if (new_val > control->max) new_val = control->max;
  if (new_val > 0)
  {
     if (control->step > 0.0)
     {
       new_val = (int)(new_val / control->step) * control->step;
     }
  }
  else
  {
     if (control->step > 0.0)
     {
       new_val = -new_val;
       new_val = (int)(new_val / control->step) * control->step;
       new_val = -new_val;
     }
  }
  *val = new_val;
}

#if 0
static void css_slider_cb_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl  *control = userdata;
  float new_val;

  css_set_focus_no (itk, control->no);
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
  new_val = ((event->x - control->x) / (control->width)) * (control->max-control->min) + control->min;

  css_float_constrain (control, &new_val);

  itk->return_value = 1;
  control->value = new_val;
  itk->slider_value = new_val;
  //if (control->set_val)
  //  control->set_val (control->val, new_val, control->data);
}
#endif

float css_slider (Css *itk, const char *label, float value, double min, double max, double step)
{
#if 0
  Ctx *ctx = itk->ctx;
  char buf[100] = "";
  float em = css_em (itk);

  float new_x = itk->x + (itk->label_width) * itk->width;
  itk->x = new_x;

  CtxControl *control = css_add_control (itk, UI_SLIDER, label, itk->x, itk->y, itk->width * (1.0 - itk->label_width) - em * 1.5, em * itk->rel_ver_advance);
  //control->data = data;
  //
  control->value  = value;
  control->min  = min;
  control->max  = max;
  control->step = step;

  if (itk->focus_no == control->no)
    css_style_color (itk->ctx, "itk-focused-bg");
  else
    css_style_color (itk->ctx, "itk-interactive-bg");
  ctx_rectangle (ctx, itk->x, itk->y, control->width, em * itk->rel_ver_advance);
  ctx_fill (ctx);
  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, control->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, css_slider_cb_drag, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);

  double fval = value;

  if (step == 1.0)
  {
    sprintf (buf, "%.0f", fval);
  }
  else
  {
    sprintf (buf, "%.3f", fval);
  }
  css_style_color (itk->ctx, "itk-slider-text");
  ctx_text (ctx, buf);

  float rel_val = ((fval) - min) / (max-min);
  css_style_color (itk->ctx, "itk-slider-cursor");
  ctx_rectangle (ctx, itk->x + control->width * rel_val, itk->y, em/8, control->height);
  ctx_fill (ctx);
  ctx_rectangle (ctx, itk->x, itk->y + em*5/6, control->width, em/8);
  ctx_fill (ctx);

  itk->x += (1.0 - itk->label_width) * itk->width;
  css_newline (itk);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return itk->slider_value;
  }
  return value;
#else
  Css *mrg = (Css*)itk;
  Ctx *ctx = itk->ctx;

  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);
  css_label (itk, label);

  CtxFloatRectangle extent;
  css_start (mrg, "slider", NULL);
  css_printf (mrg, "%f", value);
  css_end (mrg, &extent);
  CtxControl *control = css_add_control (itk, UI_SLIDER, label,
                                         extent.x, extent.y, extent.width, extent.height);
  control->value  = value;
  control->min  = min;
  control->max  = max;
  control->step = step;
  css_end (mrg, NULL);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return itk->slider_value;
  }
  return value;
#endif
}

void css_slider_float (Css *itk, const char *label, float *val, float min, float max, float step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_int (Css *itk, const char *label, int *val, int min, int max, int step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_double (Css *itk, const char *label, double *val, double min, double max, double step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_uint8 (Css *itk, const char *label, uint8_t *val, uint8_t min, uint8_t max, uint8_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_uint16 (Css *itk, const char *label, uint16_t *val, uint16_t min, uint16_t max, uint16_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_uint32 (Css *itk, const char *label, uint32_t *val, uint32_t min, uint32_t max, uint32_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_int8 (Css *itk, const char *label, int8_t *val, int8_t min, int8_t max, int8_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_int16 (Css *itk, const char *label, int16_t *val, int16_t min, int16_t max, int16_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

void css_slider_int32 (Css *itk, const char *label, int32_t *val, int32_t min, int32_t max, int32_t step)
{
  *val = css_slider (itk, label, *val, min, max, step);
}

CtxControl *css_find_control (Css *itk, int no)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == no)
     return control;
  }
  return NULL;
}


void css_entry_commit (Css *itk)
{
  if (itk->active_entry<0)
     return;

  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->active_entry)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (itk->entry_copy)
         {
  //fprintf (stderr, "ec %i %s\n", itk->active_entry, itk->entry_copy);
  //         strcpy (control->val, itk->entry_copy);
 //          free (itk->entry_copy);
 //          itk->entry_copy = NULL;
           itk->active = 2;
           ctx_queue_draw (itk->ctx);
         }
      }
      return;
    }
  }
}

void css_lost_focus (Css *itk)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->active_entry)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (itk->active_entry < 0)
           return;
         if (control->flags & CSS_FLAG_CANCEL_ON_LOST_FOCUS)
         {
           itk->active_entry = -1;
           itk->active = 0;
         }
         else if (itk->entry_copy)
         {
           itk->active = 2;
         }
         ctx_queue_draw (itk->ctx);
         break;
        default :
         itk->active = 0;
         itk->choice_active = 0;
         ctx_queue_draw (itk->ctx);
         break;
      }
      return;
    }
  }
}

void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;

  if (itk->active)
  {
    css_lost_focus (itk);
  }
  else
  {
    itk->entry_copy = strdup (control->entry_value);
    itk->entry_pos = strlen (itk->entry_copy);
    itk->active = 1;
    itk->active_entry = control->no;
  }

  css_set_focus_no (itk, control->no);
  ctx_queue_draw (event->ctx);
}


char *css_entry (Css        *itk,
                 const char *label,
                 const char *fallback,
                 const char *in_val)
{
  Css *mrg = (Css*)itk;
#if 1
  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);
  css_label (itk, label);

  if (itk->active &&
      itk->entry_copy && itk->focus_no == itk->control_no)
  {
    int backup = itk->entry_copy[itk->entry_pos];
    char buf[4]="|";
    itk->entry_copy[itk->entry_pos]=0;
    css_label (itk, itk->entry_copy);
    css_label (itk, buf);

    buf[0]=backup;
    if (backup)
    {
      css_label (itk, buf);
      css_label (itk, &itk->entry_copy[itk->entry_pos+1]);
      itk->entry_copy[itk->entry_pos] = backup;
    }
  }
  else
  {
    if (in_val[0])
    {
      css_label (itk, in_val);
    }
    else
    {
      if (fallback)
      {
        css_label (itk, fallback);
      }
    }
  }

  CtxFloatRectangle extent;
  css_end (mrg, &extent);
  CtxControl *control = css_add_control (itk, UI_ENTRY, label,
                  extent.x, extent.y, extent.width, extent.height);
  control->entry_value = strdup (in_val);
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;
#else
  float new_x = itk->x + itk->label_width * itk->width;

  float ewidth = itk->width * (1.0 - itk->label_width);

  if (label[0]) {
    css_label (itk, label);
    itk->x = new_x;
  }
  else
  {
    ewidth = itk->width;
  }
  CtxControl *control = css_add_control (itk, UI_ENTRY, label, itk->x, itk->y, ewidth, em * itk->rel_ver_advance);
  control->entry_value = strdup (in_val);
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);

  if (control->flags & CSS_FLAG_ACTIVE)
  {
    ctx_listen_with_finalize (ctx, CTX_CLICK, entry_clicked, control, itk, control_finalize, NULL);
    control_ref (control);
  }

  ctx_begin_path (ctx);
  ctx_move_to (ctx, itk->x, itk->y + em);
  if (itk->active &&
      itk->entry_copy && itk->focus_no == control->no)
  {
    int backup = itk->entry_copy[itk->entry_pos];
    char buf[4]="|";
    itk->entry_copy[itk->entry_pos]=0;
    css_style_color (itk->ctx, "itk-interactive");
    ctx_text (ctx, itk->entry_copy);
    css_style_color (itk->ctx, "itk-entry-cursor");
    ctx_text (ctx, buf);

    buf[0]=backup;
    if (backup)
    {
      css_style_color (itk->ctx, "itk-interactive");
      ctx_text (ctx, buf);
      ctx_text (ctx, &itk->entry_copy[itk->entry_pos+1]);
      itk->entry_copy[itk->entry_pos] = backup;
    }
  }
  else
  {
    if (in_val[0])
    {
      if (control->flags & CSS_FLAG_ACTIVE)
        css_style_color (itk->ctx, "itk-interactive");
      else
        css_style_color (itk->ctx, "itk-fg");
      ctx_text (ctx, in_val);
    }
    else
    {
      if (control->fallback)
      {
        css_style_color (itk->ctx, "itk-entry-fallback");
        ctx_text (ctx, control->fallback);
      }
    }
  }
  itk->x += ewidth;
  css_newline (itk);
#endif

  if (itk->active == 2 && control->no == itk->active_entry)
  {
    itk->active = 0;
    itk->active_entry = -1;
    char *copy = itk->entry_copy;
    itk->entry_copy = NULL;
    ctx_queue_draw (itk->ctx); // queue a draw - since it is likely the
                               // response triggers computation
    return copy;
  }
  return NULL;
}

int css_entry_str_len (Css *itk, const char *label, const char *fallback, char *val, int maxlen)
{
   char *new_val;
   if ((new_val = css_entry (itk, label, fallback, val)))
   {
      if ((int)strlen (new_val) > maxlen -1)
        new_val[maxlen-1]=0;
      strcpy (val, new_val);
      free (new_val);
      return 1;
   }
   return 0;
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  itk->return_value = 1; // reusing despite name
  *val = (*val)?0:1;
  event->stop_propagate = 1;
  css_set_focus_no (itk, control->no);
  ctx_queue_draw (event->ctx);
}

static void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
  css_set_focus_no (itk, control->no);
  itk->return_value = 1;
}

int css_toggle (Css *itk, const char *label, int input_val)
{
  Ctx *ctx = itk->ctx;
  Css *mrg = (Css*)itk;
  //float em = css_em (itk);
  //float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;
  CtxFloatRectangle extent;
  
  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);
  css_start (mrg, "toggle", NULL);

  css_print (mrg, label);

  if (input_val)
  {
      css_print (mrg, "[x]");
  }
  else
  {
      css_print (mrg, "[ ]");
  }

  css_end (mrg, &extent);
  css_end (mrg, NULL);
  CtxControl *control = css_add_control (itk, UI_TOGGLE, label,
                  extent.x, extent.y, extent.width, extent.height);

  control_ref (control);
  control->type = UI_TOGGLE;
  ctx_rectangle (ctx, extent.x, extent.y, extent.width, extent.height);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return !input_val;
  }
  return input_val;
}


int css_radio (Css *itk, const char *label, int set)
{
  Ctx *ctx = itk->ctx;
  Css *mrg = (Css*)itk;
  //float em = css_em (itk);
  //float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;
  CtxFloatRectangle extent;
  
  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);
  css_start (mrg, "toggle", NULL);

  css_print (mrg, label);

  if (set)
  {
      css_print (mrg, "(x)");
  }
  else
  {
      css_print (mrg, "( )");
  }

  css_end (mrg, &extent);
  css_end (mrg, NULL);
  CtxControl *control = css_add_control (itk, UI_TOGGLE, label,
                  extent.x, extent.y, extent.width, extent.height);

  control_ref (control);
  control->type = UI_TOGGLE;
  ctx_rectangle (ctx, extent.x, extent.y, extent.width, extent.height);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return !set;
  }
  return set;
}

void expander_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  css_set_focus_no (itk, control->no);
  ctx_queue_draw (event->ctx);
}

int css_expander (Css *itk, const char *label, int *val)
{
  Css *mrg = (Css*)itk;
  CtxFloatRectangle extent;
  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);

  css_labelf (itk, "%s %s", *val?"V":">", label);

  css_end (mrg, &extent);
  CtxControl *control = css_add_control (itk, UI_EXPANDER, label,
                  extent.x, extent.y, extent.width, extent.height);
  control->val = val;

  return *val;
}

int css_button (Css *itk, const char *label)
{
#if 1
  Ctx *ctx = itk->ctx;
  Css *mrg = (Css*)itk;
  //float em = css_em (itk);
  //float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;
  CtxFloatRectangle extent;
   
  css_start (mrg, itk->focus_no == itk->control_no ? "button:focused" : "button", NULL);

//  css_label (itk, label);
  css_print (mrg, label);


  css_end (mrg, &extent);
  CtxControl *control = css_add_control (itk, UI_BUTTON, label,
                  extent.x, extent.y, extent.width, extent.height);
                             //itk->x, itk->y, width, em * itk->rel_ver_advance);

  control_ref (control);
  control->type = UI_BUTTON;
  ctx_rectangle (ctx, extent.x, extent.y, extent.width, extent.height);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_rgb (ctx, 1,1,0);
  ctx_fill (ctx);
  ctx_begin_path (ctx);

  //css_newline (itk);
  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return 1;
  }
  return 0;
#else

  Ctx *ctx = itk->ctx;
  float em = css_em (itk);
  float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;



  CtxControl *control = css_add_control (itk, UI_BUTTON, label,
                             itk->x, itk->y, width, em * itk->rel_ver_advance);

  css_style_color (itk->ctx, "itk-button-shadow");
  ctx_begin_path (ctx);
  ctx_round_rectangle (ctx, itk->x + em * 0.1, itk->y + em * 0.1, width, em * itk->rel_ver_advance, em*0.33);
  ctx_fill (ctx);

  {
    float px = ctx_pointer_x (itk->ctx);
    float py = ctx_pointer_y (itk->ctx);
    if (px >= control->x && px <= control->x + control->width &&
        py >= control->y && py <= control->y + control->height)
    {
      css_style_color (itk->ctx, "itk-button-hover-bg");
    }
  else
    {
  if (itk->focus_no == control->no)
    css_style_color (itk->ctx, "itk-button-focused-bg");
  else
    css_style_color (itk->ctx, "itk-interactive-bg");
  }
  }

  ctx_round_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance, em * 0.33);
  ctx_fill (ctx);


  css_style_color (itk->ctx, "itk-button-fg");
  ctx_move_to (ctx, itk->x + em * itk->rel_hpad,  itk->y + em * itk->rel_baseline);
  ctx_text (ctx, label);

  control_ref (control);
  control->type = UI_BUTTON;
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);

//  css_newline (itk);
  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return 1;
  }
  return 0;

#endif
}

static void css_choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata2;
  CtxControl *control = userdata;
  itk->choice_active = 1;
  itk->choice_no = control->value;
  css_set_focus_no (itk, control->no);
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

int css_choice (Css *itk, const char *label, int val)
{
  Ctx *ctx = itk->ctx;
  Css *mrg = (Css*)itk;
  //float em = css_em (itk);
  //float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;
  CtxFloatRectangle extent;
  
  if (itk->focus_no == itk->control_no)
    css_start (mrg, "propline:focused", NULL);
  else
    css_start (mrg, "propline", NULL);

  for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
  {
    UiChoice *choice = l->data;
    if (choice->val == val)
      css_printf (mrg, "%s %s", label, choice->label);
  }

  //css_end (mrg, NULL);
  css_end (mrg, &extent);
  CtxControl *control = css_add_control (itk, UI_CHOICE, label,
                  extent.x, extent.y, extent.width, extent.height);
  control->value = val;
  control_ref (control);


  ctx_rectangle (ctx, extent.x, extent.y, extent.width, extent.height);
  ctx_listen_with_finalize (ctx, CTX_CLICK, css_choice_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  if (itk->focus_no == itk->control_no-1)
  {
    Css *mrg = (Css*)itk;
    if (itk->choice_active)
    {
      css_start (mrg, "div.choice_menu_wrap", NULL);
      css_start (mrg, "div.choice_menu", NULL);

      for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
      {
        UiChoice *choice = l->data;
       if (((int)control->value) == choice->val)
         css_start (mrg, "div.choice:chosen", NULL);
       else
         css_start (mrg, "div.choice", NULL);
       css_print (mrg, choice->label);
       css_end (mrg, NULL);
     }
     css_end (mrg, NULL);
     css_end (mrg, NULL);
    }
    if (!itk->choice_active)
    {
      itk->choice_no = val;
    }
    else
    {
      control->value = val;
    }
    itk->popup_x = control->x;
    itk->popup_y = control->y + (itk->panel?-itk->panel->scroll:0);
    itk->popup_width = control->width;
    itk->popup_height = control->height;
    if (itk->return_value)
    {
      itk->return_value = 0;
      ctx_queue_draw (itk->ctx);
      return itk->choice_no;
    }
  }

  return val;
}

void css_choice_add (Css *itk, int value, const char *label)
{
  UiChoice *choice= calloc (1, sizeof (UiChoice));
  choice->val = value;
  choice->label = strdup (label);
  ctx_list_append (&itk->choices, choice);
}


void css_set_focus_no (Css *itk, int pos)
{
   if (itk->focus_no != pos)
   {
     itk->focus_no = pos;

     css_lost_focus (itk);
     ctx_queue_draw (itk->ctx);
   }
}

CtxControl *css_hovered_control(Css *itk)
{
  float px = ctx_pointer_x (itk->ctx);
  float py = ctx_pointer_y (itk->ctx);
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (px >= control->x && px <= control->x + control->width &&
        py >= control->y && py <= control->y + control->height)
    {
      return control;
    }
  }
  return NULL;
}

CtxControl *css_focused_control(Css *itk)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->focus_no)
    {
      return control;
    }
  }


  return NULL;
}

#define CSS_DIRECTION_PREVIOUS  -1
#define CSS_DIRECTION_NEXT       1
#define CSS_DIRECTION_LEFT       2
#define CSS_DIRECTION_RIGHT      3
#define CSS_DIRECTION_UP         4
#define CSS_DIRECTION_DOWN       5

void css_focus (Css *itk, int dir)
{
   css_lost_focus (itk);
   if (itk->focus_no < 0)
   {
     itk->focus_no = 0;
     return;
   }

   if (dir == CSS_DIRECTION_PREVIOUS ||
       dir == CSS_DIRECTION_NEXT)
   {
     itk->focus_no += dir;

     int n_controls = ctx_list_length (itk->controls);
     CtxList *iter = ctx_list_nth (itk->controls, n_controls-itk->focus_no-1);
     if (iter)
     {
       CtxControl *control = iter->data;
       itk->focus_no = control->no;
     }
     else
     {
       if (itk->focus_wraparound)
       {
         if (itk->focus_no > 1)
           itk->focus_no = 0;
         else
           itk->focus_no = itk->control_no - 1;
       }
       else
       {
         if (itk->focus_no <= 1)
           itk->focus_no = 0;
         else
           itk->focus_no = itk->control_no - 1;
       }
     }
     // XXX no control means inifinie loop?
     CtxControl *control =
             css_focused_control (itk);
#if 1
     if (!control || 
         !(control->flags & CSS_FLAG_ACTIVE)){
       css_focus (itk, dir);
     }
#endif
   }
  else 
  {
    /* this implements the non-inner element portions of:
     *   https://drafts.csswg.org/css-nav-1/#find-the-shortest-distance
     *
     * validity is determined by the centers of the items.
     */

    CtxControl *control = css_focused_control (itk);
    CtxControl *best = control;

    if (!best)
       best = itk->controls->data;

    float best_dist = 10000000000.0;
    {
      //float mid_ref_x = control->x + control->width/2;
      //float mid_ref_y = control->y + control->height/2;
      for (CtxList *iter = itk->controls; iter; iter=iter->next)
      {
        CtxControl *candidate = iter->data;
        //float mid_cand_x = candidate->x + candidate->width/2;
        //float mid_cand_y = candidate->y + candidate->height/2;

        int valid = 0;
        if (candidate != control)
        switch (dir)
        {
          case CSS_DIRECTION_DOWN:
            valid = candidate->y > control->y;
            break;
          case CSS_DIRECTION_UP:
            valid = candidate->y < control->y;
            break;
          case CSS_DIRECTION_LEFT:
            valid = candidate->x < control->x;
            break;
          case CSS_DIRECTION_RIGHT:
            valid = candidate->x > control->x;
            break;
        }

        if (valid)
        {
          float cand_coord[2]={0.f,0.f};
          float control_coord[2]={0.f,0.f};

          float overlap = 0.0f;
          float orthogonalSize = 1.0f;
          float orthogonalBias = 1.0f;
          float orthogonalWeight = 1.0f;
          float alignWeight = 5.0f;


        switch (dir)
        {
          case CSS_DIRECTION_DOWN:
            control_coord[1] = control->y + control->height;
            cand_coord[1]    = candidate->y;
            orthogonalSize   = control->width;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 2.0f;

            if (candidate->x + candidate->width < control->x)
            {
////    ---------- control
///  --            candidate
              cand_coord[0]    = candidate->x + candidate->width;
              control_coord[0] = control->x;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///  ------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap = (candidate->x+candidate->width) - (control->x);
            }
            else if (candidate->x > control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///       ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///   --------------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap          = control->width;
            }
            else if (candidate->x > control->x &&
                     candidate->x < control->x + control->width &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///         ----------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = control->x + control->width -
                                 candidate->x;
            }
            else if (candidate->x > control->x + control->width)
            {
///     ----------
///                  ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = control->x + control->width;
            }
            else
            {
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }

            break;
          case CSS_DIRECTION_UP:

            control_coord[1] = control->y;
            cand_coord[1]    = candidate->y + candidate->height;
            orthogonalSize   = control->width;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 2.0f;

            if (candidate->x + candidate->width < control->x)
            {
////    ---------- control
///  --            candidate
              cand_coord[0]    = candidate->x + candidate->width;
              control_coord[0] = control->x;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///  ------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap = (candidate->x+candidate->width) - (control->x);
            }
            else if (candidate->x > control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///       ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///   --------------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap          = control->width;
            }
            else if (candidate->x > control->x &&
                     candidate->x < control->x + control->width &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///         ----------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = control->x + control->width -
                                 candidate->x;
            }
            else if (candidate->x > control->x + control->width)
            {
///     ----------
///                  ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = control->x + control->width;
            }
            else
            {
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }

            break;
          case CSS_DIRECTION_LEFT:
            control_coord[0] = control->x;
            cand_coord[0]    = candidate->x + candidate->width;
            orthogonalSize   = control->height;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 30.0f;

            if (candidate->y + candidate->height < control->y)
            {
////    ---------- control
///  --            candidate
              cand_coord[1]    = candidate->y + candidate->height;
              control_coord[1] = control->y;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///  ------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = (candidate->y+candidate->height) - (control->y);
            }
            else if (candidate->y > control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///       ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = candidate->height;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///   --------------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = control->height;
            }
            else if (candidate->y > control->y &&
                     candidate->y < control->y + control->height &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///         ----------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = control->y + control->height - candidate->y;
            }
            else if (candidate->y > control->y + control->height)
            {
///     ----------
///                  ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = control->y + control->height;
            }
            else
            {
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap          = candidate->height;
            }




            break;
          case CSS_DIRECTION_RIGHT:
            control_coord[0] = control->x + control->width;
            cand_coord[0]    = candidate->x;
            orthogonalSize   = control->height;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 30.0f;

            if (candidate->y + candidate->height < control->y)
            {
////    ---------- control
///  --            candidate
              cand_coord[1]    = candidate->y + candidate->height;
              control_coord[1] = control->y;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///  ------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = (candidate->y+candidate->height) - (control->y);
            }
            else if (candidate->y > control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///       ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = candidate->height;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///   --------------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = control->height;
            }
            else if (candidate->y > control->y &&
                     candidate->y < control->y + control->height &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///         ----------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = control->y + control->height - candidate->y;
            }
            else if (candidate->y > control->y + control->height)
            {
///     ----------
///                  ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = control->y + control->height;
            }
            else
            {
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap          = candidate->height;
            }

            break;
        }

        float displacement =  0.0f;

        switch (dir)
        {
          case CSS_DIRECTION_DOWN:
          case CSS_DIRECTION_UP:
            displacement = (fabsf(cand_coord[0]-control_coord[0]) +
                    orthogonalBias) * orthogonalWeight;
            break;
          case CSS_DIRECTION_LEFT:
          case CSS_DIRECTION_RIGHT:
            displacement = (fabsf(cand_coord[1]-control_coord[1]) +
                    orthogonalBias) * orthogonalWeight;
            break;
        }

        float alignBias = overlap / orthogonalSize;
        float alignment = alignWeight * alignBias;

        float euclidian = hypotf (cand_coord[0]-control_coord[0],
                                  cand_coord[1]-control_coord[1]);


        float dist = euclidian  + displacement - alignment - sqrtf(sqrtf(overlap));
        // here we deviate from the algorithm - giving a smaller bonus to overlap
        // to ensure more intermediate small ones miht be chosen
        //float dist = euclidian  + displacement - alignment - sqrtf(overlap);
          if (dist <= best_dist)
          {
             best_dist = dist;
             best = candidate;
          }
        }
      }
    }

    css_set_focus_no (itk, best->no);
  }
}

void css_key_tab (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;
  css_focus (itk, CSS_DIRECTION_NEXT);

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_shift_tab (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;
  css_focus (itk, CSS_DIRECTION_PREVIOUS);

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_return (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;
  if (control->no == itk->focus_no)
  {
    switch (control->type)
    {
      case UI_CHOICE:
       {
          itk->choice_active = !itk->choice_active;
       }
       break;
      case UI_ENTRY:
       {
         if (itk->active)
         {
           css_entry_commit (itk);
         }
         else
         {
           itk->entry_copy = strdup (control->entry_value);
           itk->entry_pos = strlen (itk->entry_copy);
           itk->active = 1;
           itk->active_entry = control->no;
         }
       }
       break;
      case UI_SLIDER:
        if (itk->active)
        {
          itk->active = 0;
        }
        else
        {
          itk->active = 1;
        }
        break;
      case UI_TOGGLE:
          itk->return_value=1;
          break;
      case UI_EXPANDER:
        {
          int *val = control->val;
          *val = !(*val);
        }
        break;
      case UI_RADIO:
      case UI_BUTTON:
        {
          itk->return_value=1;
        }
        break;
    }
  }
  event->stop_propagate=1;
  ctx_queue_draw (event->ctx);
}

void css_key_left (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;

  if (itk->active)
  {
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_CHOICE:
      css_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      itk->entry_pos --;
      if (itk->entry_pos < 0)
        itk->entry_pos = 0;
      break;
    case UI_SLIDER:
      {
        double val = control->value;
        val -= control->step;
        if (val < control->min)
          val = control->min;

        itk->slider_value = control->value = val;
        itk->return_value = 1;
      }
      break;
  }
  }
  else
  {
    css_focus (itk, CSS_DIRECTION_LEFT);
  }

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_right (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;

  if (itk->active)
  {
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_BUTTON:
    case UI_CHOICE:
    case UI_RADIO:
      // css_key_return (event, data, data2);
      break;
    case UI_ENTRY:
     itk->entry_pos ++;
     if (itk->entry_pos > (int)strlen (itk->entry_copy))
       itk->entry_pos = strlen (itk->entry_copy);
      break;
    case UI_SLIDER:
      {
        double val = control->value;
        val += control->step;
        if (val > control->max) val = control->max;

        itk->slider_value = control->value = val;
        itk->return_value = 1;
      }
      break;
  }
  }
  else
  {
    css_focus (itk, CSS_DIRECTION_RIGHT);
  }

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_up (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);

  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    int old_val = itk->choice_no;
    int prev_val = old_val;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
        itk->choice_no = prev_val;
        itk->return_value = 1;
        l=NULL;
      }
      prev_val = choice->val;
    }
  }
  else if (control)
  {
    css_focus (itk, CSS_DIRECTION_UP);
  }
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

void css_key_down (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    {
    int old_val = itk->choice_no;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
         if (l->next)
         {
           l = l->next;
           choice = l->data;
           itk->choice_no = choice->val;
           itk->return_value = 1;
         }
      }
    }
    }
  }
  else if (control)
  {
    css_focus (itk, CSS_DIRECTION_DOWN);
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}


void css_key_backspace (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;
  if (!itk->entry_copy) return;
  if (!itk->active) return;

  switch (control->type)
  {
    case UI_ENTRY:
     {
       if (itk->active && itk->entry_pos > 0)
       {
         memmove (&itk->entry_copy[itk->entry_pos-1], &itk->entry_copy[itk->entry_pos],
                   strlen (&itk->entry_copy[itk->entry_pos] )+ 1);
         itk->entry_pos --;
       }
     }
     break;
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_delete (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  CtxControl *control = css_focused_control (itk);
  if (!control) return;
  if (!itk->entry_copy) return;
  if (!itk->active) return;
  if ((int)strlen (itk->entry_copy) > itk->entry_pos)
  {
    css_key_right (event, data, data2);
    css_key_backspace (event, data, data2);
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_key_unhandled (CtxEvent *event, void *userdata, void *userdata2)
{
  Css *itk = userdata;

  if (itk->active && itk->entry_copy)
    {
      const char *str = event->string;
      if (!strcmp (str, "space"))
        str = " ";

      if (ctx_utf8_strlen (str) == 1)
      {

      char *tmp = malloc (strlen (itk->entry_copy) + strlen (str) + 1);

      char *rest = strdup (&itk->entry_copy[itk->entry_pos]);
      itk->entry_copy[itk->entry_pos]=0;

      sprintf (tmp, "%s%s%s", itk->entry_copy, str, rest);
      free (rest);
      itk->entry_pos+=strlen(str);
      free (itk->entry_copy);
      itk->entry_copy = tmp;
      ctx_queue_draw (event->ctx);
      }
      else
      {
              printf ("unhandled %s\n", str);
      }
    }
  event->stop_propagate = 1;
}

void css_key_bindings (Css *itk)
{
  Ctx *ctx = itk->ctx;
  ctx_add_key_binding (ctx, "tab", NULL, "focus next",            css_key_tab,       itk);
  ctx_add_key_binding (ctx, "shift-tab", NULL, "focus previous",      css_key_shift_tab, itk);

  ctx_add_key_binding (ctx, "up", NULL, "spatial focus up",        css_key_up,    itk);
  ctx_add_key_binding (ctx, "down", NULL, "spatical focus down",   css_key_down,  itk);
  ctx_add_key_binding (ctx, "right", NULL, "spatial focus right",  css_key_right, itk);
  ctx_add_key_binding (ctx, "left", NULL, "spatial focus left",    css_key_left,  itk);

  ctx_add_key_binding (ctx, "return", NULL, "enter/edit", css_key_return,    itk);
  ctx_add_key_binding (ctx, "backspace", NULL, NULL,    css_key_backspace, itk);
  ctx_add_key_binding (ctx, "delete", NULL, NULL,       css_key_delete,    itk);
  ctx_add_key_binding (ctx, "any", NULL, NULL,          css_key_unhandled, itk);
}

#if 0
static void css_choice_set (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  itk->choice_no = (size_t)(data2);
  itk->return_value = 1;
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}
#endif

void ctx_event_block (CtxEvent *event, void *data, void *data2)
{
  Css *itk = data;
  itk->choice_active = 0;
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void css_done (Css *itk)
{
  Ctx *ctx = itk->ctx;

  CtxControl *control = css_focused_control (itk);
#if 0
  CtxControl *hovered_control = css_hovered_control (itk);
  int hovered_no = hovered_control ? hovered_control->no : -1;

  if (itk->hovered_no != hovered_no)
  {
    itk->hovered_no = hovered_no;
    ctx_queue_draw (ctx);
  }
#endif

  if (!control){
    ctx_restore (ctx);
    return;
  }

  ctx_restore (ctx);
}

int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_fb  (Ctx *ctx);

extern int ctx_show_fps;

void
css_ctx_settings (Css *itk)
{
#ifdef CTX_MAX_THREADS
  static int ctx_settings = 0;
  static int inited = 0;
  static int threads;
  //static int hash_cache_enabled;
  Ctx *ctx = itk->ctx;

  if (!inited){
    if (!ctx_backend_is_tiled (ctx))
       return;
    inited = 1;
    threads = ctx_get_render_threads (ctx);
    //hash_cache_enabled = ctx_get_hash_cache (ctx);
  }
  if (css_expander (itk, "CTX settings", &ctx_settings))
  {
#if 0
    hash_cache_enabled = css_toggle (itk, "hash cache", hash_cache_enabled);
    if (hash_cache_enabled != ctx_get_hash_cache (ctx)){
      ctx_set_hash_cache (ctx, hash_cache_enabled);
    }
#endif
    
#if CTX_SDL
    ctx_show_fps = css_toggle (itk, "fps debug", ctx_show_fps);
#endif
    threads = css_slider (itk, "threads", threads, 1, CTX_MAX_THREADS, 1);
    if (threads != ctx_get_render_threads (ctx))
    {
      ctx_set_render_threads (ctx, threads);
    }
  }
#endif
}

void
css_css_settings (Css *itk)
{
   static int css_settings = 0;
   if (css_expander (itk, "Css settings", &css_settings))
   {
     //itk->focus_wraparound = css_toggle (itk, "focus wraparound", itk->focus_wraparound);
     //enable_keybindings = css_toggle (itk, "enable keybindings", enable_keybindings);
     //itk->light_mode = css_toggle (itk, "light mode", itk->light_mode);

     itk->scale     = css_slider (itk, "global scale", itk->scale, 0.1, 8.0, 0.1);
     itk->font_size = css_slider (itk, "font size ", itk->font_size, 3.0, 60.0, 0.25);

     // these will go away with css styling merged.
     css_slider_float (itk, "vgap", &itk->rel_vgap, 0.0, 3.0, 0.02);
     css_slider_float (itk, "scroll speed", &itk->scroll_speed, 0.0, 1.0, 0.01);
     css_slider_float (itk, "ver advance", &itk->rel_ver_advance, 0.1, 4.0, 0.01);
     css_slider_float (itk, "hmargin", &itk->rel_hmargin, 0.0, 40.0, 0.1);
     css_slider_float (itk, "vmargin", &itk->rel_vmargin, 0.0, 40.0, 0.1);
     css_slider_float (itk, "label width", &itk->label_width, 0.0, 40.0, 0.02);
   }
}

void css_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  ctx_exit (event->ctx);
}

int _css_key_bindings_active = 1;

static int
css_iteration (double time, void *data)
{
  Css *itk = (Css*)data;
  Ctx *ctx = itk->ctx;
  int   ret_val = 1;

    if (1 || ctx_need_redraw (ctx))
    {
      css_reset (itk);
      if (_css_key_bindings_active)
        css_key_bindings (itk);
      ctx_add_key_binding (itk->ctx, "control-q", NULL, "Quit", css_key_quit, NULL);
      if (itk->ui_fun)
      ret_val = itk->ui_fun (itk, itk->ui_data);

      css_done (itk);
      ctx_end_frame (ctx);
    }

    return ret_val;
}

#ifdef EMSCRIPTEN
#include <emscripten/html5.h>
#endif

void css_run_ui (Css *itk, int (*ui_fun)(Css *itk, void *data), void *ui_data)
{
  itk->ui_fun  = ui_fun;
  itk->ui_data = ui_data;
  int  ret_val = 1;

#ifdef EMSCRIPTEN
#ifdef ASYNCIFY
  while (!ctx_has_exited (itk->ctx) && (ret_val == 1))
  {
    ret_val = css_iteration (0.0, itk);
  }
#else
  emscripten_request_animation_frame_loop (css_iteration, itk);
  return;
#endif
#else
  while (!ctx_has_exited (itk->ctx) && (ret_val == 1))
  {
    ret_val = css_iteration (0.0, itk);
  }
#endif
}

void css_main (int (*ui_fun)(Css *itk, void *data), void *ui_data)
{
  Ctx *ctx = ctx_new (-1, -1, NULL);
  Css  *itk = css_new (ctx);
  css_run_ui (itk, ui_fun, ui_data);
  css_destroy (itk);
  ctx_destroy (ctx);
}


float       css_x            (Css *itk)
{
  return itk->x;
}
float       css_y            (Css *itk)
{
  return itk->y;
}
void css_set_x  (Css *itk, float x)
{
  itk->x = x;
}
void css_set_y  (Css *itk, float y)
{
  itk->y = y;
}
void css_set_xy  (Css *itk, float x, float y)
{
  itk->x = x;
  itk->y = y;
}


void css_set_height (Css *itk, float height)
{
   itk->height = height;
   itk->edge_bottom = itk->edge_top + height;
   css_set_edge_top ((Css*)itk, itk->edge_top);
   css_set_edge_bottom ((Css*)itk, itk->edge_bottom);
}

float css_edge_bottom (Css *itk)
{
   return itk->edge_bottom ;
}
float css_edge_top (Css *itk)
{
   return itk->edge_top ;
}
float css_edge_left (Css *itk)
{
   return itk->edge_left;
}
float css_height (Css *itk)
{
   return itk->height;
}
float css_wrap_width (Css *itk)
{
    return itk->width; // XXX compute from edges:w
}
void css_set_wrap_width (Css *itk, float wwidth)
{
    itk->width = wwidth;
    itk->edge_right = itk->edge_left + wwidth;
    css_set_edge_right ((Css*)itk, itk->edge_right);
    css_set_edge_left ((Css*)itk, itk->edge_left);
}

float css_edge_right (Css *itk)
{
   return itk->edge_right;
}

Ctx *css_ctx (Css *itk)
{
  return itk->ctx;
}

void css_set_scale (Css *itk, float scale)
{
  itk->scale = scale;
  ctx_queue_draw (itk->ctx);
}

float css_scale (Css *itk)
{
  return itk->scale;
}

int css_focus_no (Css *itk)
{
  return itk->focus_no;
}

int css_is_editing_entry (Css *itk)
{
  return itk->entry_copy != NULL;
}

int  css_control_no (Css *itk)
{
  return itk->control_no;
}

float css_panel_scroll (Css *itk)
{
  CssPanel *panel = itk->panel;
  if (!panel)
    panel = itk->panels?itk->panels->data:NULL;
  if (!panel) return 0.0f;
  return panel->scroll;
}

void css_panel_set_scroll (Css *itk, float scroll)
{
  CssPanel *panel = itk->panel;
  if (!panel)
    panel = itk->panels?itk->panels->data:NULL;
  if (!panel) return;
  if (scroll < 0.0) scroll = 0.0f;
    panel->scroll = scroll;
  ctx_queue_draw (itk->ctx);
}

#endif
