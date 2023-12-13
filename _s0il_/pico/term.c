/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <math.h>

#include "ctx.h"

Ctx *ctx_pico_init (void);

Ctx *ctx = NULL;
CtxClient *client = NULL;

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1


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



int main(int argc, char **argv) {
  set_sys_clock_khz(270000, true);

  ctx = ctx_pico_init ();


#if 1
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    float mindim = width;
    if (mindim > height) mindim = height;
    for (float scale = 10; scale < mindim; scale*=1.04)
    {
      ctx_start_frame (ctx);
      ctx_logo (ctx, width/2, height*0.45, scale);
      ctx_end_frame (ctx);
    }

    for (float opacity = 0.0f; opacity < 1.0f; opacity +=0.04)
    {
      ctx_start_frame (ctx);
      float em = width/8.0;
      ctx_font_size (ctx, em);
      ctx_logo (ctx, width/2, height*0.45, mindim);
      ctx_rgba (ctx, 1.0, 1.0, 1.0, opacity);
      ctx_move_to (ctx, 1, height-em*0.2);
      ctx_text (ctx, "ctx vector graphics");
      ctx_end_frame (ctx);
    }
    sleep_us(1000 * 1.5);
#endif


    int flags = 0;
    float start_font_size = 14.0;

    client = 
    ctx_client_new_argv (ctx, NULL, 0,0, width, height,
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

    while(true) // && !ctx_has_exited(ctx))
    {
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
          ctx_exit (ctx);
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
