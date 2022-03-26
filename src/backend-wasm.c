
#ifdef EMSCRIPTEN
#include "emscripten.h"

#include <unistd.h>

int width = 512;
int height = 384;

static uint8_t *fb = NULL;
static Ctx *em_ctx = NULL;

CTX_EXPORT unsigned char *
get_fb(int w, int h) {
  if (fb)
  {
    if (width == w && height == h) return fb;
    free (fb); // this is not using the ctx allocator
               // and will thus not be part of the micropython heap budget
    fb = NULL;
  }
  width  = w;
  height = h;
  fb = calloc (w *h, 4);
  if (em_ctx) ctx_destroy (em_ctx);
  em_ctx = NULL;
  return fb;
}

int update_fb (Ctx *ctx, void *user_data)
{
  EM_ASM(
    var canvas = document.getElementById('c');
    var context = canvas.getContext('2d');
    var _ctx = _ctx_wasm_get_context(0); // we presume an earlier

     if (!canvas.regevents)
     {
       canvas.onmousedown = function (e){
          var sync = 0;
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_press (_ctx, loc.x, loc.y, 0, 0, sync);
          e.stopPropagate=1;
                       };
       canvas.onmouseup = function (e){
          var sync = 0;
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_release (_ctx, loc.x, loc.y, 0, 0, sync);
          e.stopPropagate=1;
                       };
       canvas.onmousemove = function (e){
          var sync = 0;
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          _ctx_pointer_motion (_ctx, loc.x, loc.y, 0, 0, sync);
          e.stopPropagate=1;
                       };
       canvas.onkeydown = function (e){
          var sync = 0;
                       _ctx_key_down(_ctx,e.keyCode,0,0, sync);
                       _ctx_key_press(_ctx,e.keyCode,0,0, sync);
                       // XXX : todo, pass some tings like ctrl+l and ctrl+r
                       //       through?
                       e.preventDefault();
                       e.stopPropagate = 1;
                       };

       canvas.onkeyup = function (e){
          var sync = 0;
                       _ctx_key_up(_ctx,e.keyCode,0,0, sync);
                       e.preventDefault();
                       e.stopPropagate = 1;
                       };
       canvas.regevents = true;
     }


  );

   

#ifdef EMSCRIPTEN
#ifdef ASYNCIFY
   emscripten_sleep(0);
#endif
#endif
   return 0;
}

EMSCRIPTEN_KEEPALIVE
uint8_t wasm_scratch[1024*1024*4];


static void set_pixels (Ctx *ctx, void *user_data, int x0, int y0, int w, int h, void *buf, int buf_size)
{
  uint8_t *src = (uint8_t*)buf;
  int in_w = w;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x0 + w >= ctx_width (ctx))
  {
     w = ctx_width (ctx) - x0 - 1;
  // fprintf (stderr, "adjusting xbounds\n");
  }
  if (y0 + h >= ctx_height (ctx))
  {
     h = ctx_height (ctx) - y0 - 1;
  // fprintf (stderr, "adjusting ybounds\n");
  }
  for (int i = 0; i < h; i++)
  {
    ctx_RGB565_BS_to_RGBA8 (NULL, x0, src + i * in_w * 2,
                    wasm_scratch + i * w * 4, w);
  }

  EM_ASM(
    var x0 = $0;
    var y0 = $1;
    var w = $2;
    var h = $3;
    var canvas = document.getElementById('c');
    var context = canvas.getContext('2d');
    var _ctx = _ctx_wasm_get_context(0); // we presume an earlier
                                         // call to have passed
                                         // construction flags
    const offset = _get_fb(canvas.width, canvas.height);
    const imgData = context.createImageData(w,h);

    const linearMem = new Uint8Array(wasmMemory.buffer, _wasm_scratch,
                                     w*h*4);

    for (let i = 0; i < w * h;i++)
    {
      //var a = linearMem[i*4+3];
      //var r = 1.0;
      //if (a!=0) r = 255.0/a;
      imgData.data[i*4+0] = linearMem[i*4+0];// * r;
      imgData.data[i*4+1] = linearMem[i*4+1];// * r;
      imgData.data[i*4+2] = linearMem[i*4+2];// * r;
      imgData.data[i*4+3] = 255;
    }
    context.putImageData(imgData,x0,y0);
  , x0,y0, w, h);

}

int wasm_damage_control = 0;

CTX_EXPORT
void wasm_set_damage_control(int val)
{
  wasm_damage_control = val;
}


void ctx_wasm_reset (void)
{
  if (fb) free (fb); fb = NULL;
  em_ctx = NULL;
}

Ctx *ctx_wasm_get_context (int flags)
{

EM_ASM(
    {var canvas = document.getElementById('c');
     const offset = _get_fb(canvas.width, canvas.height);

     var dc = document.getElementById('damagecontrol');
     if (dc)
     {
       _wasm_set_damage_control(dc.checked?1:0);
     }
   }
);
   static uint8_t *scratch = NULL;

   if (!scratch) scratch = malloc(512*1024); // XXX this should be unnneded,
                                             // we should be able to just mp allocated
                                             // memory instead

   if (!em_ctx){
      em_ctx = ctx_new_cb (width, height, CTX_FORMAT_RGB565_BYTESWAPPED,
                           set_pixels, 
                           NULL,
                           update_fb,
                           NULL,
                           23*1024, scratch, 
                           flags);
   }

   if (wasm_damage_control)
   {
     int flags = ctx_cb_get_flags (em_ctx);
     flags |= CTX_FLAG_DAMAGE_CONTROL;
     ctx_cb_set_flags (em_ctx, flags);
   }
   else
   {
     int flags = ctx_cb_get_flags (em_ctx);
     flags &= ~CTX_FLAG_DAMAGE_CONTROL;
     ctx_cb_set_flags (em_ctx, flags);
   }
   return em_ctx;
}

#endif
