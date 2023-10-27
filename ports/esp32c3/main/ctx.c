#pragma GCC optimize("jump-tables,tree-switch-conversion")

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240

//#define SCRATCH_BUF_BYTES                  (46*1024)
#define SCRATCH_BUF_BYTES                  (55*1024)
#define CTX_HASH_COLS                      5
#define CTX_HASH_ROWS                      5

#define CTX_DITHER                         1
#define CTX_PROTOCOL_U8_COLOR              1
#define CTX_LIMIT_FORMATS                  1
#define CTX_32BIT_SEGMENTS                 0
#define CTX_RASTERIZER                     1
#define CTX_RASTERIZER_AA                  3
#if 0
#define CTX_ENABLE_GRAY1                   1
#define CTX_ENABLE_GRAY2                   1
#define CTX_ENABLE_GRAY4                   1
#define CTX_ENABLE_GRAY8                   1
#define CTX_ENABLE_RGB332                  1
#endif
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
#define CTX_BLENDING_AND_COMPOSITING       0
#define CTX_GSTATE_PROTECT                 0
#define CTX_COMPOSITE_O2                   1
//#define CTX_RASTERIZER_O3                1
#define CTX_RASTERIZER_SWITCH_DISPATCH     0
#define CTX_NATIVE_GRAYA8                  0
#define CTX_AVOID_CLIPPED_SUBDIVISION      1

#define CTX_IMPLEMENTATION
#include "ctx.h"

#include "gc9a01.h"
#include "driver/i2c.h"
#include "esp_lcd_touch_cst816s.h"


static uint8_t scratch[SCRATCH_BUF_BYTES];
static esp_lcd_touch_handle_t tp = NULL;

static void touch_init()
{
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
}

static int frame_done_ctx (Ctx *ctx, void *user_data)
{
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

    vTaskDelay(1);

    return 0;
}

void esp_backlight(int percent)
{
  if (percent < 7)percent = 7;
  GC9A01_SetBL(percent);
}

static void lcd_init (void)
{
  GC9A01_Init();
  esp_backlight(30);
}

static void set_pixels_ctx (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{   
    uint8_t *pixels = (uint8_t*)buf; 
    GC9A01_SetWindow(x,y,x+w-1,y+h-1);
    lcd_data(pixels,w*h*2);
}

Ctx *esp_ctx(void)
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
                   sizeof(scratch), scratch,
                   //CTX_FLAG_HASH_CACHE|CTX_FLAG_LOWFI
                   CTX_FLAG_HASH_CACHE|CTX_FLAG_KEEP_DATA
                   //0
                   );

  
  return ctx;
}
