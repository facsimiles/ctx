
#ifdef EMSCRIPTEN
#include "emscripten.h"

#include <unistd.h>


int width = 512;
int height = 384;

static uint8_t *fb = NULL;

CTX_EXPORT unsigned char *
get_fb(int w, int h) {
  width  = w;
  height = h;
  if (!fb)
  {
    fb = malloc (w *h * 4);
  }
  return fb;
}


void  update_fb (Ctx *ctx, void *user_data)
{
  EM_ASM(
    var canvas = document.getElementById('c');
    var context = canvas.getContext('2d');
    const offset = _get_fb(canvas.width, canvas.height);
    var _ctx = _get_context();
    const imgData = context.createImageData(canvas.width,canvas.height);
                  //console.log(offset);
    var x0 = _ctx_cb_x0 (_ctx);
    var y0 = _ctx_cb_y0 (_ctx);
    var x1 = _ctx_cb_x1 (_ctx);
    var y1 = _ctx_cb_y1 (_ctx);

    var updatew = x1 - x0 + 1;
    var updateh = y1 - y0 + 1;
    const linearMem = new Uint8Array(wasmMemory.buffer, offset,
                          canvas.width * canvas.height * 4);

    for (let y = y0; y < y1; y++)
    {
      let dsto = y * canvas.width * 4 + x0;
      let srco = y * updatew * 4;
      for (let x = x0; x < x1; x++)
      {
        var a = linearMem[srco+3];
        var r = 1.0;
        if (a!=0) r = 255.0/a;
        imgData.data[dsto+0] = linearMem[srco+0] * r;
        imgData.data[dsto+1] = linearMem[srco+1] * r;
        imgData.data[dsto+2] = linearMem[srco+2] * r;
        imgData.data[dsto+3] = a;
        srco+=4;
        dsto+=4;
      }
    }

    for (let i = 0; i < canvas.width * canvas.height;i++)
    {
      var a = linearMem[i*4+3];
      var r = 1.0;
      // if (a!=0) r = 255.0/a;
      imgData.data[i*4+0] = linearMem[i*4+0] * r;
      imgData.data[i*4+1] = linearMem[i*4+1] * r;
      imgData.data[i*4+2] = linearMem[i*4+2] * r;
      imgData.data[i*4+3] = 255;
    }
    context.putImageData(imgData,0,0);


     if (!canvas.regevents)
     {

       canvas.addEventListener('mousedown', function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_press (_ctx, loc.x, loc.y, 0, 0);
                       }
       );
       canvas.addEventListener('mouseup', function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_release (_ctx, loc.x, loc.y, 0, 0);
                       });
       canvas.addEventListener('mousemove', function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_motion (_ctx, loc.x, loc.y, 0, 0);
                       });
       canvas.addEventListener('keydown', function (e){
                       _ctx_key_down(_ctx,e.keyCode,0,0);
                       _ctx_key_press(_ctx,e.keyCode,0,0);
                       // XXX : todo, pass some tings like ctrl+l and ctrl+r
                       //       through?
                       e.preventDefault();
                       e.stopPropagate = 1;
                       });

       canvas.addEventListener('keyup', function (e){
                       _ctx_key_up(_ctx,e.keyCode,0,0);
                       e.preventDefault();
                       e.stopPropagate = 1;
                       });
       canvas.regevents = true;
     }


  );

   

#ifdef EMSCRIPTEN
#ifdef ASYNCIFY
   emscripten_sleep(0);
#endif
#endif
}

static void set_pixels (Ctx *ctx, void *user_data, int x0, int y0, int w, int h, void *buf)
{
  //TFT_eSPI *tft = (TFT_eSPI*)user_data;
  //tft->pushRect (x, y, w, h, (uint16_t*)buf);
  int stride = ctx_width (ctx) * 4;
  uint8_t *src = (uint8_t*)buf;
  uint8_t *dst = fb + y0 * stride + x0 * 4;
  for (int y = y0; y < y0 + h; y++)
  {
    memcpy (dst, src, w * 4);
    dst += stride;
    src += w * 4;
  }
}

Ctx *get_context (void)
{
   static Ctx *ctx = NULL;

EM_ASM(
    {var canvas = document.getElementById('c');
     const offset = _get_fb(canvas.width, canvas.height);
     
     }
);

   if (!ctx){ctx = ctx_new_cb (width, height, CTX_FORMAT_RGBA8, set_pixels, 
                               update_fb,
                               fb,
                               width * height * 4, NULL, 
                               CTX_CB_DEFAULTS|CTX_CB_HASH_CACHE);
   }
   return ctx;
}

#endif
