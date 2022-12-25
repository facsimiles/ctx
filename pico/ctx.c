
#define CTX_PTY                 0
#define CTX_1BIT_CLIP           1
#define CTX_RASTERIZER_AA       15
#define CTX_RASTERIZER_FORCE_AA 0
#define CTX_SHAPE_CACHE         0
#define CTX_SHAPE_CACHE_DIM     16*18
#define CTX_SHAPE_CACHE_ENTRIES 128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 36
#define CTX_MIN_EDGE_LIST_SIZE 256
#define CTX_MAX_EDGE_LIST_SIZE 512
#define CTX_MIN_JOURNAL_SIZE   8192
#define CTX_MAX_JOURNAL_SIZE   8192

#define CTX_LIMIT_FORMATS       1
#define CTX_DITHER              1
#define CTX_32BIT_SEGMENTS      0
#define CTX_ENABLE_RGB565       1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_BITPACK_PACKER      0
#define CTX_COMPOSITING_GROUPS  0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_GRADIENT_CACHE      1
#define CTX_ENABLE_CLIP         1
#define CTX_BLOATY_FAST_PATHS   0

#define CTX_VT         1
#define CTX_PARSER     1
#define CTX_RASTERIZER 1
#define CTX_EVENTS     1
#define CTX_RAW_KB_EVENTS 0
#define CTX_STRINGPOOL_SIZE 512
#define CTX_FORMATTER 0
#define CTX_TERMINAL_EVENTS 1
#define CTX_FONTS_FROM_FILE 0

#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))
#include <stdint.h>
#include "Arimo-Regular.h"
#include "Cousine-Regular.h"
#include "Cousine-Bold.h"
#include "Cousine-Italic.h"

#define CTX_FONT_0   CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_8   CTX_STATIC_FONT(Cousine_Regular)
#define CTX_FONT_9   CTX_STATIC_FONT(Cousine_Italic)
#define CTX_FONT_10  CTX_STATIC_FONT(Cousine_Bold)


#define CTX_IMPLEMENTATION 1
#include "ctx.h"


#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

#include "st7789_lcd.pio.h"

// usb
#include "bsp/board.h"
#include "tusb.h"

uint8_t scratch[24*1024]; // perhaps too small, but for a flexible terminal
                          // we need all the memory possible for terminal
                          // and its scrollback
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

#if 0
#define PIN_DIN 11
#define PIN_CLK 10
#define PIN_CS 9
#define PIN_DC 8
#define PIN_RESET 14
#define PIN_BL 13
#else
#define PIN_DIN 11
#define PIN_CLK 10
#define PIN_CS 9
#define PIN_DC 8
#define PIN_RESET 12
#define PIN_BL 13

#endif

#define SERIAL_CLK_DIV 2.0f

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
static const uint8_t st7789_init_seq[] = {
        1, 20, 0x01,                         // Software reset
        1, 10, 0x11,                         // Exit sleep mode
        2, 2, 0x3a, 0x55,                   // Set colour mode to 16 bit
        2, 0, 0x36, 0x00,                   // Set MADCTL: row then column, refresh is bottom to top ????
        5, 0, 0x2a, 0x00, 0x00, 0x00, 0xf0, // CASET: column addresses from 0 to 240 (f0)
        5, 0, 0x2b, 0x00, 0x00, 0x01, 0x40, // RASET: row addresses from 0 to 240 (f0)
        1, 2, 0x21,                         // Inversion on, then 10 ms delay (supposedly a hack?)
        1, 2, 0x13,                         // Normal display on, then 10 ms delay
        1, 2, 0x29,                         // Main screen turn on, then wait 500 ms
        0                                     // Terminate list
};

static inline void lcd_set_dc_cs(bool dc, bool cs) {
    sleep_us(1);
    gpio_put_masked((1u << PIN_DC) | (1u << PIN_CS), !!dc << PIN_DC | !!cs << PIN_CS);
    sleep_us(1);
}

static inline void lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count) {
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(0, 0);
    st7789_lcd_put(pio, sm, *cmd++);
    if (count >= 2) {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 0);
        for (size_t i = 0; i < count - 1; ++i)
            st7789_lcd_put(pio, sm, *cmd++);
    }
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

static inline void spi_run_commands(PIO pio, uint sm, const uint8_t *init_seq) {
    const uint8_t *cmd = init_seq;
    while (*cmd) {
        lcd_write_cmd(pio, sm, cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void st7789_set_window(PIO pio, uint sm, int x0, int y0, int x1, int y1) {
   uint8_t caset_cmd[] =
       {5, 0, 0x2a, 0x00, 0x00, 0x00, 0xf0, 0};
   uint8_t raset_cmd[] =
       {5, 0, 0x2b, 0x00, 0x00, 0x01, 0x40, 0};

   caset_cmd[3] = x0 >> 8;
   caset_cmd[4] = x0 & 0xff;
   caset_cmd[5] = x1 >> 8;
   caset_cmd[6] = x1 & 0xff;

   raset_cmd[3] = y0 >> 8;
   raset_cmd[4] = y0 & 0xff;
   raset_cmd[5] = y1 >> 8;
   raset_cmd[6] = y1 & 0xff;

   spi_run_commands(pio,sm, caset_cmd);
   spi_run_commands(pio,sm, raset_cmd);
}

static inline void st7789_start_pixels(PIO pio, uint sm) {
    uint8_t cmd = 0x2c; // RAMWR
    lcd_write_cmd(pio, sm, &cmd, 1);
    lcd_set_dc_cs(1, 0);
}

#include "ctx.h"



    PIO pio = pio0;
    uint sm = 0;
void fb_init (void)
{
}

static void fb_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf, int buf_size)
{
    uint8_t *pixels = (uint8_t*)buf;
    st7789_set_window(pio, sm, x,y,x+w-1,y+h-1);
    st7789_start_pixels(pio, sm);
    for (int i = 0; i < w*h*2;i+=2)
    {
      st7789_lcd_put(pio, sm, pixels[i]);
      st7789_lcd_put(pio, sm, pixels[i+1]);
    }
}
extern void hid_app_task(void);
void cdc_task(void)
{
}

#if 0
void tuh_mount_cb(uint8_t dev_addr)
{
  char buf[400];
  // application set-up
  sprintf(buf, "A device with address %d is mounted\r\n", dev_addr);
  buffer_add_str(buf);
  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // schedule first transfer
}

void tuh_umount_cb(uint8_t dev_addr)
{
  char buf[400];
  // application tear-down
  sprintf(buf, "A device with address %d is unmounted \r\n", dev_addr);
  buffer_add_str(buf);
}

// invoked ISR context
#endif
CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };
void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes)
{ 
  (void) event;
  (void) pipe_id;
  (void) xferred_bytes;
  
  printf(serial_in_buffer);
  tu_memclr(serial_in_buffer, sizeof(serial_in_buffer));
  
  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // waiting for next data
}
static void ghostbuster(Ctx *ctx)
{
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,width, height);
    ctx_rgb(ctx,0,0,0);ctx_fill(ctx);
    ctx_end_frame(ctx);
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,width, height);
    ctx_rgb(ctx,1,1,1);ctx_fill(ctx);
    ctx_end_frame(ctx);
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,width, height);
    ctx_rgb(ctx,0,0,0);ctx_fill(ctx);
    ctx_end_frame(ctx);
}


static int ctx_usb_task (Ctx *ctx, void *data)
{
  tuh_task();
  //cdc_task();
  hid_app_task();
  return 1;
}


Ctx *ctx_pico_st7789_init (int fb_width, int fb_height,
                           int pin_din, int pin_clk, int pin_cs, int pin_dc,
                           int pin_reset, int pin_backlight, float clk_div)
{
    uint offset = pio_add_program(pio, &st7789_lcd_program);
    st7789_lcd_program_init(pio, sm, offset, pin_din, pin_clk, clk_div);

    gpio_init(pin_cs);
    gpio_init(pin_dc);
    gpio_init(pin_reset);
    gpio_init(pin_backlight);
    gpio_set_dir(pin_cs, GPIO_OUT);
    gpio_set_dir(pin_dc, GPIO_OUT);
    gpio_set_dir(pin_reset, GPIO_OUT);
    gpio_set_dir(pin_backlight, GPIO_OUT);

    gpio_put(pin_cs, 1);
    gpio_put(pin_reset, 1);

    spi_run_commands (pio, sm, st7789_init_seq);
    gpio_put(pin_backlight, 1);

    Ctx *ctx = ctx_new_cb(fb_width, fb_height, CTX_FORMAT_RGB565_BYTESWAPPED,
                          fb_set_pixels,
                          NULL,
                          NULL, // update_fb
                          NULL,
                          sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE);
    ghostbuster(ctx);

    board_init();
    tusb_init();
    ctx_add_idle (ctx, ctx_usb_task, ctx); 
    return ctx;
}


Ctx *ctx_pico_init (void)
{
  return ctx_pico_st7789_init (SCREEN_WIDTH, SCREEN_HEIGHT,
                               PIN_DIN,
                               PIN_CLK,
                               PIN_CS,
                               PIN_DC,
                               PIN_RESET,
                               PIN_BL,
                               SERIAL_CLK_DIV);
}
