#pragma GCC optimize("jump-tables,tree-switch-conversion")

#define LCD_GC9A01    0
#define LCD_ILI9341   1
#define TOUCH_CST816S 0

#define DISPLAY_WIDTH  240
#if LCD_GC9A01
#define DISPLAY_HEIGHT 240
#endif
#if LCD_ILI9341
#define DISPLAY_HEIGHT 320
#endif

#define SCRATCH_BUF_BYTES (42*1024)
#define CTX_ESP                            1
#define CTX_DITHER                         1
#define CTX_PROTOCOL_U8_COLOR              1
#define CTX_LIMIT_FORMATS                  1
#define CTX_32BIT_SEGMENTS                 0
#define CTX_RASTERIZER                     1
#define CTX_RASTERIZER_AA                  3
#define CTX_ENABLE_GRAY1                   1
#define CTX_ENABLE_GRAY2                   1
#define CTX_ENABLE_GRAY4                   1
#define CTX_ENABLE_GRAY8                   1
#define CTX_ENABLE_RGB332                  1
#define CTX_ENABLE_RGB565                  1
#define CTX_ENABLE_RGB565_BYTESWAPPED      1
#define CTX_COMPOSITING_GROUPS             0
#define CTX_ALWAYS_USE_NEAREST_FOR_SCALE1  1
#define CTX_EVENTS                         1
#define CTX_FORCE_INLINES                  1
#define CTX_THREADS                        0
#define CTX_TILED                          0
#define CTX_BAREMETAL                      1
#define CTX_ONE_FONT_ENGINE                1

#define CTX_MAX_SCANLINE_LENGTH            480
#define CTX_MAX_FRAMEBUFFER_WIDTH CTX_MAX_SCANLINE_LENGTH
#define CTX_MAX_JOURNAL_SIZE               (1024*512)
// is also max and limits complexity
// of paths that can be filled
#define CTX_MIN_EDGE_LIST_SIZE             512

#define CTX_HASH_COLS                      4
#define CTX_HASH_ROWS                      4

#define CTX_MAX_DASHES                     32
#define CTX_MAX_GRADIENT_STOPS             10
#define CTX_MAX_STATES                     10
#define CTX_MAX_EDGES                      127
#define CTX_MAX_PENDING                    64

#define CTX_GRADIENT_CACHE_ELEMENTS        128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 64
#define CTX_MAX_KEYDB                      16
#define CTX_MAX_TEXTURES                   16
#define CTX_PARSER_MAXLEN                  512
#define CTX_PARSER_FIXED_TEMP              1
#define CTX_STRINGPOOL_SIZE                256
#define CTX_MAX_DEVICES                    1
#define CTX_MAX_KEYBINDINGS                16
#define CTX_MAX_CBS                        8
#define CTX_MAX_LISTEN_FDS                 1
#define CTX_TERMINAL_EVENTS                0
#define CTX_FRAGMENT_SPECIALIZE            1
#define CTX_GSTATE_PROTECT                 1
#define CTX_COMPOSITE_O2                   1
#define CTX_RASTERIZER_SWITCH_DISPATCH     0
#define CTX_NATIVE_GRAYA8                  1
#define CTX_AVOID_CLIPPED_SUBDIVISION      1

#define CTX_IMPLEMENTATION
#include "ctx.h"

#include "driver/i2c.h"


#if TOUCH_CST816S 
#include "esp_lcd_touch_cst816s.h"
#endif

#if LCD_GC9A01

#define LCD_PIN_SCLK        6
#define LCD_PIN_MOSI        7
#define LCD_PIN_MISO        GPIO_NUM_NC
#define LCD_PIN_DC          2
#define LCD_PIN_CS          10
#define LCD_PIN_BACKLIGHT   3
#define LCD_MHZ             80
#include "driver/spi_master.h"

#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "esp_err.h"
#include "esp_log.h"
#endif
#if LCD_ILI9341
#define TOUCH_CS            33
#define TOUCH_MHZ           25
#define LCD_PIN_SCLK        14
#define LCD_PIN_MOSI        13
#define LCD_PIN_MISO        12
#define LCD_PIN_DC          2
#define LCD_PIN_CS          15
#define LCD_PIN_BACKLIGHT   27
#define LCD_MHZ             55

#include "driver/spi_master.h"

#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_err.h"
#include "esp_log.h"
#endif


static const char *TAG = "example";

static uint8_t scratch[SCRATCH_BUF_BYTES];
#if TOUCH_CST816S 
static esp_lcd_touch_handle_t tp = NULL;
#endif

static void touch_init()
{
#if TOUCH_CST816S 
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 4,
        .scl_io_num = 5,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, // 400kHz
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0));


    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    //ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = DISPLAY_WIDTH,
        .y_max = DISPLAY_HEIGHT,
        .rst_gpio_num = 1,
        .int_gpio_num = 0,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
#endif
}

static int frame_done_ctx (Ctx *ctx, void *user_data)
{
#if TOUCH_CST816S 
    esp_lcd_touch_read_data(tp);
    static uint16_t touch_x[1];
    static uint16_t touch_y[1];
    static uint16_t touch_strength[1];
    uint8_t  touch_cnt = 0;
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1);
    float x = touch_x[0];
    float y = touch_y[0];
    x -= 10;
    y -= 10;
    if (y < 0) y= 0;
    if (x < 0) x= 0;
    y *= 1.07;
    x *= 1.05;
    if (y > DISPLAY_HEIGHT) y= DISPLAY_HEIGHT;
    if (x > DISPLAY_WIDTH) x= DISPLAY_WIDTH;

    static bool was_pressed = false;
    if (touchpad_pressed)
    {
      if (!was_pressed)
        ctx_pointer_press (ctx, x, y, 0, 0);
      else
        ctx_pointer_motion (ctx, x, y, 0, 0);
    }
    else if (was_pressed)
        ctx_pointer_release (ctx, x, y, 0, 0);
    was_pressed = touchpad_pressed;
#endif
    vTaskDelay(1);
    return 0;
}

void esp_backlight(int percent)
{
#if LCD_GC9A01XX
  if (percent < 7)percent = 7;
  GC9A01_SetBL(percent);
#endif
}


#if LCD_ILI9341 | LCD_GC9A01
    esp_lcd_panel_handle_t panel_handle = NULL;
#endif

static int fb_ready = 1;

static bool lcd_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    fb_ready = 1;
    return false;
}


static void lcd_init (void)
{
#if LCD_ILI9341 | LCD_GC9A01

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BACKLIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
  gpio_set_level(LCD_PIN_BACKLIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);


 spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_SCLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = LCD_MHZ * 1000000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = lcd_flush_ready,
        //.user_ctx = &disp_drv,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, //EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
#if LCD_ILI9341
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
#elif LCD_GC9A01
    ESP_LOGI(TAG, "Install GC9A01 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));  
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
#endif

}

static void set_pixels_ctx (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{   
#if LCD_ILI9341 || LCD_GC9A01
    //fb_ready = 0;
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x+w, y+h, buf);
    //while(!fb_ready) vTaskDelay(1);
#endif
}

Ctx *ctx_host(void)
{
  static Ctx *ctx = NULL;
  if (ctx) return ctx;

  lcd_init();
  touch_init();
  ctx = ctx_new_cb(DISPLAY_WIDTH, DISPLAY_HEIGHT, CTX_FORMAT_RGB565_BYTESWAPPED,
                   set_pixels_ctx,
                   NULL,
                   frame_done_ctx,
                   NULL, 
//                 sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE|CTX_FLAG_LOWFI);
                   sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE);//|CTX_FLAG_KEEP_DATA);

  
  return ctx;
}
