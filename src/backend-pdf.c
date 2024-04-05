/*
 * TODO:
 *   gradients
 *   text-layout
 *   textures
 *   links
 *
 */


#include "ctx-split.h"
#if CTX_PDF

#define CTX_PDF_MAX_OBJS 256
#define CTX_PDF_MAX_RESOURCES 256 // in one page

#define CTX_PDF_MAX_PAGES CTX_PDF_MAX_OBJS

typedef struct _CtxPDF CtxPDF;
enum { CTX_PDF_TIMES = 1,
       CTX_PDF_HELVETICA, //2
       CTX_PDF_COURIER, //3
       CTX_PDF_SYMBOL, //4
       CTX_PDF_TIMES_BOLD,
       CTX_PDF_HELVETICA_BOLD,
       CTX_PDF_COURIER_BOLD,
       CTX_PDF_ZAPF_DING_BATS, // 8
       CTX_PDF_TIMES_ITALIC, // 9
       CTX_PDF_HELVETICA_ITALIC, // 10
       CTX_PDF_COURIER_ITALIC, // 11
       CTX_PDF_TIMES_BOLD_ITALIC, // 12
       CTX_PDF_HELVETICA_BOLD_ITALIC, //13
       CTX_PDF_COURIER_BOLD_ITALIC, //14
       // courier and helvetica variants are called
       // oblique not italic in the PDF spec

};

typedef struct
_CtxPdfResource
{
  int id;
  int type; // 0 opacity, 1 linear grad, 2 radial grad
  union { 
     struct { float value;}                   opacity;
     struct { float x0, y0, x1, y1;}          linear_gradient;
     struct { float x0, y0, r0, x1, y1, r1;}  radial_gradient;
     struct { const char *eid;int width, height,stride,format;uint8_t *data;}  texture;
     struct { int   no;}    font;
     // texture
     // linear-gradient
     // radial-gradient
  };
} CtxPdfResource;


struct
  _CtxPDF
{
  CtxBackend     backend;
  int            preserve;
  const char    *path;
  CtxString     *document;
  CtxState       state;
  int            pat;
  int            xref[CTX_PDF_MAX_OBJS];
  int            objs;
  int            page_length_offset;
  int            page_height_offset;
  int            kids_offset;
  int            page_count_offset;

  int            width;
  int            height;

  char          *encoding;

  CtxPdfResource resource[CTX_PDF_MAX_RESOURCES];
  int            resource_count;

  int            page_resource[CTX_PDF_MAX_RESOURCES];
  int            page_resource_count;
  int            new_resource[CTX_PDF_MAX_RESOURCES];
  int            new_resource_count;

  int            next_obj; // pre-emptive builds
                           // during page build

  float          page_size[4];

  int            page_objs[CTX_PDF_MAX_PAGES];
  int            content_objs[CTX_PDF_MAX_PAGES];
  int            page_count;

  int            pages; // known to be 1
  int            font;
  int            font_map;


  int alphas[10];
};


#define ctx_pdf_print(str) \
        do { ctx_string_append_str (pdf->document, str);\
}while (0)
#define ctx_pdf_printf(fmt, a...) \
        do { ctx_string_append_printf (pdf->document, fmt, ##a);\
}while (0)
#define ctx_pdf_print1i(i0) \
        do { ctx_string_append_int (pdf->document, i0);\
        ctx_string_append_byte (pdf->document, ' ');\
}while (0)
#define ctx_pdf_print1f(f0) \
        do { ctx_string_append_float (pdf->document, f0);\
             ctx_string_append_byte (pdf->document, ' '); }while (0)
#define ctx_pdf_print2f(f0,f1) \
        do { ctx_pdf_print1f(f0);ctx_pdf_print1f(f1); }while (0)
#define ctx_pdf_print3f(f0,f1,f2) \
        do { ctx_pdf_print2f(f0,f1);ctx_pdf_print1f(f2); }while (0)
#define ctx_pdf_print4f(f0,f1,f2,f3) \
        do { ctx_pdf_print3f(f0,f1,f2);ctx_pdf_print1f(f3); }while (0)
#define ctx_pdf_print5f(f0,f1,f2,f3,f4) \
        do { ctx_pdf_print4f(f0,f1,f2,f3);ctx_pdf_print1f(f4); }while (0)
#define ctx_pdf_print6f(f0,f1,f2,f3,f4,f5) \
        do { ctx_pdf_print5f(f0,f1,f2,f3,f4);ctx_pdf_print1f(f5); }while (0)

/**
 * Generate a cubic Bezier representing an arc on the unit circle of total
 * angle ‘size‘ radians, beginning ‘start‘ radians above the x-axis.
 */
static void acuteArcToBezier(float start, float size,
                float *ax,
                float *ay,
                float *bx,
                float *by,
                float *cx,
                float *cy,
                float *dx,
                float *dy
                ) {
  // Evaluate constants.
  float alpha = size / 2.0,
      cos_alpha = ctx_cosf(alpha),
      sin_alpha = ctx_sinf(alpha),
      cot_alpha = 1.0 / ctx_tanf(alpha),
      phi = start + alpha, // This is how far the arc needs to be rotated.
      cos_phi = ctx_cosf(phi),
      sin_phi = ctx_sinf(phi),
      lambda = (4.0 - cos_alpha) / 3.0,
      mu = sin_alpha + (cos_alpha - lambda) * cot_alpha;
 // Return rotated waypoints.
 *ax = ctx_cosf(start),
 *ay = ctx_sinf(start),
 *bx = lambda * cos_phi + mu * sin_phi,
 *by = lambda * sin_phi - mu * cos_phi,
 *cx = lambda * cos_phi - mu * sin_phi,
 *cy = lambda * sin_phi + mu * cos_phi,
 *dx = ctx_cosf(start + size),
 *dy = ctx_sinf(start + size);
}


static char *ctx_utf8_to_mac_roman (const uint8_t *string);
static char *ctx_utf8_to_windows_1252 (const uint8_t *string);

void pdf_end_object (CtxPDF *pdf)
{
  ctx_pdf_print("\nendobj\n");
}

int pdf_add_object (CtxPDF *pdf)
{
  if (pdf->objs) pdf_end_object (pdf);
  // we use 1 indexing in this array
  pdf->xref[++pdf->objs] = pdf->document->length;
  ctx_pdf_printf("%i 0 obj\n", pdf->objs);
  return pdf->objs;
}

static void
pdf_start_page (CtxPDF *pdf)
{
  pdf->page_count++;
  pdf->content_objs[pdf->page_count]=pdf_add_object (pdf); // 2 - our actual page contents
  ctx_pdf_printf ("<</Length ");
  pdf->page_length_offset = pdf->document->length;
  ctx_pdf_printf ("XXXXXXXXXX>>\n");
  ctx_pdf_printf ("stream\nBT\n1 0 0 -1 0 ");
  pdf->page_height_offset = pdf->document->length;
  ctx_pdf_printf ("XXXXXXXXXX cm\n/F1 24 Tf\n", pdf->height);

  pdf->page_resource_count = 0;
  pdf->new_resource_count = 0;
  pdf->next_obj = pdf->content_objs[pdf->page_count]+1;
}

static void
pdf_end_page (CtxPDF *pdf)
{
  int length = (pdf->document->length - pdf->page_length_offset) - 17;
  char buf[11];
  snprintf (buf, 11, "%10u", length);
  memcpy   (&pdf->document->str[pdf->page_length_offset], buf, 10);
  snprintf (buf, 11, "% 9f", pdf->page_size[3]);
  memcpy   (&pdf->document->str[pdf->page_height_offset], buf, 10);
  ctx_pdf_printf("\nET\nendstream\n");

  for (int i = 0; i < pdf->new_resource_count; i ++)
  {
    float opacity = 1.0f;
    for (int j = 0; j < pdf->resource_count; j ++)
    {
      if (pdf->resource[j].id == pdf->new_resource[i])
        opacity = pdf->resource[j].opacity.value;
    }
    pdf->alphas[i]=pdf_add_object (pdf); // 4
    ctx_pdf_printf ("<</Type/ExtGState/ca %.2f/CA %.2f>>", opacity, opacity);
  }

   pdf->page_objs[pdf->page_count]=pdf_add_object (pdf);
   ctx_pdf_printf ("<<"
"/Contents %i 0 R/Type/Page/Resources<</ProcSet[/PDF/Text]/Font %i 0 R", pdf->content_objs[pdf->page_count], pdf->font_map);
   ctx_pdf_printf ("/ExtGState");

   ctx_pdf_printf ("<<");
   for (int i = 0; i < pdf->page_resource_count; i++)
   {
     ctx_pdf_printf ("/G%i %i 0 R", pdf->page_resource[i],
                                    pdf->page_resource[i]);
   }
   ctx_pdf_print (">>");
   ctx_pdf_print (">>/Parent ");
   ctx_pdf_print1i (pdf->pages);ctx_pdf_print ("0 R");
   ctx_pdf_print ("/MediaBox[");
   ctx_pdf_print4f (pdf->page_size[0], pdf->page_size[1],
                    pdf->page_size[2]+pdf->page_size[0], pdf->page_size[3]+pdf->page_size[1]);
   ctx_pdf_print ("]>>");

}


void ctx_pdf_set_opacity (CtxPDF *pdf, float alpha)
{
  int obj_no = 0;

  for (int i = 0; i < pdf->resource_count; i++)
  {
    if (pdf->resource[i].type == 0 &&
        pdf->resource[i].opacity.value == alpha)
    {
      obj_no = pdf->resource[i].id;
    }
  }

  if (obj_no == 0)
  {
     pdf->resource[pdf->resource_count].type = 0;
     pdf->resource[pdf->resource_count].opacity.value = alpha;
     obj_no = pdf->resource[pdf->resource_count].id = 
             pdf->next_obj++;
     pdf->resource_count++;

     pdf->new_resource[pdf->new_resource_count++] =
       obj_no;
  }

  ctx_pdf_printf("/G%i gs ", obj_no);

  for (int i = 0; i < pdf->page_resource_count; i ++)
  {
    if (pdf->page_resource[i] == obj_no)
      return;
  }
  pdf->page_resource[pdf->page_resource_count++] = obj_no;
}

static void
ctx_pdf_line_to (Ctx *ctx, float x, float y)
{
  CtxPDF *pdf = (void*)ctx->backend;
  ctx_pdf_print2f(x, y); ctx_pdf_print("l ");
}
static void
ctx_pdf_move_to (Ctx *ctx, float x, float y)
{
  CtxPDF *pdf = (void*)ctx->backend;
  ctx_pdf_print2f(x, y); ctx_pdf_print("m ");
}
static void
ctx_pdf_curve_to (Ctx *ctx, float cx0, float cy0, float cx1, float cy1, float x, float y)
{
  CtxPDF *pdf = (void*)ctx->backend;
  ctx_pdf_print6f(cx0,cy0,cx1,cy1,x,y); ctx_pdf_print("c ");
}

static void
ctx_pdf_apply_transform (Ctx *ctx, float a, float b, float c, float d, float e, float f)
{
  CtxPDF *pdf = (void*)ctx->backend;
  ctx_pdf_print6f(a,b,c,d,e,f);
  ctx_pdf_print("cm\n");
}

static void
ctx_pdf_process (Ctx *ctx, const CtxCommand *c)
{
  CtxPDF *pdf = (void*)ctx->backend;
  const CtxEntry *entry = (CtxEntry *) &c->entry;
  CtxState *state = &pdf->state;

  CtxDrawlist *preserved = NULL;

  ctx_interpret_style (&pdf->state, entry, NULL);

  switch (entry->code)
    {
      case CTX_NEW_PAGE:
        pdf_end_page (pdf);
        pdf_start_page (pdf);
        break;

      case CTX_LINE_TO:         ctx_pdf_line_to (ctx, c->line_to.x, c->line_to.y); break;
      case CTX_HOR_LINE_TO:     ctx_pdf_line_to (ctx, ctx_arg_float(0), state->y); break;
      case CTX_VER_LINE_TO:     ctx_pdf_line_to (ctx, state->x, ctx_arg_float(0)); break;
      case CTX_REL_LINE_TO:     ctx_pdf_line_to (ctx, c->line_to.x + state->x, c->line_to.y + state->y); break;
      case CTX_REL_HOR_LINE_TO: ctx_pdf_line_to (ctx, ctx_arg_float(0) + state->x, state->y); break;
      case CTX_REL_VER_LINE_TO: ctx_pdf_line_to (ctx, state->x, ctx_arg_float(0) + state->y); break;

      case CTX_MOVE_TO:         ctx_pdf_move_to (ctx, c->move_to.x, c->move_to.y); break;
      case CTX_REL_MOVE_TO:     ctx_pdf_move_to (ctx, c->move_to.x + state->x, c->move_to.y + state->y); break;

      case CTX_CURVE_TO:
        ctx_pdf_curve_to (ctx, c->curve_to.cx1, c->curve_to.cy1,
                               c->curve_to.cx2, c->curve_to.cy2,
                               c->curve_to.x, c->curve_to.y);
        break;


      case CTX_REL_CURVE_TO:
        ctx_pdf_curve_to (ctx, c->curve_to.cx1 + state->x, c->curve_to.cy1 + state->y,
                               c->curve_to.cx2 + state->x, c->curve_to.cy2 + state->y,
                               c->curve_to.x   + state->x, c->curve_to.y   + state->y);
        break;


      case CTX_PRESERVE:
        pdf->preserve = 1;
        break;

      case CTX_QUAD_TO:
        {
          float cx = ctx_arg_float (0);
          float cy = ctx_arg_float (1);
          float  x = ctx_arg_float (2);
          float  y = ctx_arg_float (3);
          float cx1 = (cx * 2 + state->x) / 3.0f;
          float cy1 = (cy * 2 + state->y) / 3.0f;
          float cx2 = (cx * 2 + x) / 3.0f;
          float cy2 = (cy * 2 + y) / 3.0f;
          ctx_pdf_curve_to (ctx, cx1, cy1, cx2, cy2, x, y);
        }
        break;

      case CTX_REL_QUAD_TO:
        {
          float cx = ctx_arg_float (0);
          float cy = ctx_arg_float (1);
          float  x = ctx_arg_float (2);
          float  y = ctx_arg_float (3);
          float cx1 = (cx * 2 ) / 3.0f;
          float cy1 = (cy * 2 ) / 3.0f;
          float cx2 = (cx * 2 + x) / 3.0f;
          float cy2 = (cy * 2 + y) / 3.0f;
          ctx_pdf_curve_to (ctx, cx1 + state->x, cy1 + state->y,
                                 cx2 + state->x, cy2 + state->y,
                                 x   + state->x, y   + state->y);
        }
        break;

      case CTX_LINE_WIDTH:
        ctx_pdf_printf("%f w\n", ctx_arg_float (0));
        break;

      case CTX_ARC:
        {
           float x = c->arc.x,
                 y = c->arc.y,
                 w = c->arc.radius,
                 h = c->arc.radius,
                 stop  = c->arc.angle1,
                 start = c->arc.angle2;
                 //direction = c->arc.direction;

           start = start * 0.99;

           while (start < 0) start += CTX_PI * 2;
           while (stop < 0) stop += CTX_PI * 2;

           start = ctx_fmodf (start, CTX_PI * 2);
           stop  = ctx_fmodf (stop, CTX_PI * 2);
           // Adjust angles to counter linear scaling.
           if (start <= CTX_PI/2) {
             start = ctx_atanf(w / h * ctx_tanf(start));
           } else if (start > CTX_PI/2 && start <= 3 * CTX_PI/2) {
             start = ctx_atanf(w / h * ctx_tanf(start)) + CTX_PI;
           } else {
             start = ctx_atanf(w / h * ctx_tanf(start)) + CTX_PI*2;
           }
           if (stop <= CTX_PI/2) {
             stop = ctx_atanf(w / h * ctx_tanf(stop));
           } else if (stop > CTX_PI/2 && stop <= 3 * CTX_PI/2) {
             stop = ctx_atanf (w / h * ctx_tanf(stop)) + CTX_PI;
           } else {
             stop = ctx_atanf (w / h * ctx_tanf(stop)) + CTX_PI*2;
           }
           // Exceed the interval if necessary in order to preserve the size and
           // orientation of the arc.
           if (start > stop) {
             stop += CTX_PI * 2;
           }
             // Create curves
             float epsilon = 0.00001f; // Smallest visible angle on displays up to 4K.
             float arcToDraw = 0;
             float curves[4][8]={{0.0f,}};
             int n_curves = 0;
             while(stop - start > epsilon) {
               arcToDraw = ctx_minf(stop - start, CTX_PI/2);
               {
                 //float cx0, cy0, cx1, cy1, cx2, cy2, x, y;
                 acuteArcToBezier(start, arcToDraw, 
                                 &curves[n_curves][0], &curves[n_curves][1],
                                 &curves[n_curves][2], &curves[n_curves][3],
                                 &curves[n_curves][4], &curves[n_curves][5],
                                 &curves[n_curves][6], &curves[n_curves][7]);
                 n_curves++;
               }
             start += arcToDraw;
           }

             float rx = w / 2.0f;
             float ry = h / 2.0f;
             ctx_pdf_print2f(x + rx * curves[0][0], y + ry * curves[0][1]);
             ctx_pdf_print("m\n");
             for (int i = 0; i < n_curves; i++)
             {
               ctx_pdf_curve_to (ctx, x + rx * curves[i][2], y + ry * curves[i][3],
                                  x + rx * curves[i][4], y + ry * curves[i][5],
                                  x + rx * curves[i][6], y + ry * curves[i][7]);
             }
        }
#if 0
        fprintf (stderr, "F %2.1f %2.1f %2.1f %2.1f %2.1f %2.1f\n",
                        ctx_arg_float(0),
                        ctx_arg_float(1),
                        ctx_arg_float(2),
                        ctx_arg_float(3),
                        ctx_arg_float(4),
                        ctx_arg_float(5),
                        ctx_arg_float(6));
        if (ctx_arg_float (5) == 1)
          pdf_arc (cr, ctx_arg_float (0), ctx_arg_float (1),
                     ctx_arg_float (2), ctx_arg_float (3),
                     ctx_arg_float (4) );
        else
          pdf_arc_negative (cr, ctx_arg_float (0), ctx_arg_float (1),
                              ctx_arg_float (2), ctx_arg_float (3),
                              ctx_arg_float (4) );
#endif
        break;

      case CTX_COLOR:
        {
        int space =  ((int) ctx_arg_float (0)) & 511;
        switch (space) // XXX remove 511 after stroke source is complete
        {
           case CTX_RGBA:
           case CTX_DRGBA:
             ctx_pdf_set_opacity (pdf, c->rgba.a);
             /*FALLTHROUGH*/
           case CTX_RGB:
              if (space == CTX_RGB || space == CTX_DRGB)
                ctx_pdf_set_opacity (pdf, 1.0);
             ctx_pdf_print3f(c->rgba.r, c->rgba.g, c->rgba.b);
             ctx_pdf_print("rg ");
             ctx_pdf_print3f(c->rgba.r, c->rgba.g, c->rgba.b);
             ctx_pdf_print("RG\n");
             break;
           case CTX_CMYKA:
           case CTX_DCMYKA:
             ctx_pdf_set_opacity (pdf, c->cmyka.a);
               /*FALLTHROUGH*/
           case CTX_CMYK:
           case CTX_DCMYK:
              if (space == CTX_CMYK || space == CTX_DCMYK)
                ctx_pdf_set_opacity (pdf, 1.0);
              ctx_pdf_print4f(c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k);
              ctx_pdf_print("k ");
              ctx_pdf_print4f(c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k);
              ctx_pdf_print("K ");
              break;
           case CTX_GRAYA:
             ctx_pdf_set_opacity (pdf, c->graya.a);
               /*FALLTHROUGH*/
           case CTX_GRAY:
              if (space == CTX_GRAY)
                ctx_pdf_set_opacity (pdf, 1.0);
              ctx_pdf_print1f(c->graya.g);
              ctx_pdf_print("g ");
              ctx_pdf_print1f(c->graya.g);
              ctx_pdf_print("G\n");
              break;
            }
        }
        break;

      case CTX_SET_RGBA_U8:
        ctx_pdf_printf("/G%i gs\n", ctx_arg_u8(3)*10/255);
        ctx_pdf_printf("%f %f %f RG\n",
                               ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ));
        ctx_pdf_printf("%f %f %f rg\n",
                               ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ));
        break;

      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE:
        ctx_pdf_print4f(c->rectangle.x, c->rectangle.y,
                        c->rectangle.width, c->rectangle.height);
        ctx_pdf_print("re\n");
        break;

      case CTX_SET_PIXEL:
#if 0
        pdf_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        pdf_rectangle (cr, ctx_arg_u16 (2), ctx_arg_u16 (3), 1, 1);
        pdf_fill (cr);
#endif
        break;

      case CTX_FILL:
        if (pdf->preserve)
        {
          preserved = ctx_current_path (ctx);
          pdf->preserve = 0;
        }
        ctx_pdf_print ("f\n");
        break;

      case CTX_TRANSLATE:
         ctx_pdf_apply_transform (ctx, 1.f, 0.f, 0.f, 1.f, c->f.a0, c->f.a1); 
         break;
      case CTX_SCALE:     
         ctx_pdf_apply_transform (ctx, c->f.a0, 0.f, 0.f, c->f.a1, 0.f, 0.f); 
         break;
      case CTX_ROTATE:
         ctx_pdf_apply_transform (ctx,
             ctx_cosf (-c->f.a0), ctx_sinf (-c->f.a0),
             -ctx_sinf (-c->f.a0), ctx_cosf (-c->f.a0),
             0.f, 0.f);
         break;

      case CTX_APPLY_TRANSFORM:
        ctx_pdf_apply_transform (ctx, c->f.a0, c->f.a1,
                                      c->f.a3, c->f.a4,
                                      c->f.a4, c->f.a7);
        break;

      case CTX_STROKE:
        if (pdf->preserve)
        {
          preserved = ctx_current_path (ctx);
          ctx_pdf_print("S\n");
          pdf->preserve = 0;
        }
        else
        {
          ctx_pdf_print("S\n");
        }
        break;

      case CTX_CLIP:
        if (pdf->preserve)
        {
          preserved = ctx_current_path (ctx);
          ctx_pdf_print("W\n");
          pdf->preserve = 0;
        }
        else
        {
          ctx_pdf_print("W\n");
        }
        break;
      case CTX_BEGIN_PATH:  ctx_pdf_print("n\n"); break;
      case CTX_CLOSE_PATH:  ctx_pdf_print("h\n"); break;
      case CTX_SAVE:        ctx_pdf_print("q\n"); break;
      case CTX_RESTORE:     ctx_pdf_print("Q\n"); break;
      case CTX_FONT_SIZE:   ctx_pdf_printf("/F%i %f Tf\n", pdf->font, state->gstate.font_size); break;
      case CTX_MITER_LIMIT: ctx_pdf_printf("%f M\n", ctx_arg_float (0)); break;
      case CTX_LINE_CAP:    ctx_pdf_printf("%i J\n", ctx_arg_u8 (0)); break;
      case CTX_LINE_JOIN:   ctx_pdf_printf("%i j\n", ctx_arg_u8 (0)); break;

      case CTX_FONT:
        {
          const char *str = ctx_arg_string ();
          if (!strcmp (str, "Helvetica"))             pdf->font = CTX_PDF_HELVETICA;
          if (!strcmp (str, "Helvetica Bold"))        pdf->font = CTX_PDF_HELVETICA_BOLD;
          if (!strcmp (str, "Helvetica Italic"))      pdf->font = CTX_PDF_HELVETICA_ITALIC;
          if (!strcmp (str, "Helvetica BoldItalic"))  pdf->font = CTX_PDF_HELVETICA_BOLD_ITALIC;
          if (!strcmp (str, "Helvetica Bold Italic")) pdf->font = CTX_PDF_HELVETICA_BOLD_ITALIC;
          if (!strcmp (str, "Symbol"))                pdf->font = CTX_PDF_SYMBOL;
          if (!strcmp (str, "Zapf Dingbats"))         pdf->font = CTX_PDF_ZAPF_DING_BATS;
          if (!strcmp (str, "ZapfDingbats"))          pdf->font = CTX_PDF_ZAPF_DING_BATS;
          if (!strcmp (str, "Times"))                 pdf->font = CTX_PDF_TIMES;
          if (!strcmp (str, "Times Italic"))          pdf->font = CTX_PDF_TIMES_ITALIC;
          if (!strcmp (str, "Times Bold"))            pdf->font = CTX_PDF_TIMES_BOLD;
          if (!strcmp (str, "Times Bold Italic"))     pdf->font = CTX_PDF_TIMES_BOLD_ITALIC;
          if (!strcmp (str, "Times BoldItalic"))      pdf->font = CTX_PDF_TIMES_BOLD_ITALIC;
          if (!strcmp (str, "Courier"))               pdf->font = CTX_PDF_COURIER;
          if (!strcmp (str, "Courier Bold"))          pdf->font = CTX_PDF_COURIER_BOLD;
          if (!strcmp (str, "Courier Italic"))        pdf->font = CTX_PDF_COURIER_ITALIC;
          if (!strcmp (str, "Courier Bold Italic"))   pdf->font = CTX_PDF_COURIER_BOLD_ITALIC;
          if (!strcmp (str, "Courier BoldItalic"))    pdf->font = CTX_PDF_COURIER_BOLD_ITALIC;
        }
        ctx_pdf_printf("/F%i %f Tf\n", pdf->font, state->gstate.font_size);
        break;


#if 0
      case CTX_BLEND_MODE:
        {
        }
        break;
      case CTX_COMPOSITING_MODE:
        {
          int pdf_val = CAIRO_OPERATOR_OVER;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_COMPOSITE_SOURCE_OVER:
                pdf_val = CAIRO_OPERATOR_OVER;
                break;
              case CTX_COMPOSITE_COPY:
                pdf_val = CAIRO_OPERATOR_SOURCE;
                break;
            }
          pdf_set_operator (cr, pdf_val);
        }
        break;
#endif
      case CTX_LINEAR_GRADIENT: { ctx_pdf_print("1 0 0 rg\n"); } break;
      case CTX_RADIAL_GRADIENT: { ctx_pdf_print("0 2 0 rg\n"); } break;
      case CTX_TEXTURE:
      case CTX_DEFINE_TEXTURE: { ctx_pdf_print("0 0 1 rg\n"); } break;
      case CTX_GRADIENT_STOP:
        // we set the color so we might get a flavour of the gradient
         ctx_pdf_printf("%f %f %f rg\n", ctx_arg_u8(4)/255.0f,
                                         ctx_arg_u8(4+1)/255.0f,
                                         ctx_arg_u8(4+2)/255.0f);
        break;
      case CTX_TEXT:
        ctx_pdf_print("1 0 0 -1 ");
        ctx_pdf_print2f(state->x, state->y);
        ctx_pdf_print("Tm ");
        if (0)
        {
          char *encoded = ctx_utf8_to_mac_roman ((uint8_t*)ctx_arg_string ());
          ctx_pdf_printf ("(%s) Tj\n", encoded);
          ctx_free (encoded);
        }
        else
        {
          char *encoded = ctx_utf8_to_windows_1252 ((uint8_t*)ctx_arg_string ());
          ctx_pdf_printf ("(%s) Tj\n", encoded);
          ctx_free (encoded);
        }
        break;
      case CTX_CONT:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_END_FRAME:
        break;
      case CTX_VIEW_BOX:
        pdf->page_size[0] = ctx_arg_float(0);
        pdf->page_size[1] = ctx_arg_float(1);
        pdf->page_size[2] = ctx_arg_float(2);
        pdf->page_size[3] = ctx_arg_float(3);
        ctx_set_size (ctx, 
          ctx_arg_float(2),
          ctx_arg_float(3));
        break;
    }
  ctx_interpret_pos_bare (&pdf->state, entry, pdf);
#if CTX_CURRENT_PATH
  ctx_update_current_path (ctx, entry);
#endif

  if (preserved)
  {
    CtxIterator iterator;
    CtxCommand *command;
    
    ctx_iterator_init (&iterator, preserved, 0, CTX_ITERATOR_EXPAND_BITPACK);
    while ( (command = ctx_iterator_next (&iterator) ) )
      { ctx_pdf_process (ctx, command); }
    ctx_free (preserved);
  }
}

void ctx_pdf_destroy (CtxPDF *pdf)
{
  FILE *f = fopen (pdf->path, "w");
  char buf[12];

  pdf_end_page (pdf);

  int outlines=pdf_add_object (pdf);
  ctx_pdf_print("<</Type/Outlines/Count 0>>");
  int catalog=pdf_add_object (pdf);
  ctx_pdf_printf("<</Type/Catalog/Outlines %i 0 R/Pages %i 0 R>>", outlines, pdf->pages);


  // patch-back the value in pages earlier
  snprintf (buf, 11, "% 10d", pdf->page_count);
  memcpy   (&pdf->document->str[pdf->page_count_offset], buf, 10);

  // patch-back the value in pages earlier
  int kids = pdf_add_object (pdf); 
  snprintf (buf, 11, "% 10d", kids);
  memcpy   (&pdf->document->str[pdf->kids_offset], buf, 10);

  ctx_pdf_print ("[");
  for (int page_no =1; page_no <= pdf->page_count; page_no++)
    ctx_pdf_printf ("%i 0 R ", pdf->page_objs[page_no]);
  ctx_pdf_print ("]");
  pdf_end_object(pdf);

  int start_xref = pdf->document->length;
  ctx_pdf_printf ("xref\n0 %i\n", pdf->objs + 1);
  ctx_pdf_print ("0000000000 65535 f\n");
        for(int i = 1; i <= pdf->objs; i++)
        {
          ctx_pdf_printf ("%010d 65535 n\n", pdf->xref[i]);
        }
  ctx_pdf_printf ("trailer\n\n"
"<</Root %i 0 R\n/Size %i>>\n"
"startxref\n"
"%d\n"
"%%%%EOF\n", catalog, pdf->objs+1,
       start_xref);

  fwrite (pdf->document->str, pdf->document->length, 1, f);
  ctx_string_free (pdf->document, 1);
  ctx_free (pdf);
}

Ctx *
ctx_new_pdf (const char *path, float width, float height)
{
  Ctx *ctx = _ctx_new_drawlist (width, height);
  CtxPDF *pdf = ctx_calloc(sizeof(CtxPDF),1);
  CtxBackend *backend  = (CtxBackend*)pdf;
  if (width <= 0) width = 595;
  if (width <= 0) height = 842;

  pdf->width = width;
  pdf->height = height;

  backend->type = CTX_BACKEND_PDF;
  backend->destroy = (void*)ctx_pdf_destroy;
  backend->process = ctx_pdf_process;
  backend->ctx     = ctx;
  pdf->document    = ctx_string_new("");

  pdf->path = ctx_strdup (path);
  ctx_state_init (&pdf->state);
  ctx_set_backend (ctx, (void*)pdf);
  ctx_pdf_print ("%PDF-1.4\n%ÆØÅ\n");
  //ctx_pdf_printf ("%%PDF-1.4\n%%%c%c%c%c\n", 0xe2, 0xe3, 0xcf, 0xd3);
  pdf->pages=pdf_add_object (pdf); // 1
  pdf->font = CTX_PDF_HELVETICA;
  //pdf->encoding = "/MacRomanEncoding";
  pdf->encoding = "/WinAnsiEncoding";

  ctx_pdf_print ("<</Kids ");
  pdf->kids_offset = pdf->document->length;
  ctx_pdf_print ("XXXXXXXXXX 0 R/Type/Pages/Count ");
  pdf->page_count_offset = pdf->document->length;
  ctx_pdf_print ("XXXXXXXXXX");
  ctx_pdf_print (">>");

  { // shared fontmap for all pages
    // good enough without TTF fonts
    int font[16];

    char *font_names[]={"","Times","Helvetica","Courier","Symbol",
"Times-Bold", "Helvetica-Bold", "Courier-Bold",
"ZapfDingbats", "Times-Italic", "Helvetica-Oblique",
"Courier-Oblique", "Times-BoldItalic", "Helvetica-BoldItalic", "Courier-BoldItalic"
    };

    for (int font_no = 1; font_no <= 14; font_no++)
    {
      font[font_no]= pdf_add_object (pdf);
      ctx_pdf_printf ("<</Name/F%i/Subtype/Type1/Type/Font/BaseFont /%s /Encoding %s>>",
                      font_no, font_names[font_no], pdf->encoding);
    }

    pdf->font_map=pdf_add_object(pdf);
    ctx_pdf_print ("<<");
    for (int font_no = 1; font_no <= 14; font_no++)
      ctx_pdf_printf ("/F%i %i 0 R", font_no, font[font_no]);
    ctx_pdf_print (">>");
  }

  pdf->page_size[0] = 0;
  pdf->page_size[1] = 0;
  pdf->page_size[2] = pdf->width;
  pdf->page_size[3] = pdf->height;

  pdf_start_page (pdf);

  return ctx;
}

void
ctx_render_pdf (Ctx *ctx, const char *path)
{
  Ctx *pdf = ctx_new_pdf (path, 0, 0);
  CtxIterator iterator;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0, CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_pdf_process (pdf, command); }
  ctx_destroy (pdf);
}




static char *ctx_utf8_to_mac_roman (const uint8_t *string)
{
  CtxString *ret = ctx_string_new ("");
  if (*string)
  for (const uint8_t *utf8 = (uint8_t*)string; utf8[0]; utf8 = *utf8?(uint8_t*)ctx_utf8_skip ((char*)utf8, 1):(utf8+1))
  {
    uint8_t copy[5];

    memcpy (copy, utf8, ctx_utf8_len (utf8[0]));
    copy[ctx_utf8_len (utf8[0])]=0;
    if (copy[0] <=127)
    {
      ctx_string_append_byte (ret, copy[0]);
    }
    else
    {
      int code = 128;
      /* it would be better to to this comparison on a unicode table,
       * but this was easier to create
       */
#define C(a) \
      if (!strcmp ((char*)&copy[0], a)) { ctx_string_append_byte (ret, code); continue; }; code++
      C("Ä");C("Å");C("Ç");C("É");C("Ñ");C("Ö");C("Ü");C("á");C("à");C("â");C("ä");C("ã");C("å");C("ç");C("é");C("è");
      C("ê");C("ë");C("í");C("ì");C("î");C("ï");C("ñ");C("ó");C("ò");C("ô");C("ö");C("õ");C("ú");C("ù");C("û");C("ü");
      C("†");C("°");C("¢");C("£");C("§");C("•");C("¶");C("ß");C("®");C("©");C("™");C("´");C("¨");C("≠");C("Æ");C("Ø");
      C("∞");C("±");C("≤");C("≥");C("¥");C("µ");C("∂");C("∑");C("∏");C("π");C("∫");C("ª");C("º");C("Ω");C("æ");C("ø");
      C("¿");C("¡");C("¬");C("√");C("ƒ");C("≈");C("∆");C("«");C("»");C("…");C(" ");C("À");C("Ã");C("Õ");C("Œ");C("œ");
      C("–");C("—");C("“");C("”");C("‘");C("’");C("÷");C("◊");C("ÿ");C("Ÿ");C("⁄");C("€");C("‹");C("›");C("ﬁ");C("ﬂ");
      C("‡");C("·");C("‚");C("„");C("‰");C("Â");C("Ê");C("Á");C("Ë");C("È");C("Í");C("Î");C("Ï");C("Ì");C("Ó");C("Ô");
      C("?");C("Ò");C("Ú");C("Û");C("Ù");C("ı");C("ˆ");C("˜");C("¯");C("˘");C("˙");C("˚");C("¸");C("˝");C("˛");C("ˇ");
      ctx_string_append_byte (ret, '?');
    }
  }

  return ctx_string_dissolve (ret);
}

static char *ctx_utf8_to_windows_1252 (const uint8_t *string)
{
  CtxString *ret = ctx_string_new ("");
  if (*string)
  for (const uint8_t *utf8 = (uint8_t*)string; utf8[0]; utf8 = *utf8?(uint8_t*)ctx_utf8_skip ((char*)utf8, 1):(utf8+1))
  {
    uint8_t copy[5];

    memcpy (copy, utf8, ctx_utf8_len (utf8[0]));
    copy[ctx_utf8_len (utf8[0])]=0;
    if (copy[0] == '(' || copy[0] == ')')
    {
      ctx_string_append_byte (ret, '\\');
      ctx_string_append_byte (ret, copy[0]);
    }
    else if (copy[0] <=127)
    {
      ctx_string_append_byte (ret, copy[0]);
    }
    else
    {
      int code = 128;
      /* it would be better to to this comparison on a unicode table,
       * but this was easier to create
       */
C("€");C(" ");C("‚");C("ƒ");C("„");C("…");C("†");C("‡");C("ˆ");C("‰");C("Š");C("‹");C("Œ");C(" ");C("Ž");C(" ");
C(" ");C("‘");C("’");C("“");C("”");C("•");C("–");C("—");C("˜");C("™");C("š");C("›");C("œ");C(" ");C("ž");C("Ÿ");
C(" ");C("¡");C("¢");C("£");C("¤");C("¥");C("¦");C("§");C("¨");C("©");C("ª");C("«");C("¬");C("-");C("®");C("¯");
C("°");C("±");C("²");C("³");C("´");C("µ");C("¶");C("·");C("¸");C("¹");C("º");C("»");C("¼");C("½");C("¾");C("¿");
C("À");C("Á");C("Â");C("Ã");C("Ä");C("Å");C("Æ");C("Ç");C("È");C("É");C("Ê");C("Ë");C("Ì");C("Í");C("Î");C("Ï");
C("Ð");C("Ñ");C("Ò");C("Ó");C("Ô");C("Õ");C("Ö");C("×");C("Ø");C("Ù");C("Ú");C("Û");C("Ü");C("Ý");C("Þ");C("ß");
C("à");C("á");C("â");C("ã");C("ä");C("å");C("æ");C("ç");C("è");C("é");C("ê");C("ë");C("ì");C("í");C("î");C("ï");
C("ð");C("ñ");C("ò");C("ó");C("ô");C("õ");C("ö");C("÷");C("ø");C("ù");C("ú");C("û");C("ü");C("ý");C("þ");C("ÿ");
#undef C
      ctx_string_append_byte (ret, '?');
    }
  }
  return ctx_string_dissolve (ret);
}



#endif
