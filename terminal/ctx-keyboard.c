
#if !__COSMOPOLITAN__
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
//#include <pty.h>
#include <math.h>
//#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#endif
#include "ctx.h"

int   on_screen_keyboard = 1;

typedef struct KeyCap {
  char *label;
  char *label_shifted;
  char *label_fn;
  char *label_fn_shifted;
  float wfactor; // 1.0 is regular, tab is 1.5
  char *sequence;
  char *sequence_shifted;
  char *sequence_fn;
  char *sequence_fn_shifted;
  int   sticky;
  int   down;
  int   hovered;
} KeyCap;

typedef struct KeyBoard {
  KeyCap keys[9][30];
  int shifted;
  int control;
  int alt;
  int fn;
  int down;
} KeyBoard;

static void ctx_on_screen_key_event (CtxEvent *event, void *data1, void *data2)
{
  KeyCap *key = data1;
  KeyBoard *kb = data2;
  float h = ctx_height (event->ctx);
  float w = ctx_width (event->ctx);
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;

  if (event->y < y0)
    return;

  key = NULL;

  ctx_event_stop_propagate(event);

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
#if 0
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
#endif
      if (event->x >= x &&
          event->x < x + c * cap->wfactor-0.1 &&
          event->y >= y &&
          event->y < y + c * 0.9)
       {
         key = cap;
         if (cap->hovered != 1)
         {
           ctx_queue_draw (event->ctx);
         }
         cap->hovered = 1;
       }
      else
       {
         cap->hovered = 0;
       }

      x += cap->wfactor * c;
    }
  }

  event->stop_propagate = 1;
  switch (event->type)
  {
     default:
       break;
     case CTX_MOTION:
         ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_MOTION:
       if (!key)
         ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_PRESS:
       kb->down = 1;
       ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_RELEASE:
       kb->down = 0;
        ctx_queue_draw (event->ctx);
       if (!key)
         return;

      if (key->sticky)
      {
        if (key->down)
          key->down = 0;
        else
          key->down = 1;

        if (!strcmp (key->label, "Shift"))
        {
          kb->shifted = key->down;
        }
        else if (!strcmp (key->label, "Ctrl"))
        {
          kb->control = key->down;
        }
        else if (!strcmp (key->label, "Alt"))
        {
          kb->alt = key->down;
        }
        else if (!strcmp (key->label, "Fn"))
        {
          kb->fn = key->down;
        }
      }
      else
      {
        if (kb->control || kb->alt)
        {
          char combined[200]="";
          if (kb->shifted)
          {
            sprintf (&combined[strlen(combined)], "shift-");
          }
          if (kb->control)
          {
            sprintf (&combined[strlen(combined)], "control-");
          }
          if (kb->alt)
          {
            sprintf (&combined[strlen(combined)], "alt-");
          }
          if (kb->fn)
            sprintf (&combined[strlen(combined)], "%s", key->sequence_fn);
          else
            sprintf (&combined[strlen(combined)], "%s", key->sequence);
          ctx_key_press (event->ctx, 0, combined, 0);
        }
        else
        {
          const char *sequence = key->sequence;

          if (kb->fn && kb->shifted && key->sequence_fn_shifted)
          {
            sequence = key->sequence_fn_shifted;
          }
          else if (kb->fn && key->sequence_fn)
          {
            sequence = key->sequence_fn;
          }
          else if (kb->shifted && key->sequence_shifted)
          {
            sequence = key->sequence_shifted;
          }
          ctx_key_press (event->ctx, 0, sequence, 0);
        }
      }
      break;
  }
}

KeyBoard en_intl = {
   {  
     { {"Esc","Esc","`","~",1.0f,"escape","escape","`","~",0},
       {"1","!","F1","F1",1.0f,"1","!","F1","F1",0},
       {"2","@","F2","F2",1.0f,"2","@","F2","F2",0},
       {"3","#","F3","F3",1.0f,"3","#","F3","F3",0},
       {"4","$","F4","F4",1.0f,"4","$","F4","F4",0},
       {"5","%","F5","F5",1.0f,"5","%","F5","F5",0},
       {"6","^","F6","F6",1.0f,"6","^","F6","F6",0},
       {"7","&","F7","F7",1.0f,"7","&","F7","F7",0},
       {"8","*","F8","F8",1.0f,"8","*","F8","F8",0},
       {"9","(","F9","F9",1.0f,"9","(","F9","F9",0},
       {"0",")","F10","F10",1.0f,"0",")","F10","F10",0},
       {"-","_","F11","F11",1.0f,"-","_","F11","F11",0},
       {"=","+","F12","F12",1.0f,"=","+","F12","F12",0},
       {"⌫","⌫",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0},
       {NULL}},
     //⌨
     { {"Tab","Tab",NULL,NULL,1.3f,"tab","tab",NULL,NULL,0},
       {"q","Q",NULL,NULL,1.0f,"q","Q",NULL,NULL,0},
       {"w","W",NULL,NULL,1.0f,"w","W",NULL,NULL,0},
       {"e","E","æ","Æ",  1.0f,"e","E","æ","Æ",0},
       {"r","R",NULL,NULL,1.0f,"r","R",NULL,NULL,0},
       {"t","T",NULL,NULL,1.0f,"t","T",NULL,NULL,0},
       {"y","Y",NULL,NULL,1.0f,"y","Y",NULL,NULL,0},
       {"u","U",NULL,NULL,1.0f,"u","U",NULL,NULL,0},
       {"i","I",NULL,NULL,1.0f,"i","I",NULL,NULL,0},
       {"o","O","ø","Ø",  1.0f,"o","O","ø","Ø",0},
       {"p","P",NULL,NULL,1.0f,"p","P",NULL,NULL,0},
       {"[","{",NULL,NULL,1.0f,"[","{",NULL,NULL,0},
       {"]","}",NULL,NULL,1.0f,"]","}",NULL,NULL,0},
       {"\\","|",NULL,NULL,1.0f,"\\","|",NULL,NULL,0},
       {NULL} },
     { {"Fn","Fn",NULL,NULL,1.5f," "," ",NULL,NULL,1},
       {"a","A","å","Å", 1.0f,"a","A","å","Å",0},
       {"s","S","ß",NULL,1.0f,"s","S","ß",NULL,0},
       {"d","D",NULL,NULL,1.0f,"d","D",NULL,NULL,0},
       {"f","F",NULL,NULL,1.0f,"f","F",NULL,NULL,0},
       {"g","G",NULL,NULL,1.0f,"g","G",NULL,NULL,0},
       {"h","H",NULL,NULL,1.0f,"h","H",NULL,NULL,0},
       {"j","J",NULL,NULL,1.0f,"j","J",NULL,NULL,0},
       {"k","K",NULL,NULL,1.0f,"k","K",NULL,NULL,0},
       {"l","L",NULL,NULL,1.0f,"l","L",NULL,NULL,0},
       {";",":",NULL,NULL,1.0f,";",":",NULL,NULL,0},
       {"'","\"",NULL,NULL,1.0f,"'","\"",NULL,NULL,0},
       {"⏎","⏎",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       {NULL} },
     { {"Ctrl","Ctrl",NULL,NULL,1.7f,"","",NULL,NULL,1},
       {"z","Z",NULL,NULL,1.0f,"z","Z",NULL,NULL,0},
       {"x","X",NULL,NULL,1.0f,"x","X",NULL,NULL,0},
       {"c","C",NULL,NULL,1.0f,"c","C",NULL,NULL,0},
       {"v","V",NULL,NULL,1.0f,"v","V",NULL,NULL,0},
       {"b","B",NULL,NULL,1.0f,"b","B",NULL,NULL,0},
       {"n","N",NULL,NULL,1.0f,"n","N",NULL,NULL,0},
       {"m","M",NULL,NULL,1.0f,"m","M",NULL,NULL,0},
       {",","<",NULL,NULL,1.0f,",","<",NULL,NULL,0},
       {".",">",NULL,NULL,1.0f,".",">",NULL,NULL,0},
       {"/","?",NULL,NULL,1.0f,"/","?",NULL,NULL,0},
       {"↑","↑","PgUp","PgUp",1.0f,"up","up","page-up","page-up",0},
       {NULL} },
     { {"Shift","Shift",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {"Alt","Alt",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {" "," ",NULL,NULL,8.1f,"space","space",NULL,NULL,0},
       {"←","←","Home","Home",1.0f,"left","left","home","home",0},
       {"↓","↓","PgDn","PgDn",1.0f,"down","down","page-down","page-down",0},
       {"→","→","End","End",1.0f,"right","right","end","end",0},
       {NULL} },
     { {NULL}},
   }
};

  // 0.0 = full tilt
  // 0.5 = balanced
  // 1.0 = full saving

void ctx_osk_draw (Ctx *ctx)
{
  if (!on_screen_keyboard)
    return;
  static float fade = 0.0;
  KeyBoard *kb = &en_intl;

  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
     fade = 0.9;
  else {
     fade *= 0.95;
     if (fade < 0.05)
     {
        fade = 0.0;
     }
  }

  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  float m = h;
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;
  if (w < h)
    m = w;
      
  ctx_save (ctx);
  ctx_rectangle (ctx, 0, y0, w, c * rows);
  ctx_listen (ctx, CTX_DRAG, ctx_on_screen_key_event, NULL, &en_intl);
  ctx_rgba (ctx, 0,0,0, 0.6 * fade);
  if (fade < 0.05)
  {
    ctx_restore (ctx);
    ctx_begin_path (ctx);
    return;
  }
  ctx_preserve (ctx);
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
    ctx_fill (ctx);
  //ctx_line_width (ctx, m * 0.01);
  ctx_begin_path (ctx);
#if 0
  ctx_rgba (ctx, 1,1,1, 0.5);
  ctx_stroke (ctx);
#endif

  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_line_width (ctx, m * 0.01);

  float font_size = c * 0.7;
  ctx_font_size (ctx, font_size);

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
  
      const char *label = cap->label;

      if ((kb->fn && kb->shifted && cap->label_fn_shifted))
      {
        label = cap->label_fn_shifted;
      }
      else if (kb->fn && cap->label_fn)
      {
        label = cap->label_fn;
      }
      else if (kb->shifted && cap->label_shifted)
      {
        label = cap->label_shifted;
      }

      if (ctx_utf8_strlen (label) > 1)
      {
        if (font_size != c * 0.33)
        {
          font_size = c * 0.33;
          ctx_font_size (ctx, font_size);
        }
      }
      else
      {
        if (font_size != c * 0.7)
        {
          font_size = c * 0.7;
          ctx_font_size (ctx, font_size);
        }
      }

      ctx_begin_path (ctx);
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
      
      if (cap->down || (cap->hovered && kb->down))
      {
        ctx_rgba (ctx, 1,1,1, fade);
#if 1
      ctx_fill (ctx);
#else
      ctx_preserve (ctx);
      ctx_fill (ctx);

      ctx_rgba (ctx, 0,0,0, fade);

      ctx_stroke (ctx);
#endif
      }
      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 1,1,1, fade);
      else
        ctx_rgba (ctx, 0,0,0, fade);

      ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
      ctx_text_baseline (ctx, CTX_TEXT_BASELINE_MIDDLE);

      ctx_move_to (ctx, x + cap->wfactor * c*0.5, y + c * 0.5);

      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 0,0,0, fade);
      else
        ctx_rgba (ctx, 1,0,0, 0.5f);

      ctx_text (ctx, label);

      x += cap->wfactor * c;
    }
  }
  ctx_restore (ctx);
}
