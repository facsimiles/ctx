
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

EMSCRIPTEN_KEEPALIVE
float pointer_x = 0;
EMSCRIPTEN_KEEPALIVE
float pointer_y = 0;
EMSCRIPTEN_KEEPALIVE
int32_t pointer_down = 0;
int32_t pointer_was_down = 0;


static uint32_t key_queue[32];
static int key_queue_head = 0; // read head
static int key_queued = 0;

EMSCRIPTEN_KEEPALIVE
void ctx_wasm_queue_key_event (int type, int keycode)
{
  if (key_queued >= 31) return;
  int pos = (key_queue_head + key_queued) % 32;
  key_queue[pos * 2 + 0] = type;
  key_queue[pos * 2 + 1] = keycode;
  key_queued ++;
}

int ctx_wasm_get_key_event (int *type, int *keycode)
{
  if (!key_queued)
    return -1;

  *type = key_queue[key_queue_head * 2 + 0];
  *keycode = key_queue[key_queue_head * 2 + 1];

  key_queued--;
  key_queue_head++;
  key_queue_head = key_queue_head % 16;

  return 0;
}

int update_fb (Ctx *ctx, void *user_data)
{
  EM_ASM(
    var canvas = document.getElementById('c');
    var context = canvas.getContext('2d');

     if (!canvas.regevents)
     {
       canvas.onmousedown = function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          setValue(_pointer_x, loc.x, "float");
          setValue(_pointer_y, loc.y, "float");
          setValue(_pointer_down, 1, "i32");
          e.stopPropagate=1;
                       };
       canvas.onmouseup = function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          setValue(_pointer_x, loc.x, "float");
          setValue(_pointer_y, loc.y, "float");
          setValue(_pointer_down, 0, "i32");
          e.stopPropagate=1;
                       };
       canvas.onmousemove = function (e){
          var loc = windowToCanvas (canvas, e.clientX, e.clientY);
          setValue(_pointer_x, loc.x, "float");
          setValue(_pointer_y, loc.y, "float");
          e.stopPropagate=1;
                       };
       canvas.onkeydown = function (e){
          _ctx_wasm_queue_key_event (1, e.keyCode);
                       e.preventDefault();
                       e.stopPropagate = 1;
                       };

       canvas.onkeyup = function (e){
          _ctx_wasm_queue_key_event (2, e.keyCode);
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

   int sync = 0;
   int ret = 0;

   if (key_queued)
     while (key_queued)
   {
     int type = 0 , keycode = 0;

     ctx_wasm_get_key_event (&type, &keycode);
     switch (type)
     {
       case 1:
         ctx_key_down(ctx,keycode,NULL,0, sync);
         ctx_key_press(ctx,keycode,NULL,0, sync);
         ret = 1;
         break;
       case 2:
         ctx_key_up(ctx,keycode,NULL,0, sync);
         ret = 1;
         break;
     }
   }

   if (pointer_down && !pointer_was_down)
   {
      ctx_pointer_press (ctx, pointer_x, pointer_y, 0, 0, sync);
      ret = 1;
   } else if (!pointer_down && pointer_was_down)
   {
      ctx_pointer_release (ctx, pointer_x, pointer_y, 0, 0, sync);
      ret = 1;
   } else if (pointer_down)
   {
      ctx_pointer_motion (ctx, pointer_x, pointer_y, 0, 0, sync);
      ret = 1;
   }

   pointer_was_down = pointer_down;

   return ret;
}

EMSCRIPTEN_KEEPALIVE
uint8_t wasm_scratch[1024*1024*4];

static void set_pixels (Ctx *ctx, void *user_data, int x0, int y0, int w, int h, void *buf, int buf_size)
{
  uint8_t *src = (uint8_t*)buf;
  int in_w = w;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x0 + w > ctx_width (ctx))
  {
     fprintf (stderr, "adjusting xbounds from %i %i\n", x0, w);
     w = ctx_width (ctx) - x0;
  }
  if (y0 + h > ctx_height (ctx))
  {
     h = ctx_height (ctx) - y0;
     fprintf (stderr, "adjusting ybounds\n");
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
                                         // the memory budget
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

#if 0
int wasm_damage_control = 0;

CTX_EXPORT
void wasm_set_damage_control(int val)
{
  wasm_damage_control = val;
}
#endif

void ctx_wasm_reset (void)
{
  if (fb) free (fb); fb = NULL;
  em_ctx = NULL;
}

Ctx *ctx_wasm_get_context (int memory_budget)
{


EM_ASM(
    {var canvas = document.getElementById('c');
     const offset = _get_fb(canvas.width, canvas.height);

     //var dc = document.getElementById('damagecontrol');
     //if (dc)
     //{
     //  _wasm_set_damage_control(dc.checked?1:0);
     //}
   }
);

   if (em_ctx && memory_budget)
   {
      CtxCbBackend *cb_backend = (CtxCbBackend*)em_ctx->backend;
      if (memory_budget != cb_backend->memory_budget)
      {
         ctx_cb_set_memory_budget (em_ctx, memory_budget);
         ctx_cb_set_flags (em_ctx, 0);
      }
   }



   if (!em_ctx){
      em_ctx = ctx_new_cb (width, height, CTX_FORMAT_RGB565_BYTESWAPPED,
                           set_pixels, 
                           NULL,
                           update_fb,
                           NULL,
                           memory_budget, NULL, 
                           0);
   }

#if 0
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
#endif
   return em_ctx;
}

#endif
