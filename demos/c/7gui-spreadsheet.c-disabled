#include "itk.h"

typedef enum {
  CELL_TYPE_NIL,
  CELL_TYPE_NUMBER,
  CELL_TYPE_FORMULA,
  CELL_TYPE_LABEL
} CellType;

typedef struct _Cell Cell;
struct _Cell {
  char  display[80];
  char  value[80];
  int   dirty;
  CellType type;
  double number;
  Cell *dependencies[30];
  int   dependencies_count;
};

static void cell_formula_compute(Cell *cell);
static void update_cell (Cell *cell)
{
  if (cell->dirty)
  {
    cell->type = CELL_TYPE_NIL;
    if (cell->value[0]==0)
    {
      cell->display[0] = 0;
      cell->dirty = 0;
      cell->number = 0.0;
      cell->type = CELL_TYPE_NIL;
      return;
    }
    int is_number = 1;
    for (int i = 0; cell->value[i]; i++)
    {
      int val = cell->value[i];
      if ( ! ((val >= '0' && val <= '9')  || val == '.'))
        is_number = 0;
    }

    if (is_number)
    {
      cell->number = atof (cell->value); // XXX - locale dependent
      //sprintf (cell->display, "%.2f", cell->number);
      strcpy (cell->display, cell->value);
      cell->type = CELL_TYPE_NUMBER;
    }
    else
    {
      if (cell->value[0]=='=')
      {
        cell_formula_compute (cell);
        cell->type = CELL_TYPE_FORMULA;
      }
      else
      {
        sprintf (cell->display, "%s", cell->value);
        cell->type = CELL_TYPE_LABEL;
      }
    }
    cell->dirty = 0;
  }
}

static int str_is_number (const char *str, double *number)
{
  //int is_number = 1;
  int len = 0;
  if (str[0] == 0) return 0;
  for (int i = 0; str[i]; i++)
  {
    if (!((str[i]>='0' && str[i]<='9') || str[i] == '.'))
    {
      break; 
    }
    len = i + 1;
  }
  if (((str[0]>='0' && str[0]<='9') || str[0] == '.'))
  {
    if (number) *number = atof (str); // XXX locale dependent
    return len;
  }
  return 0;
}

static int str_is_coord (const char *str, int *colp, int *rowp)
{
  int len = 0;
  if (str[0] >= 'A' && str[1] <= 'Z')
  {
    int col = str[0]-'A';
    if (str[1] && str[1] >= '0' && str[1] <= '9')
    {
      int row = 0;
      if (str[2] && str[2] >= '0' && str[2] <= '9')
      {
        row = (str[1] - '0') * 10 + (str[2]-'0');
        len = 3;
      }
      else
      {
        row = (str[1] - '0');
        len = 2;
      }
      if (colp) *colp = col;
      if (rowp) *rowp = row;
      return len;
    }
  }
  return 0;
}

#define SPREADSHEET_COLS 27
#define SPREADSHEET_ROWS 100

static Cell spreadsheet[SPREADSHEET_ROWS][SPREADSHEET_COLS]={0,};
static float col_width[SPREADSHEET_COLS];

static int spreadsheet_first_row = 0;
static int spreadsheet_first_col = 0;

static void cell_formula_compute(Cell *cell)
{
  double arg1  = 0.0;
  int operator = 0;
  double arg2  = 0.0;
  int arg1_col = 0;
  int arg1_row = 0;
  int arg2_col = 0;
  int arg2_row = 0;
  int len      = 0;
  int len2     = 0;

  int rest = 0;
  cell->number = -14;

  if ((len=str_is_coord (cell->value+1, &arg1_col, &arg1_row)))
  {
    if (cell == &spreadsheet[arg1_row][arg1_col])
    {
      sprintf (cell->display, "¡CIRCREF!");
      return;
    }
    update_cell (&spreadsheet[arg1_row][arg1_col]);
    arg1 = spreadsheet[arg1_row][arg1_col].number;
    rest = len + 1;
  }
  else if ((len=str_is_number (cell->value+1, &arg1)))
  {
    rest = len + 1;
  }

  if (rest)(operator = cell->value[rest]);
  if (operator && cell->value[rest+1])
  {
    if ((len2 = str_is_coord (cell->value+rest+1, &arg2_col, &arg2_row)))
    {
      if (cell == &spreadsheet[arg2_row][arg2_col])
      {
        sprintf (cell->display, "¡CIRCREF!");
        return;
      }
      update_cell (&spreadsheet[arg2_row][arg2_col]);
      arg2 = spreadsheet[arg2_row][arg2_col].number;
    }
    else if ((len2 = str_is_number (cell->value+rest+1, &arg2)))
    {
    }
  }

  switch (operator)
  {
    case 0:
            if (cell->value[1]=='s'&&
                cell->value[2]=='u'&&
                cell->value[3]=='m')
            {
              len  = str_is_coord (cell->value+4+1, &arg1_col, &arg1_row);
              len2 = str_is_coord (cell->value+4+1 + len + 1, &arg2_col, &arg2_row);
              if (len && len2)
              {
                double sum = 0.0f;
                for (int v = arg1_row; v <= arg2_row; v++)
                  for (int u = arg1_col; u <= arg2_col; u++)
                  {
                     if (&spreadsheet[v][u] != cell)
                     {
                       update_cell (&spreadsheet[v][u]);
                       sum += spreadsheet[v][u].number;
                     }
                     else
                     {
                       sprintf (cell->display, "¡CIRCREF!");
                       return;
                     }
                  }
                cell->number = sum;
              }
            }
            else
            {
              cell->number = arg1;
            }
            break;
    case '+': cell->number = arg1 + arg2; break;
    case '-': cell->number = arg1 - arg2; break;
    case '*': cell->number = arg1 * arg2; break;
    case '/': cell->number = arg1 / arg2; break;
    default: sprintf(cell->display, "!ERROR"); return;
  }
  sprintf (cell->display, "%.2f", cell->number);
}

static int spreadsheet_col = 0;
static int spreadsheet_row = 0;

static void spreadsheet_keynav (CtxEvent *event, void *data, void *data2)
{
  if (!strcmp (event->string, "up"))
  {
    spreadsheet_row --;
    if (spreadsheet_row < 0) spreadsheet_row = 0;
  }
  else if (!strcmp (event->string, "down"))
  {
    spreadsheet_row ++;
  }
  else if (!strcmp (event->string, "left"))
  {
    spreadsheet_col --;
    if (spreadsheet_col < 0) spreadsheet_col = 0;
  }
  else if (!strcmp (event->string, "right"))
  {
    spreadsheet_col ++;
  }

  event->stop_propagate=1;
  ctx_queue_draw (event->ctx);
}

static void dirty_cell (Cell *cell)
{
  cell->dirty = 1;
  for (int i = 0; i < cell->dependencies_count; i++)
  {
    dirty_cell (cell->dependencies[i]);
  }
}

static void cell_mark_dep (Cell *cell, Cell *dependency)
{
  if (cell != dependency)
  cell->dependencies[cell->dependencies_count++]=dependency;
}

static void cell_unmark_dep (Cell *cell, Cell *dependency)
{
  if (cell != dependency)
  for (int i = 0; i < cell->dependencies_count; i++)
  {
    if (cell->dependencies[i] == dependency)
    {
       cell->dependencies[i] = 
         cell->dependencies[cell->dependencies_count-1];
       cell->dependencies_count--;
       return;
    }
  }
  fprintf (stderr, "tried unmarking nonexisting dep\n");
}

static void formula_update_deps (Cell *cell, const char *formula, int unmark)
{
  for (int i = 0; formula[i]; i++)
  {
    if (formula[i] >= 'A' && formula[i] <= 'Z')
    {
      int col = formula[i] - 'A';
      int row = 0 ;
      if (formula[i+1] && formula[i+1]>='0' && formula[i+1]<='9')
      {
        int n = 0;
        if (formula[i+2] && formula[i+2]>='0' && formula[i+2]<='9')
        {
          row = (formula[i+1] - '0') * 10 +
                (formula[i+2] - '0')
                ;
          n = i + 3;
        }
        else
        {
          row = formula[i+1] - '0';
          n = i + 2;
        }

        if (formula[n]==':')
        {
           int target_row = row;
           int target_col = col;
           n++;

    if (formula[n] >= 'A' && formula[n] <= 'Z')
    {
      target_col = formula[n] - 'A';
      if (formula[n+1] && formula[n+1]>='0' && formula[n+1]<='9')
      {
        if (formula[n+2] && formula[n+2]>='0' && formula[n+2]<='9')
        {
          target_row = (formula[n+1] - '0') * 10 +
                (formula[n+2] - '0')
                ;
        }
        else
        {
          target_row = formula[n+1] - '0';
        }
      }
    }
           for (int v = row; v <= target_row; v++)
           for (int u = col; u <= target_col; u++)
           {

        if (u >= 0 && u <= 26 && v >= 0 && v <= 99)
        {
          if (unmark)
            cell_unmark_dep (&spreadsheet[v][u], cell);
          else
            cell_mark_dep (&spreadsheet[v][u], cell);
        }

           }
        }
        else
        {

        if (col >= 0 && col <= 26 && row >= 0 && row <= 99)
        {
          if (unmark)
            cell_unmark_dep (&spreadsheet[row][col], cell);
          else
            cell_mark_dep (&spreadsheet[row][col], cell);
        }

        }

      }
    }
  }
}

static void cell_set_value (Cell *cell, const char *value)
{
  formula_update_deps (cell, cell->value, 1);
  formula_update_deps (cell, value, 0);
  strcpy (cell->value, value);
  dirty_cell (cell);
}

static int spreadsheet_ui (ITK  *itk, void *data)
{
  Ctx *ctx = itk_ctx (itk);
  float em = itk_em (itk);
  float row_height = em * 1.2;
  static int inited = 0;
  if (!inited)
  {
    int row = 0;
    for (int i = 0; i < SPREADSHEET_COLS; i++)
      col_width[i] = em * 4;
    inited = 1;

    cell_set_value (&spreadsheet[row][0], "5");
    cell_set_value (&spreadsheet[row][1], "3");
    cell_set_value (&spreadsheet[row][2], "4");
    cell_set_value (&spreadsheet[row][3], "6");
    cell_set_value (&spreadsheet[row][4], "7");
    cell_set_value (&spreadsheet[row][5], "8");
    cell_set_value (&spreadsheet[row][6], "9");
    cell_set_value (&spreadsheet[row][7], "1");
    cell_set_value (&spreadsheet[row][8], "2");
    cell_set_value (&spreadsheet[row][9], "=sum(A0:I0)");

    row++;
    cell_set_value (&spreadsheet[row][0], "6");
    cell_set_value (&spreadsheet[row][1], "7");
    cell_set_value (&spreadsheet[row][2], "2");
    cell_set_value (&spreadsheet[row][3], "1");
    cell_set_value (&spreadsheet[row][4], "9");
    cell_set_value (&spreadsheet[row][5], "5");
    cell_set_value (&spreadsheet[row][6], "3");
    cell_set_value (&spreadsheet[row][7], "4");
    cell_set_value (&spreadsheet[row][8], "8");
    cell_set_value (&spreadsheet[row][9], "=sum(A1:I1)");

    row++;
    cell_set_value (&spreadsheet[row][0], "1");
    cell_set_value (&spreadsheet[row][1], "9");
    cell_set_value (&spreadsheet[row][2], "8");
    cell_set_value (&spreadsheet[row][3], "3");
    cell_set_value (&spreadsheet[row][4], "4");
    cell_set_value (&spreadsheet[row][5], "2");
    cell_set_value (&spreadsheet[row][6], "5");
    cell_set_value (&spreadsheet[row][7], "6");
    cell_set_value (&spreadsheet[row][8], "7");
    cell_set_value (&spreadsheet[row][9], "=sum(A2:I2)");

    row++;
    cell_set_value (&spreadsheet[row][0], "8");
    cell_set_value (&spreadsheet[row][1], "5");
    cell_set_value (&spreadsheet[row][2], "9");
    cell_set_value (&spreadsheet[row][3], "7");
    cell_set_value (&spreadsheet[row][4], "6");
    cell_set_value (&spreadsheet[row][5], "1");
    cell_set_value (&spreadsheet[row][6], "4");
    cell_set_value (&spreadsheet[row][7], "2");
    cell_set_value (&spreadsheet[row][8], "3");
    cell_set_value (&spreadsheet[row][9], "=sum(A3:I3)");


    row++;
    cell_set_value (&spreadsheet[row][0], "4");
    cell_set_value (&spreadsheet[row][1], "2");
    cell_set_value (&spreadsheet[row][2], "6");
    cell_set_value (&spreadsheet[row][3], "8");
    cell_set_value (&spreadsheet[row][4], "5");
    cell_set_value (&spreadsheet[row][5], "3");
    cell_set_value (&spreadsheet[row][6], "7");
    cell_set_value (&spreadsheet[row][7], "9");
    cell_set_value (&spreadsheet[row][8], "1");
    cell_set_value (&spreadsheet[row][9], "=sum(A4:I4)");


    row++;
    cell_set_value (&spreadsheet[row][0], "7");
    cell_set_value (&spreadsheet[row][1], "1");
    cell_set_value (&spreadsheet[row][2], "3");
    cell_set_value (&spreadsheet[row][3], "9");
    cell_set_value (&spreadsheet[row][4], "2");
    cell_set_value (&spreadsheet[row][5], "4");
    cell_set_value (&spreadsheet[row][6], "8");
    cell_set_value (&spreadsheet[row][7], "5");
    cell_set_value (&spreadsheet[row][8], "6");
    cell_set_value (&spreadsheet[row][9], "=sum(A5:I5)");

    row++;
    cell_set_value (&spreadsheet[row][0], "9");
    cell_set_value (&spreadsheet[row][1], "6");
    cell_set_value (&spreadsheet[row][2], "1");
    cell_set_value (&spreadsheet[row][3], "5");
    cell_set_value (&spreadsheet[row][4], "3");
    cell_set_value (&spreadsheet[row][5], "7");
    cell_set_value (&spreadsheet[row][6], "2");
    cell_set_value (&spreadsheet[row][7], "8");
    cell_set_value (&spreadsheet[row][8], "4");
    cell_set_value (&spreadsheet[row][9], "=sum(A6:I6)");


    row++;
    cell_set_value (&spreadsheet[row][0], "2");
    cell_set_value (&spreadsheet[row][1], "8");
    cell_set_value (&spreadsheet[row][2], "7");
    cell_set_value (&spreadsheet[row][3], "4");
    cell_set_value (&spreadsheet[row][4], "1");
    cell_set_value (&spreadsheet[row][5], "9");
    cell_set_value (&spreadsheet[row][6], "6");
    cell_set_value (&spreadsheet[row][7], "3");
    cell_set_value (&spreadsheet[row][8], "5");
    cell_set_value (&spreadsheet[row][9], "=sum(A7:I7)");

    row++;
    cell_set_value (&spreadsheet[row][0], "3");
    cell_set_value (&spreadsheet[row][1], "4");
    cell_set_value (&spreadsheet[row][2], "5");
    cell_set_value (&spreadsheet[row][3], "2");
    cell_set_value (&spreadsheet[row][4], "8");
    cell_set_value (&spreadsheet[row][5], "6");
    cell_set_value (&spreadsheet[row][6], "1");
    cell_set_value (&spreadsheet[row][7], "7");
    cell_set_value (&spreadsheet[row][8], "9");
    cell_set_value (&spreadsheet[row][9], "=sum(A8:I8)");

    row=9;
    cell_set_value (&spreadsheet[row][0], "=sum(A0:A8)");
    cell_set_value (&spreadsheet[row][1], "=sum(B0:B8)");
    cell_set_value (&spreadsheet[row][2], "=sum(C0:C8)");
    cell_set_value (&spreadsheet[row][3], "=sum(D0:D8)");
    cell_set_value (&spreadsheet[row][4], "=sum(E0:E8)");
    cell_set_value (&spreadsheet[row][5], "=sum(F0:F8)");
    cell_set_value (&spreadsheet[row][6], "=sum(G0:G8)");
    cell_set_value (&spreadsheet[row][7], "=sum(H0:H8)");
    cell_set_value (&spreadsheet[row][8], "=sum(I0:I8)");

    row=11;
    cell_set_value (&spreadsheet[row][0], "=sum(A0:C2)");
    cell_set_value (&spreadsheet[row][1], "=sum(D0:F2)");
    cell_set_value (&spreadsheet[row][2], "=sum(G0:I2)");
    row++;
    cell_set_value (&spreadsheet[row][0], "=sum(A3:C5)");
    cell_set_value (&spreadsheet[row][1], "=sum(D3:F5)");
    cell_set_value (&spreadsheet[row][2], "=sum(G3:I5)");
    row++;
    cell_set_value (&spreadsheet[row][0], "=sum(A6:C8)");
    cell_set_value (&spreadsheet[row][1], "=sum(D6:F8)");
    cell_set_value (&spreadsheet[row][2], "=sum(G6:I8)");


  }

  itk_panel_start (itk, "7gui - Cells",
                  0, 0,
                  ctx_width (ctx), ctx_height (ctx));

  float saved_x = itk_x (itk);
  float saved_x0 = itk_edge_left (itk);
  float saved_y = itk_y (itk);

  if (!itk_is_editing_entry (itk))
  {
    ctx_add_key_binding (ctx, "left", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "right", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "up", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "down", NULL, "foo", spreadsheet_keynav, NULL);
  }

  /* draw white background */
  ctx_gray (ctx, 1.0);
  ctx_rectangle (ctx, saved_x, saved_y, itk_wrap_width (itk), ctx_height (ctx));
  ctx_fill (ctx);

  float row_header_width = em * 1.5;

  /* draw gray gutters for col/row headers */
  ctx_rectangle (ctx, saved_x, saved_y, itk_wrap_width (itk), row_height);
  ctx_gray (ctx, 0.7);
  ctx_fill (ctx);
  ctx_rectangle (ctx, saved_x, saved_y, row_header_width, ctx_height (ctx));
  ctx_fill (ctx);

  ctx_font_size (ctx, em);
  ctx_gray (ctx, 0.0);

  // ensure current cell is within viewport 
  if (spreadsheet_col < spreadsheet_first_col)
    spreadsheet_first_col = spreadsheet_col;
  else
  {
  float x = saved_x + row_header_width;
  int found = 0;
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    if (col == spreadsheet_col)
    {
       if (x + col_width[col] > itk->panel->x + itk->panel->width)
         spreadsheet_first_col++;
       found = 1;
    }
    x += col_width[col];
  }
  if (!found)
  {
    spreadsheet_first_col++;
    ctx_queue_draw (ctx);
  }
  }

  if (spreadsheet_row < spreadsheet_first_row)
    spreadsheet_first_row = spreadsheet_row;
  else
  {
  float y = saved_y + em;
  int found = 0;
  for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
  {
    if (row == spreadsheet_row)
    {
       if (y + row_height > itk->panel->y + itk->panel->height)
         spreadsheet_first_row++;
       found = 1;
    }
    y += row_height;
  }
  if (!found)
  {
    spreadsheet_first_row++;
    ctx_queue_draw (ctx);
  }
  }

  /* draw col labels */
  float x = saved_x + row_header_width;
  ctx_save (ctx);
  ctx_gray (ctx, 0.1);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + em;
    char label[4]="E";
    ctx_move_to (ctx, x + col_width[col]/2, y);
    label[0]=col+'A';
    ctx_text (ctx, label);
    x += col_width[col];
  }
  ctx_restore (ctx);

  /* draw vertical lines */
  x = saved_x + row_header_width;
  ctx_gray (ctx, 0.5);
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + em;
    ctx_move_to (ctx, floor(x)+0.5, y - row_height);
    ctx_rel_line_to (ctx, 0, itk->panel->height);
    ctx_line_width (ctx, 1.0);
    ctx_stroke (ctx);
    x += col_width[col];
  }

  /* row header labels */
  ctx_save (ctx);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_RIGHT);
  x = saved_x + row_header_width - em * 0.1;
  ctx_gray (ctx, 0.1);
  {
    float y = saved_y + row_height + em;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      char label[10];
      sprintf (label, "%i", row);
      ctx_move_to (ctx, x, y);
      ctx_text (ctx, label);
      y += row_height;
    }
  }
  ctx_restore (ctx);
  ctx_gray (ctx, 0.5);
  x = saved_x;
  /* draw horizontal lines */
  {
    float y = saved_y + row_height;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      ctx_move_to (ctx, x, floor(y)+0.5);
      ctx_rel_line_to (ctx, itk->width, 0);
      ctx_line_width (ctx, 1.0);
      ctx_stroke (ctx);
      y += row_height;
    }
  }

  ctx_gray (ctx, 0.0);
  x = saved_x + row_header_width;
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + row_height + em;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      Cell *cell = &spreadsheet[row][col];
      int drawn = 0;

      if (spreadsheet_col == col && spreadsheet_row == row)
      {
        drawn = 1;
        itk->x = x;
        itk->y = y - em;
        itk->width = col_width[col];

        char *new_value;
        if ((new_value=itk_entry (itk, "", "", cell->value)))
        {
          cell_set_value (cell, new_value);
          free (new_value);
        }
        itk->focus_no = itk->control_no-1;
        if (itk->focus_label){
          free (itk->focus_label);
          itk->focus_label = NULL;
        }

        /* draw cursor around selected cell */
        ctx_gray (ctx, 0);
        ctx_gray_stroke (ctx, 0);
        ctx_rectangle (ctx, x-em*0.1, y - em-em*0.1, col_width[col]+em*0.2, row_height+em*0.2);
        ctx_line_width (ctx, em*0.2);
        ctx_stroke (ctx);
        ctx_gray (ctx, 1);
        ctx_gray_stroke (ctx, 1);
        ctx_rectangle (ctx, x-em*0.1, y - em-em*0.1, col_width[col]+em*0.2, row_height+em*0.2);
        ctx_line_width (ctx, em*0.1);
        ctx_stroke (ctx);
        ctx_gray (ctx, 0);
        ctx_gray_stroke (ctx, 0);
      }
      else
      {

      }
      if (!drawn)
      {
        update_cell (cell);

        if (cell->display[0])
        {
          switch (cell->type)
          {
            case CELL_TYPE_NUMBER:
            case CELL_TYPE_FORMULA:
              ctx_save (ctx);
              ctx_text_align (ctx, CTX_TEXT_ALIGN_RIGHT);
              ctx_move_to (ctx, x + col_width[col] - em * 0.1, y);
              if (cell->display[0]=='!')
              {
                ctx_save (ctx);
                ctx_rgb (ctx, 1,0,0);
                ctx_text (ctx, cell->display);
                ctx_restore (ctx);
              }
              else
              {
                ctx_text (ctx, cell->display);
              }
              ctx_restore (ctx);
              break;
            default:
              ctx_move_to (ctx, x + em * 0.1, y);
              ctx_text (ctx, cell->display);
              break;
          }
        }
      }

      y += row_height;
    }
    x += col_width[col];
  }

  float page_len = 
                 (itk->panel->x+itk->panel->width - saved_x) - row_header_width - em;
  float page_max = 27.0;
  float page_pos = spreadsheet_first_col / page_max;

  ctx_rectangle (ctx, saved_x + row_header_width,
                 saved_y + itk->panel->height - row_height*2,
                 page_len,
                 row_height);
  ctx_rgb (ctx, 0,1,0);
  ctx_fill (ctx);
  ctx_rgb (ctx, 1,0,0);
  float avg_col_width = (col_width[0] + col_width[1] + col_width[2])/3.0;
  ctx_rectangle (ctx,
                 saved_x + row_header_width + page_len * page_pos,
                 saved_y + itk->panel->height - row_height * 2,
                   page_len *
                   (page_len / (avg_col_width * page_max))
                   ,
                 row_height);
  ctx_fill (ctx);




  page_len = itk->panel->height - row_height * 3;
  page_max = 99;
  page_pos = spreadsheet_first_row / page_max;
  ctx_rectangle (ctx, itk->panel->x + itk->panel->width - 
                 em,
                 saved_y + row_height,
                 em, page_len);
  ctx_rgb (ctx, 0,1,0);
  ctx_fill (ctx);

  ctx_rectangle (ctx, itk->panel->x + itk->panel->width - 
                 em,
                 saved_y + row_height + page_len * page_pos,
                 em, 
                   page_len *
                   (page_len / (avg_col_width * page_max)));

  ctx_rgb (ctx, 1,0,0);
  ctx_fill (ctx);


  itk->x  = saved_x;
  itk->edge_left = saved_x0;
  itk->y  = saved_y;

  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main (spreadsheet_ui, NULL);
  return 0;
}
