#include "epicardium.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "emscripten.h"

#include "py/obj.h"
#include "py/runtime.h"

static Ctx *wasm_ctx = NULL;

void epic_set_ctx (Ctx *ctx)
{
  wasm_ctx = ctx;
}

static inline void
rgb565_to_rgb888(uint16_t pixel, uint8_t *red, uint8_t *green, uint8_t *blue)
{
        *blue  = (pixel & 31) << 3;
        *green = ((pixel >> 5) & 63) << 2;
        *red   = ((pixel >> 11) & 31) << 3;
}

int epic_file_stat (const char* path, struct epic_stat* stat)
{
  struct stat native_stat;
  int ret = lstat (path, &native_stat);
  stat->type = EPICSTAT_NONE;
  if (!ret)
  {
    switch (native_stat.st_mode & S_IFMT)
    {
       case S_IFREG:
         stat->size = native_stat.st_size;
         /* FALLTHROUGH */
       case S_IFDIR:
         if ((native_stat.st_mode & S_IFMT) == S_IFDIR)
           stat->type = EPICSTAT_DIR;
         else
           stat->type = EPICSTAT_FILE;
         if (strrchr (path, '/'))
         {
           strncpy (stat->name, strrchr (path, '/')+1, EPICSTAT_MAX_PATH);
         }
         else
         {
           strncpy (stat->name, path, EPICSTAT_MAX_PATH);
         }
         break;
    }
  }
  return ret;
}

int epic_file_write (int fd, const void *buf, size_t nbytes)
{
  return write (fd, buf, nbytes);
}

int epic_file_read (int fd, void *buf, size_t nbytes)
{
  return read (fd, buf, nbytes);
}

#define EPIC_MAX_OPENDIRS 32
DIR *open_dirs[EPIC_MAX_OPENDIRS];

int epic_file_opendir (const char *path)
{
  for (int i = 0; i < EPIC_MAX_OPENDIRS; i++)
  {
    if (open_dirs[i] == NULL)
    {
      open_dirs[i] = opendir (path);
      return 9000 + i;
    }
  }
  return -1;
}

int epic_file_readdir (int fd, struct epic_stat *entry)
{
  fd -= 9000;
  if (fd < 0 || fd >= EPIC_MAX_OPENDIRS || open_dirs[fd] == NULL) return -1;
  struct dirent *native = readdir (open_dirs[fd]);
  entry->type = EPICSTAT_NONE;
  if (!native)
  {
    return 0;
  }
    switch (native->d_type)
    {
       case DT_REG:
       case DT_LNK:
         entry->type = EPICSTAT_FILE;
         break;
       case DT_DIR:
         entry->type = EPICSTAT_DIR;
         break;
       default:
         break;
    }
  strncpy (entry->name, native->d_name, EPICSTAT_MAX_PATH-1);
  return 0;
}

int epic_file_close (int fd)
{
  if (fd >= 9000)
  {
    fd-=9000;
    if (fd >= EPIC_MAX_OPENDIRS) return -1;
    if (!open_dirs[fd]) return -2;
    closedir (open_dirs[fd]);
    open_dirs[fd]=NULL;
    return 0;
  }
  return close (fd);
}

int epic_file_mkdir (const char *path)
{
  return mkdir (path, 0777);
}

int epic_file_unlink(const char *path)
{
  return unlink (path);
}

int epic_file_rename (const char *a, const char *b)
{
  return rename (a, b);
}

void epic_system_reset (void) { }
int epic_read_battery_voltage(float *result)
{
  *result = 23;
  return 0;
}

void epic_exit (int val)
{
  mp_raise_type(&mp_type_SystemExit);
}

int epic_file_open (const char *path, const char *modeString)
{
  int mode = O_RDONLY;
  if (strchr (modeString, 'r')) mode |= O_RDONLY;

  if (strchr (modeString, 'w'))
  {
     mode |= O_WRONLY;
     if (!access (path, R_OK))
     {
       unlink (path);
     }
     mode |= O_CREAT;
  }
  return open (path, mode);
}

int epic_file_tell (int fd)
{
  return lseek (fd, 0, SEEK_CUR);
}

int epic_file_seek (int fd, long offset, int whence)
{
  return lseek (fd, offset, whence);
}

EMSCRIPTEN_KEEPALIVE
char epic_exec_path[256]="";

int epic_exec (char *path)
{
  strncpy (epic_exec_path, path, sizeof(epic_exec_path)-1);
  mp_raise_type(&mp_type_SystemExit);
  return 0;
}

EMSCRIPTEN_KEEPALIVE
uint32_t epic_buttons = 0;
void mp_idle (int ms); 
uint8_t epic_buttons_read (uint8_t mask)
{
  emscripten_sleep(0);
  //mp_idle (0); // 1 means no gc
  return epic_buttons & mask;
}

static const float font_map[] = {
        [DISP_FONT8] = 8.0f,   [DISP_FONT12] = 12.0f, [DISP_FONT16] = 16.0f,
        [DISP_FONT20] = 20.0f, [DISP_FONT24] = 24.0f,
};

int epic_disp_print_adv(
        uint8_t font,
        int16_t posx,
        int16_t posy,
        const char *pString,
        uint16_t fg,
        uint16_t bg
) {
        Ctx *ctx = wasm_ctx;
        uint8_t r, g, b;
        //int cl = check_lock();
        //if (cl < 0) {
        //        return cl;
       // }

        if (font >= (sizeof(font_map) / sizeof(font_map[0]))) {
                return -EINVAL;
        }

        float font_size = font_map[font];
        ctx_font_size(ctx, font_size);

        if (fg != bg) {
                /* non-transparent background */
                rgb565_to_rgb888(bg, &r, &g, &b);
                ctx_rgba8(ctx, r, g, b, 255);
                float width = ctx_text_width(ctx, pString);
                ctx_rectangle(ctx, posx, posy, width, font_size);
                ctx_fill(ctx);
        }

        rgb565_to_rgb888(fg, &r, &g, &b);
        ctx_rgba8(ctx, r, g, b, 255);
        ctx_move_to(ctx, posx, (float)posy + font_size * 0.8f);
        ctx_text(ctx, pString);

        return 0;
}

int epic_disp_print (
        int16_t posx,
        int16_t posy,
        const char *pString,
        uint16_t fg,
        uint16_t bg)
{
   return epic_disp_print_adv(DISP_FONT20, posx, posy, pString, fg, bg);
}



int epic_disp_pixel (int16_t x, int16_t y, uint16_t col)
{
  return epic_disp_rect (x, y, x, y, col, FILLSTYLE_FILLED, 0);
}

int epic_disp_close (void)
{
  //emscripten_sleep (0);
  return 0;
}


void key_down_cb (CtxEvent *e, void *data1, void *data2)
{
  if (!strcmp (e->string, "left"))
    epic_buttons |= BUTTON_LEFT_BOTTOM;
  else if (!strcmp (e->string, "right"))
    epic_buttons |= BUTTON_RIGHT_BOTTOM;
  else if (!strcmp (e->string, "return")||
           !strcmp (e->string, "down")||
           !strcmp (e->string, "space"))
    epic_buttons |= BUTTON_RIGHT_TOP;
  else if (!strcmp (e->string, "esc")||
           !strcmp (e->string, "up")||
           !strcmp (e->string, "tab"))
  {
    epic_buttons |= BUTTON_LEFT_TOP;
    epic_exec ("menu.py");
  }
}

void key_up_cb (CtxEvent *e, void *data1, void *data2)
{
  if (!strcmp (e->string, "left"))
    epic_buttons &= ~BUTTON_LEFT_BOTTOM;
  else if (!strcmp (e->string, "right"))
    epic_buttons &= ~BUTTON_RIGHT_BOTTOM;
  else if (!strcmp (e->string, "return")||
           !strcmp (e->string, "space")||
           !strcmp (e->string, "down"))
    epic_buttons &= ~BUTTON_RIGHT_TOP;
  else if (!strcmp (e->string, "up"))
  {
    epic_buttons &= ~BUTTON_LEFT_TOP;
  }
}

int epic_disp_open (void)
{
  Ctx *ctx = wasm_ctx;
  ctx_start_frame (ctx);
  //ctx_clear_bindings (ctx);
  ctx_listen (ctx, CTX_KEY_DOWN,
                   key_down_cb,
                   NULL, NULL);
  ctx_listen (ctx, CTX_KEY_UP,
                   key_up_cb,
                   NULL, NULL);
  ctx_rectangle (ctx, 0, 0, 180, 80);
  ctx_begin_path (ctx);
  return 0;
}

int epic_disp_update (void)
{
  Ctx *ctx = wasm_ctx;
  ctx_end_frame (ctx);
  return 0;
}

int epic_disp_clear (uint16_t color)
{
  Ctx *ctx = wasm_ctx;
  uint8_t r, g, b;
  rgb565_to_rgb888(color, &r, &g, &b);
  ctx_rgba8 (ctx, r, g, b, 255);
  ctx_paint (ctx);
  return 0;
}

int epic_disp_circ (int16_t x, int16_t y, uint16_t rad, uint16_t color, enum disp_fillstyle fs, uint16_t ps)
{
   Ctx *ctx = wasm_ctx;
   uint8_t r, g, b;
   rgb565_to_rgb888(color, &r, &g, &b);

   ctx_arc(ctx, x, y, rad, 0.0f, CTX_PI * 1.95f, 0);

   switch (fs) {
  case FILLSTYLE_EMPTY:
          ctx_rgba8_stroke(ctx, r, g, b, 255);
          ctx_line_width(ctx, ps);
          ctx_stroke(ctx);
          break;
  case FILLSTYLE_FILLED:
          ctx_rgba8(ctx, r, g, b, 255);
          ctx_fill(ctx);
          break;
  }

  return 0;
}

int epic_disp_rect (int16_t xs, int16_t ys, int16_t xe, int16_t ye, uint16_t col, enum disp_fillstyle fillstyle, uint16_t ps)
{
   Ctx *ctx = wasm_ctx;
   uint8_t r, g, b;
   rgb565_to_rgb888(col, &r, &g, &b);
   ctx_rectangle (ctx, xs, ys, xe-xs+1, ye-ys+1);
   ctx_rgba8 (ctx, r, g, b, 255);
   if (fillstyle == FILLSTYLE_EMPTY)
   {
     ctx_line_width (ctx, ps);
     ctx_stroke(ctx);
   }
   else
   {
     ctx_fill(ctx);
   }
  return 0;
}

int epic_disp_line (int16_t xs, int16_t ys, int16_t xe, int16_t ye, uint16_t col, enum disp_linestyle ls, uint16_t pixelsize)
{
   Ctx *ctx = wasm_ctx;
   uint8_t r, g, b;
   rgb565_to_rgb888(col, &r, &g, &b);
   ctx_rgba8 (ctx, r, g, b, 255);

   float xstartf = xs, ystartf = ys, xendf = xe, yendf = ye;

   /*
    * For odd line widths, shift the line by half a pixel so it aligns
    * perfectly with the pixel grid.
    */
   if (pixelsize % 2 == 1) {
           xstartf += 0.5f;
           ystartf += 0.5f;
           yendf += 0.5f;
           xendf += 0.5f;
   }

   ctx_move_to (ctx, xstartf, ystartf);
   ctx_line_to (ctx, xendf, yendf);
   ctx_line_width (ctx, pixelsize);
   ctx_stroke(ctx);
  return 0;
}


int epic_disp_backlight (uint16_t brightness)
{
  return 0;
}

int epic_disp_blit (int16_t pos_x, int16_t pos_y, int16_t width, int16_t height, void *img, enum epic_rgb_format format)
{
  return 0;
}

int epic_config_set_string (const char *key, const char *value)
{
  return 0;
}

int epic_config_get_string (const char *key, char *value, size_t len)
{
  if (!strcmp (key, "long_press_ms"))
  {
     strncpy (value, "1000",len-1);
     return 0;
  }
  else if (!strcmp (key, "retrigger_ms"))
  {
     strncpy (value, "250", len-1);
     return 0;
  }
  else if (!strcmp (key, "right_scroll"))
  {
     strncpy (value, "0", len-1);
     return 0;
  }
  else if (!strcmp (key, "default_app"))
  {
     //strncpy (value, "/apps/g_watch/__init__.py", len-1);
     strncpy (value, "/apps/digiclk/__init__.py", len-1);
     return 0;
  }
  return -ENOENT;
}

int epic_personal_state_set (uint8_t state, bool set)
{
  return 0;
}

int epic_personal_state_get (void)
{
  return 0;
}

int epic_personal_state_is_persistent (void)
{
  return 0;
}

void epic_leds_set_gamma_table(
                uint8_t rgb_channel, uint8_t *gamma_table)
{
}

void epic_leds_prep(int led, uint8_t r, uint8_t g, uint8_t b)
{
}

void epic_leds_set_all (uint8_t *pat, uint8_t len)
{
}

void epic_leds_dim_top (uint8_t dim)
{
}

void epic_leds_dim_bottom (uint8_t dim)
{
}

void epic_leds_update (void)
{
}

void epic_leds_set_all_hsv (float *pat, uint8_t len)
{
}

void epic_leds_set_powersave(bool eco)
{
}

void epic_set_flashlight(bool power)
{
}


void epic_leds_flash_rocket (int no, uint8_t val, int time)
{
}

int epic_leds_get_rocket (int no)
{
  return 0;
}

void epic_leds_prep_hsv(int led, float h, float s, float v)
{
}

void epic_leds_set_hsv(int led, float h, float s, float v)
{
}

int epic_leds_get_rgb(int led, uint8_t *rgb)
{
  return 0;
}


int epic_bhi160_enable_sensor(
        enum bhi160_sensor_type sensor_type,
        struct bhi160_sensor_config *config
)
{
  return 0;
}

int epic_bhi160_disable_sensor(
        enum bhi160_sensor_type sensor_type
)
{
  return 0;
}


void epic_bhi160_disable_all_sensors(void)
{
}


int epic_stream_read(int sd, void *buf, size_t count)
{
  return 0;
}

int epic_gpio_set_pin_mode (uint8_t pin, uint8_t mode)
{
  return 0;
}

int epic_gpio_get_pin_mode (uint8_t pin)
{
  return 0;
}

int epic_gpio_read_pin (uint8_t pin)
{
  return 0;
}

int epic_gpio_write_pin (uint8_t pin, bool on)
{
  return 0;
}

uint64_t emscripten_ticks_us (void);

uint32_t epic_rtc_get_monotonic_seconds(void)
{
  return emscripten_ticks_us () / 1000 / 1000;
}

uint64_t epic_rtc_get_monotonic_milliseconds(void)
{
  return emscripten_ticks_us () / 1000;
}

uint32_t epic_rtc_get_seconds(void)
{
  return emscripten_ticks_us () / 1000 / 1000;
}

uint64_t epic_rtc_get_milliseconds(void)
{
  return emscripten_ticks_us () / 1000;
}

void epic_rtc_set_milliseconds(uint64_t milliseconds)
{
}

int epic_rtc_schedule_alarm(uint32_t timestamp)
{
  return 0;
}

int epic_light_sensor_run(void)
{
  return 0;
}

uint16_t epic_light_sensor_read(void)
{
  static int val = 0;
  if (val >= 42)
  {
    val = 23;
  }
  else
  {
    val++;
  }
  return val;
}

int epic_light_sensor_get(uint16_t *value)
{
  value[0] = epic_light_sensor_read();
  return 0;
}

int epic_light_sensor_stop(void)
{
  return 0;
}

int epic_read_battery_current(float *result)
{
  *result = 1.0;
  return 0;
}

int epic_read_chargein_voltage(float *result)
{
  *result = 1.0;
  return 0;
}

int epic_read_chargein_current(float *result)
{
  *result = 1.0;
  return 0;
}

int epic_read_system_voltage(float *result)
{
  *result = 1.0;
  return 0;
}

int epic_read_thermistor_voltage(float *result)
{
  *result = 1.0;
  return 0;
}

/* these are the led functions lsd nick seems to use */
uint8_t epic_leds[16*4]={0,};

EMSCRIPTEN_KEEPALIVE
uint32_t epic_get_led (int led)
{
  return ((uint32_t*)(&epic_leds[0]))[led];
}

void epic_leds_set(int led, uint8_t r, uint8_t g, uint8_t b)
{
  if (led < 0 || led > 14) return;
  epic_leds[led*4+0]=r;
  epic_leds[led*4+1]=g;
  epic_leds[led*4+2]=b;
}

void epic_leds_set_rocket (int no, uint8_t val)
{
  if (no<0 || no>2) return;
  epic_leds[15*4+no]=val*8;
}



