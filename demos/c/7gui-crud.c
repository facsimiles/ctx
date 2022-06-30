#include "itk.h"
#include <ctype.h>

static char name[20]="";
static char surname[20]="";

#define MAX_NAMES 23

typedef struct _name
{ char name[80];
  char surname[80];
  int  id;
} Name;

static Name name_list[MAX_NAMES]={{"Unknown", "Slaritbartfast", 1},
                                  {"Øyvind", "Kolås", 2}};
static int name_count = 2;

static int name_ids = 2;
static int selected_name = -1;


static void select_name (CtxEvent *event, void *data1, void *data2)
{
  Name *item = data1;
  selected_name = item->id;
  ctx_queue_draw (event->ctx);
  strcpy (name, item->name);
  strcpy (surname, item->surname);
}

static int crud_ui (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  static char filter_prefix[20];

  itk_panel_start (itk, "7gui - CRUD",
                  ctx_width(ctx)*0.2, 0,
                  ctx_width (ctx) * 0.8, ctx_height (ctx));

  itk->width/=2;
  itk_entry_str_len (itk, "Filter prefix:", "", filter_prefix, 20-1);

  int saved_y = itk->y;
  itk->x += itk->width;
  itk->edge_left += itk->width;

  itk_entry_str_len (itk, "Name:", "",    name, 20-1);
  itk_entry_str_len (itk, "Surname:", "", surname, 20-1);
  itk->x -= itk->width;
  itk->edge_left -= itk->width;

  itk->y=saved_y;

  for (int i = 0; i < name_count; i++)
  {
    Name *name = &name_list[i];
    int show = 1;
    if (filter_prefix[0])
    {
      for (int j = 0; filter_prefix[j] && j<20; j++)
      {
        if (tolower(name->surname[j]) != tolower(filter_prefix[j]))
          show = 0;
      }
    }
    if (show)
    {
      /* makes it focusable - and gives us a control handle */
      CtxControl *control = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, itk->width, itk_em(itk) * itk->rel_ver_advance);
      ctx_begin_path (itk->ctx);
      ctx_rectangle (itk->ctx, control->x, control->y, control->width, control->height);
      ctx_listen (itk->ctx, CTX_PRESS, select_name, name, NULL);
      ctx_begin_path (itk->ctx);
      if (control->no == itk->focus_no)
      {
        ctx_add_key_binding (ctx, "right", NULL, "foo", select_name, name);
      }

      if (name->id == selected_name)
      {
        itk_labelf (itk, "[%s, %s]", name->surname, name->name);
      } else
      {
        itk_labelf (itk, "%s, %s", name->surname, name->name);
      }
    }
  }

  if (itk_button (itk, "Create"))
  {
    strcpy (name_list[name_count].name, name);
    strcpy (name_list[name_count].surname, surname);
    name_list[name_count].id = ++name_ids;
    selected_name = name_ids;
    name_count++;
  }
  itk_sameline (itk);
  if (itk_button (itk, "Update"))
  {
    for (int i = 0; i < name_count; i++)
    {
      Name *item = &name_list[i];
      if (selected_name == item->id)
       {
         strcpy (item->name, name);
         strcpy (item->surname, surname);
       }
    }
  }
  itk_sameline (itk);
  if (itk_button (itk, "Delete"))
  {
  }
  itk_sameline (itk);

  itk->width*=2;
  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main ((void*)crud_ui, NULL);
  return 0;
}
