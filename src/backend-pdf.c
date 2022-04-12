#include "ctx-split.h"
#if CTX_PDF

#define CTX_PDF_MAX_OBJS 256

#define CTX_PDF_MAX_PAGES CTX_PDF_MAX_OBJS

typedef struct _CtxPDF CtxPDF;
struct
  _CtxPDF
{
  CtxBackend    backend;
  int           preserve;
  const char   *path;
  char         *font;
  CtxString    *document;
  CtxState      state;
  int           pat;
  int           xref[CTX_PDF_MAX_OBJS];
  int           objs;
  int           page_length_offset;
  int           kids_offset;
  int           page_count_offset;

  int           width;
  int           height;

  int           page_objs[CTX_PDF_MAX_PAGES];
  int           content_objs[CTX_PDF_MAX_PAGES];
  int           page_count;

  int           pages; // known to be 1
};

#define ctx_pdf_printf(fmt, a...) do {\
        ctx_string_append_printf (pdf->document, fmt, ##a);\
}while (0)

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

int pdf_add_object (CtxPDF *pdf)
{
  // we use 1 indexing in this array
  pdf->xref[++pdf->objs] = pdf->document->length;
  ctx_pdf_printf("%i 0 obj\n\n", pdf->objs);
  return pdf->objs;
}
void pdf_end_object (CtxPDF *pdf)
{
  ctx_pdf_printf("endobj\n\n");
}

static void
pdf_start_page (CtxPDF *pdf)
{
  pdf->page_count++;
  pdf->content_objs[pdf->page_count]=pdf_add_object (pdf); // 2 - our actual page contents
  ctx_pdf_printf ("<<\n/Length ");
  pdf->page_length_offset = pdf->document->length;
  ctx_pdf_printf ("XXXXXXXXXX\n>>\n");
  ctx_pdf_printf ("stream\nBT\n1 0 0 -1 0 %i cm\n/F1 24 Tf\n", pdf->height);
}

static void
pdf_end_page (CtxPDF *pdf)
{
  int length = (pdf->document->length - pdf->page_length_offset) - 18;
  char buf[12];
  snprintf (buf, 11, "% 10d", length);
  memcpy   (&pdf->document->str[pdf->page_length_offset], buf, 10);
  ctx_pdf_printf("ET\n");
  ctx_pdf_printf("\nendstream \n");
  pdf_end_object(pdf);
}


static void
ctx_pdf_process (Ctx *ctx, CtxCommand *c)
{
  CtxPDF *pdf = (void*)ctx->backend;
  CtxEntry *entry = (CtxEntry *) &c->entry;
  CtxState *state = &pdf->state;

  CtxDrawlist *preserved = NULL;


  ctx_interpret_style (&pdf->state, entry, NULL);

  switch (entry->code)
    {
      case CTX_NEW_PAGE:
        pdf_end_page (pdf);
        pdf_start_page (pdf);
        break;
      case CTX_LINE_TO:
        ctx_pdf_printf("%f %f l\n", c->line_to.x, c->line_to.y);
        break;
      case CTX_HOR_LINE_TO:
        ctx_pdf_printf("%f %f l\n", ctx_arg_float(0), c->line_to.y);
        break;
      case CTX_VER_LINE_TO:
        ctx_pdf_printf("%f %f l\n", ctx_arg_float(0), c->line_to.y);
        break;
      case CTX_MOVE_TO:
        ctx_pdf_printf("%f %f m\n", c->move_to.x, c->move_to.y);
        break;
      case CTX_CURVE_TO:
        ctx_pdf_printf("%f %f %f %f %f %f c\n",
                        c->curve_to.cx1, c->curve_to.cy1,
                        c->curve_to.cx2, c->curve_to.cy2,
                        c->curve_to.x, c->curve_to.y);
        break;
      case CTX_REL_LINE_TO:
        ctx_pdf_printf("%f %f l\n", c->line_to.x + state->x, c->line_to.y + state->y);
        break;
      case CTX_REL_MOVE_TO:
        ctx_pdf_printf("%f %f m\n", c->move_to.x + state->x, c->move_to.y + state->y);
        break;
      case CTX_REL_CURVE_TO:
        ctx_pdf_printf("%f %f %f %f %f %f c\n",
                        c->curve_to.cx1 + state->x, c->curve_to.cy1 + state->y,
                        c->curve_to.cx2 + state->x, c->curve_to.cy2 + state->y,
                        c->curve_to.x   + state->x, c->curve_to.y   + state->y);
        break;
      case CTX_REL_HOR_LINE_TO:
        ctx_pdf_printf("%f %f l\n", ctx_arg_float(0) + state->x, c->line_to.y + state->y);
        break;
      case CTX_REL_VER_LINE_TO:
        ctx_pdf_printf("%f %f l\n", ctx_arg_float(0) + state->x, c->line_to.y + state->y);
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
          ctx_pdf_printf("%f %f %f %f %f %f c\n",
                             cx1, cy1, cx2, cy2, x, y);
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
          ctx_pdf_printf("%f %f %f %f %f %f c\n",
                             cx1 + state->x, cy1 + state->y,
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
             ctx_pdf_printf("%f %f m\n", x + rx * curves[0][0], y + ry * curves[0][1]);
             for (int i = 0; i < n_curves; i++)
             {
               ctx_pdf_printf("%f %f %f %f %f %f c\n", 
                                 x + rx * curves[i][2], y + ry * curves[i][3],
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
        switch ( ((int) ctx_arg_float (0)) & 511) // XXX remove 511 after stroke source is complete
        {
           case CTX_RGB:
           case CTX_RGBA:
           case CTX_DRGBA:
             ctx_pdf_printf("%f %f %f rg\n", c->rgba.r, c->rgba.g, c->rgba.b);
             ctx_pdf_printf("%f %f %f RG\n", c->rgba.r, c->rgba.g, c->rgba.b);
             break;
           case CTX_CMYKA:
           case CTX_CMYK:
           case CTX_DCMYKA:
           case CTX_DCMYK:
              ctx_pdf_printf("%f %f %f %f k\n", c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k);
              ctx_pdf_printf("%f %f %f %f K\n", c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k);
              break;
           case CTX_GRAYA:
           case CTX_GRAY:
              ctx_pdf_printf("%f g\n", c->graya.g);
              ctx_pdf_printf("%f G\n", c->graya.g);
              break;
            }
        break;
      case CTX_SET_RGBA_U8:
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
      case CTX_ROUND_RECTANGLE: // XXX - arcs
        ctx_pdf_printf("%f %f %f %f re\n",
                         c->rectangle.x, c->rectangle.y,
                         c->rectangle.width, c->rectangle.height);
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
          ctx_pdf_printf("f\n");
          pdf->preserve = 0;
        }
        else
        {
          // XXX have state and check even odd-rule if so use "f*" instead
          ctx_pdf_printf("f\n");
        }
        break;
      case CTX_TRANSLATE: ctx_pdf_printf("1 0 0 1 %f %f cm\n", c->f.a0, c->f.a1); break;
      case CTX_SCALE:     ctx_pdf_printf("%f 0 0 %f 0 0 cm\n", c->f.a0, c->f.a1); break;
      case CTX_ROTATE:    ctx_pdf_printf("%f %f %f %f 0 0 cm\n", 
                             ctx_cosf (-c->f.a0), ctx_sinf (-c->f.a0),
                            -ctx_sinf (-c->f.a0), ctx_cosf (-c->f.a0));break;
      case CTX_APPLY_TRANSFORM:
        ctx_pdf_printf("%f %f %f %f %f %f cm\n",
                   c->f.a0, c->f.a1,
                   c->f.a3, c->f.a4,
                   c->f.a4, c->f.a7);
        break;
      case CTX_STROKE:
        if (pdf->preserve)
        {
          preserved = ctx_current_path (ctx);
          ctx_pdf_printf("S\n");
          pdf->preserve = 0;
        }
        else
        {
          ctx_pdf_printf("S\n");
        }
        break;
      case CTX_CLIP:
        if (pdf->preserve)
        {
          preserved = ctx_current_path (ctx);
          ctx_pdf_printf("W\n");
          pdf->preserve = 0;
        }
        else
        {
          ctx_pdf_printf("W\n");
        }
        break;
        break;
      case CTX_BEGIN_PATH:
        ctx_pdf_printf("n\n");
        break;
      case CTX_CLOSE_PATH:
        ctx_pdf_printf("h\n");
        break;
      case CTX_SAVE:
        ctx_pdf_printf("q\n");
        break;
      case CTX_RESTORE:
        ctx_pdf_printf("Q\n");
        break;
      case CTX_FONT_SIZE:
        //pdf_set_font_size (cr, ctx_arg_float (0) );
        break;
      case CTX_MITER_LIMIT:
        ctx_pdf_printf("%f M\n", ctx_arg_float (0));
        break;
      case CTX_LINE_CAP:
        ctx_pdf_printf("%i J\n", ctx_arg_u8 (0));
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
      case CTX_LINE_JOIN:
        ctx_pdf_printf("%i j\n", ctx_arg_u8 (0));
        break;
      case CTX_LINEAR_GRADIENT:
        {
          ctx_pdf_printf("1 0 0 rg\n");
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          ctx_pdf_printf("0 2 0 rg\n");
        }
        break;
      case CTX_TEXTURE:
      case CTX_DEFINE_TEXTURE:
        {
          ctx_pdf_printf("0 0 1 rg\n");
        }
        break;
      case CTX_GRADIENT_STOP:
        // we set the color so we might get a flavour of the gradient
         ctx_pdf_printf("%f %f %f rg\n", ctx_arg_u8(4)/255.0f,
                                         ctx_arg_u8(4+1)/255.0f,
                                         ctx_arg_u8(4+2)/255.0f);
        break;
      case CTX_TEXT:
        /* XXX: implement some/same linebreaking/wrap, positioning
         *      behavior here?
         *
         *      or fallback to rasterizing as curves if
         *      any non default parameters
         */
        if (state->gstate.font == 0)
          ctx_pdf_printf("/F1 %f Tf\n", state->gstate.font_size);
        else
          ctx_pdf_printf("/F2 %f Tf\n", state->gstate.font_size);
        ctx_pdf_printf("1 0 0 -1 %f %f Tm ( %s ) Tj\n",
                        state->x,
                        state->y,
                        ctx_arg_string ());
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_END_FRAME:
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
    
    //CtxEntry clear_path = {CTX_BEGIN_PATH};
    //ctx_pdf_process (ctx, (CtxCommand*)&clear_path);
    //ctx_update_current_path (ctx, &clear_path);
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

  int outlines=pdf_add_object (pdf); // 1
  ctx_pdf_printf("<<\n/Type /Outlines\n/Count 0\n>>\n");
  pdf_end_object(pdf);
  int catalog=pdf_add_object (pdf); // 2
  ctx_pdf_printf("<<\n/Type /Catalog\n/Outlines %i 0 R\n/Pages %i 0 R\n>>\n", outlines, pdf->pages);
  pdf_end_object(pdf);

  int font1=pdf_add_object (pdf); // 3
  ctx_pdf_printf ("<<\n");
  ctx_pdf_printf (
"/Name /F1\n"
"/Subtype /Type1\n"
"/Type /Font\n"
"/BaseFont /Helvetica\n"
"/Encoding /MacRomanEncoding\n"
">>\n");
  pdf_end_object(pdf);
  int font2=pdf_add_object (pdf); // 4
  ctx_pdf_printf ("<<\n");
  ctx_pdf_printf (
"/Name /F2\n"
"/Subtype /Type1\n"
"/Type /Font\n"
"/BaseFont /Times\n"
"/Encoding /MacRomanEncoding\n"
">>\n");
  pdf_end_object(pdf);
  int fontmap=pdf_add_object(pdf);
  ctx_pdf_printf ("<</F1 %i 0 R /F2 %i 0 R >>\n", font1, font2);
  pdf_end_object(pdf);


for (int page_no =1; page_no <= pdf->page_count; page_no++)
{
  pdf->page_objs[page_no]=pdf_add_object (pdf);
  ctx_pdf_printf ("<<\n"
"/ProcSet [/PDF /Text]\n"
"/Contents %i 0 R\n"
"/Type /Page\n"
"/Resources \n<<\n"
"/Font %i 0 R\n"
">>\n"
"/Parent %i 0 R\n"
"/MediaBox [0 0 %i %i]\n"
">>\n", pdf->content_objs[page_no], fontmap, pdf->pages
, pdf->width, pdf->height);
  pdf_end_object(pdf);

}

  // we now patch-back the value in pages earlier
  snprintf (buf, 11, "% 10d", pdf->page_count);
  memcpy   (&pdf->document->str[pdf->page_count_offset], buf, 10);

  // we now patch-back the value in pages earlier
  int kids = pdf_add_object (pdf); 
  snprintf (buf, 11, "% 10d", kids);
  memcpy   (&pdf->document->str[pdf->kids_offset], buf, 10);

  ctx_pdf_printf ("[");
  for (int page_no =1; page_no <= pdf->page_count; page_no++)
    ctx_pdf_printf ("%i 0 R ", pdf->page_objs[page_no]);
  ctx_pdf_printf ("]");
  pdf_end_object(pdf);

  int start_xref = pdf->document->length + 1;
  ctx_pdf_printf ("xref\n0 %i\n", pdf->objs + 1);
  ctx_pdf_printf ("0000000000 65535 f\n");
        for(int i = 1; i <= pdf->objs; i++)
        {
          ctx_pdf_printf ("%010d 65535 n\n", pdf->xref[i]);
        }
  ctx_pdf_printf ("\ntrailer\n"
"<< /Size %i /Root %i 0 R >>\n"
"startxref\n"
"%d\n"
"%%EOF\n", pdf->objs+1, catalog,
       start_xref);

  fwrite (pdf->document->str, pdf->document->length, 1, f);
  ctx_string_free (pdf->document, 1);
  ctx_free (pdf);
}

Ctx *
ctx_new_pdf (const char *path, int width, int height)
{
  Ctx *ctx = _ctx_new_drawlist (width, height);
  CtxPDF *pdf = ctx_calloc(sizeof(CtxPDF),1);
  CtxBackend *backend  = (CtxBackend*)pdf;
  if (width <= 0) width = 595;
  if (width <= 0) height = 842;


  pdf->width = width;
  pdf->height = height;

  backend->destroy = (void*)ctx_pdf_destroy;
  backend->process = ctx_pdf_process;
  backend->ctx     = ctx;
  pdf->document    = ctx_string_new("");

  pdf->path = ctx_strdup (path);
  ctx_state_init (&pdf->state);
  ctx_set_backend (ctx, (void*)pdf);
  ctx_pdf_printf ("%%PDF-1.4\n%%%c%c%c%c\n", 0xe2, 0xe3, 0xcf, 0xd3);
  pdf->pages=pdf_add_object (pdf); // 1
  ctx_pdf_printf ("<<\n/Kids ");
  pdf->kids_offset = pdf->document->length;
  ctx_pdf_printf ("XXXXXXXXXX 0 R\n");
  ctx_pdf_printf ("/Type /Pages\n");
  ctx_pdf_printf ("/Count ");
  pdf->page_count_offset = pdf->document->length;
  ctx_pdf_printf ("XXXXXXXXXX");
  ctx_pdf_printf ("\n>>\n");
  pdf_end_object(pdf);


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

#endif
