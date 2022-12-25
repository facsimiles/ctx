/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "st7789_lcd.pio.h"

// usb
#include "bsp/board.h"
#include "tusb.h"

uint8_t scratch[24*1024]; // perhaps too small, but for a flexible terminal
                          // we need all the memory possible for terminal
                          // and its scrollback
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320
#define IMAGE_SIZE 256
#define LOG_IMAGE_SIZE 8

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

#define CTX_1BIT_CLIP           1
#define CTX_RASTERIZER_AA       15
#define CTX_RASTERIZER_FORCE_AA 0
#define CTX_SHAPE_CACHE         0
#define CTX_SHAPE_CACHE_DIM     16*18
#define CTX_SHAPE_CACHE_ENTRIES 128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 36
#define CTX_MIN_EDGE_LIST_SIZE 256
#define CTX_MAX_EDGE_LIST_SIZE 512
#define CTX_MIN_JOURNAL_SIZE   512
#define CTX_MAX_JOURNAL_SIZE   512

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
#include "ctx.h"



    PIO pio = pio0;
    uint sm = 0;
void fb_init (void)
{
    uint offset = pio_add_program(pio, &st7789_lcd_program);
    st7789_lcd_program_init(pio, sm, offset, PIN_DIN, PIN_CLK, SERIAL_CLK_DIV);

    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RESET);
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RESET, GPIO_OUT);
    gpio_set_dir(PIN_BL, GPIO_OUT);

    gpio_put(PIN_CS, 1);
    gpio_put(PIN_RESET, 1);


    spi_run_commands (pio, sm, st7789_init_seq);
    gpio_put(PIN_BL, 1);
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

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

typedef struct _BufferedLine BufferedLine;
struct _BufferedLine {
  char *str;
  BufferedLine *prev;
};

BufferedLine *scrollback = NULL;

int scrollback_count ()
{
  BufferedLine *line = scrollback;
  int count = 0;
  while (line)
  {
    count++; line = line->prev;
  }
  return count;
}

void trim_scrollback (int maxlines)
{
  BufferedLine *line = scrollback;

  int count = scrollback_count ();
  while (maxlines < count)
  {
    int no = 0;
    BufferedLine *line = scrollback;
    BufferedLine *next = NULL;
    for (;no < maxlines; no++) { next=line;line = line->prev; }
    
    next->prev = NULL;
    while (line)
    {
      free (line->str);
      BufferedLine *tmp=line;
      line=line->prev;
      free(line);
    }
    count = scrollback_count ();
  }
  

}


void buffer_add_byte (const char byte)
{
  BufferedLine *line = scrollback;

  if (!line)
  {
    line = calloc (sizeof (BufferedLine), 1);
    line->str = strdup("");
    scrollback = line;
  }

  if (byte == '\n')
  {
    line = calloc (sizeof (BufferedLine), 1);
    line->str = strdup("");
    line->prev = scrollback;
    scrollback = line;
    trim_scrollback (12);
  }
  else
  {
    int oldlen = strlen (line->str);
    char *copy = malloc (oldlen+2);
    strcpy(copy, line->str);
    copy[oldlen] = byte;
    copy[oldlen+1] = 0;
    free (line->str);
    line->str = copy;
  }
}

void buffer_add_str (const char *str)
{
  if (str[strlen(str)]=='\n')
  {
    BufferedLine *line = calloc (sizeof (BufferedLine), 1);
    line->str = strdup(str);
    line->prev = scrollback;
  }
  else
  if (str)
  for (int i = 0; str[i]; i++)
    buffer_add_byte (str[i]);
}

static int frame = 0;

void draw_text_buffer (Ctx *ctx)
{
      ctx_start_frame (ctx);
      float em = 18;
      ctx_wrap_right (ctx, SCREEN_WIDTH-em);
      ctx_line_height (ctx, 0.9);
      ctx_font_size (ctx, em);
     // ctx_move_to (ctx, 1, em);
     // char buf[64];sprintf(buf, "frame: %i\n",frame);
     // ctx_text (ctx, buf);

      float y = SCREEN_HEIGHT - em * 0.2;
      BufferedLine *line = scrollback;
      while (line && y > 0)
      {
        ctx_move_to (ctx, 1, y);
        ctx_text (ctx, line->str);
        line = line->prev;
        y -= em * 1.5;
      }
 
      ctx_end_frame (ctx);
      frame++;

}
extern void hid_app_task(void);
void cdc_task(void)
{
}

#if 1
CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };
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
void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes)
{ 
  (void) event;
  (void) pipe_id;
  (void) xferred_bytes;
  
  printf(serial_in_buffer);
  tu_memclr(serial_in_buffer, sizeof(serial_in_buffer));
  
  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // waiting for next data
}
#endif

static void ghostbuster(Ctx *ctx)
{
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
    ctx_rgb(ctx,0,0,0);ctx_fill(ctx);
    ctx_end_frame(ctx);
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
    ctx_rgb(ctx,1,1,1);ctx_fill(ctx);
    ctx_end_frame(ctx);
    ctx_start_frame(ctx);
    ctx_rectangle(ctx,0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
    ctx_rgb(ctx,0,0,0);ctx_fill(ctx);
    ctx_end_frame(ctx);
}

Ctx *ctx = NULL;
    CtxClient *client = NULL;



static void handle_event (Ctx        *ctx,
                          CtxEvent   *ctx_event,  
                          const char *event)
{
  ctx_client_feed_keystring (client, ctx_event, event);
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)  
{ 
  {  
    switch (event->type)
    {  
      case CTX_KEY_PRESS:
        handle_event (ctx, event, event->string);  
        break;  
      case CTX_KEY_UP:  
        { char buf[1024];  
          snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);  
          handle_event (ctx, event, buf);  
        }  
        break;  
      case CTX_KEY_DOWN:  
        { char buf[1024];  
          snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);  
          handle_event (ctx, event, buf);  
        }  
        break;  
      default:  
        break;  
    }  
  }  
}  

void on_uart_rx()
{
  while (uart_is_readable(uart0))
    ctx_vt_write (ctx, uart_getc(uart0));
  ctx_clients_handle_events (ctx);
  ctx_handle_events (ctx);
}



void ctx_pico_st7789_init (int pin_din, int pin_clk, int pin_cs, int pin_dc,
                           int pin_reset, int pin_backlight)
{
}




int main(int argc, char **argv) {
  set_sys_clock_khz(270000, true);
  //  stdio_init_all();


    fb_init();

    ctx = ctx_new_cb(SCREEN_WIDTH, SCREEN_HEIGHT, CTX_FORMAT_RGB565_BYTESWAPPED,
                          fb_set_pixels,
                          NULL,
                          NULL, // update_fb
                          NULL,
                          sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE);

    ghostbuster(ctx);

#if 1
    for (float scale = 10; scale < SCREEN_WIDTH; scale*=1.04)
    {
      tuh_task();
      //cdc_task();
      hid_app_task();
      ctx_start_frame (ctx);
      ctx_logo (ctx, SCREEN_WIDTH/2, SCREEN_HEIGHT*0.45, scale);
      ctx_end_frame (ctx);
    }

    for (float opacity = 0.0f; opacity < 1.0f; opacity +=0.04)
    {
      tuh_task();
      hid_app_task();
      ctx_start_frame (ctx);
      float em = SCREEN_WIDTH/8.0;
      ctx_font_size (ctx, em);
      ctx_logo (ctx, SCREEN_WIDTH/2, SCREEN_HEIGHT*0.45, SCREEN_WIDTH);
      ctx_rgba (ctx, 1.0, 1.0, 1.0, opacity);
      ctx_move_to (ctx, 1, SCREEN_HEIGHT-em*0.2);
      ctx_text (ctx, "ctx vector graphics");
      ctx_end_frame (ctx);
    }
    sleep_us(1000 * 1.5);
#endif

    board_init();
    tusb_init();

    int flags = 0;
    float start_font_size = 16.0;

    client = 
    ctx_client_new_argv (ctx, NULL, 0,0, SCREEN_WIDTH, SCREEN_HEIGHT,
      start_font_size, flags, NULL, NULL);

    ctx_client_maximize(ctx, ctx_client_id(client));

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled(UART_ID, false);
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

    while(true) // && !ctx_has_quit(ctx))
    {
      tuh_task();
      hid_app_task();

      if (ctx_need_redraw(ctx))
      {
        ctx_start_frame (ctx);
        ctx_clients_draw (ctx, 0);
        ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, NULL, NULL);  
        ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, NULL, NULL);  
        ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, NULL, NULL);  
        ctx_end_frame (ctx);
      }
      //draw_text_buffer (ctx);
      //while (uart_is_readable(uart0))
      //  ctx_vt_write (ctx, uart_getc(uart0));


      ctx_handle_events (ctx);
      while (ctx_vt_has_data (ctx))
        uart_putc (uart0, ctx_vt_read (ctx));

#if 0
      CtxEvent *event;
      while((event = ctx_get_event (ctx)))
      {
#if 0
        char buf[128];
        if (event->type == CTX_KEY_PRESS && !strcmp (event->string, "q"))
          ctx_quit (ctx);
        switch (event->type)
        {
          default:
           sprintf(buf, "type: %i", event->type);
           break;
          case CTX_KEY_DOWN: sprintf(buf, "down: %s\n", event->string);break;
          case CTX_KEY_PRESS: sprintf(buf, "press: %s\n", event->string);break;
          case CTX_KEY_UP: sprintf(buf, "up: %s\n", event->string);break;
        }
        buffer_add_str(buf);
#endif
      }
#endif

    }
    //ctx_destroy (ctx);
}
