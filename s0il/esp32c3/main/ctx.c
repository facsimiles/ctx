#pragma GCC optimize("jump-tables,tree-switch-conversion")

#include "port_config.h"
#define TAG "ctx"

#define SCRATCH_BUF_BYTES (32 * 1024)
#define CTX_VT 1
#define CTX_PTY 0
#define CTX_THREAD 1
#define CTX_HASH_COLS 5
#define CTX_HASH_ROWS 5
#define CTX_DITHER 1
#define CTX_PARSER 1
#define CTX_FORMATTER 1
#define CTX_PROTOCOL_U8_COLOR 1
#define CTX_LIMIT_FORMATS 1
#define CTX_32BIT_SEGMENTS 0
#define CTX_RASTERIZER 1
#define CTX_RASTERIZER_AA 3
#if 0
#define CTX_ENABLE_GRAY1 1
#define CTX_ENABLE_GRAY2 1
#define CTX_ENABLE_GRAY4 1
#define CTX_ENABLE_GRAY8 1
#endif
#define CTX_ENABLE_RGB332 0
#define CTX_ENABLE_RGB565 1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_COMPOSITING_GROUPS 0
#define CTX_ALWAYS_USE_NEAREST_FOR_SCALE1 1
#define CTX_EVENTS 1
#define CTX_FORCE_INLINES 0
#define CTX_THREADS 0
#define CTX_TILED 0
#define CTX_BAREMETAL 1
#define CTX_ONE_FONT_ENGINE 1
#define CTX_MAX_FONTS 3

#define CTX_MAX_SCANLINE_LENGTH 480
#define CTX_MAX_FRAMEBUFFER_WIDTH CTX_MAX_SCANLINE_LENGTH
#define CTX_MAX_JOURNAL_SIZE (1024 * 32)
// is also max and limits complexity
// of paths that can be filled
#define CTX_MIN_EDGE_LIST_SIZE 512

#define CTX_MAX_DASHES 32
#define CTX_MAX_GRADIENT_STOPS 10
#define CTX_MAX_STATES 16
#define CTX_MAX_EDGES 127
#define CTX_MAX_PENDING 64
#define CTX_VT_STYLE_SIZE 32

#define CTX_GRADIENT_CACHE_ELEMENTS 128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 64
#define CTX_MAX_KEYDB 16
#define CTX_MAX_TEXTURES 16
#define CTX_PARSER_MAXLEN 512
#define CTX_PARSER_FIXED_TEMP 1
#define CTX_STRINGPOOL_SIZE 256
#define CTX_MAX_DEVICES 1
#define CTX_MAX_KEYBINDINGS 24
#define CTX_MAX_CBS 32 // max defined interaction listeners
#define CTX_MAX_LISTEN_FDS 1
#define CTX_TERMINAL_EVENTS 0
#define CTX_FRAGMENT_SPECIALIZE 1      // more optimize texture|gradients
#define CTX_BLENDING_AND_COMPOSITING 0 // only support normal/over/copy
#define CTX_GSTATE_PROTECT 1
#define CTX_COMPOSITE_O2 1
// #define CTX_RASTERIZER_O2                  1
#define CTX_NATIVE_GRAYA8 0
#define CTX_AVOID_CLIPPED_SUBDIVISION 0

#include <stdint.h>

#define _CTX_INTERNAL_FONT_

#include "Arimo-Regular.h"
#include "Cousine-Regular.h"
#define CTX_STATIC_FONT(font)                                                  \
  ctx_load_font_ctx(ctx_font_##font##_name, ctx_font_##font,                   \
                    sizeof(ctx_font_##font))

#define CTX_FONT_1 CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_2 CTX_STATIC_FONT(Cousine_Regular)

#define CTX_IMPLEMENTATION
#include "ctx.h"

#include "driver/i2c.h"
#include "esp_lcd_touch_cst816s.h"
#include "gc9a01.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"

static uint8_t scratch[SCRATCH_BUF_BYTES];
static esp_lcd_touch_handle_t tp = NULL;

static void touch_init() {
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

  esp_lcd_panel_io_i2c_config_t tp_io_config =
      ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
  // ESP_LOGI(TAG, "Initialize touch IO (I2C)");
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0,
                                           &tp_io_config, &tp_io_handle));

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = DISPLAY_WIDTH,
      .y_max = DISPLAY_HEIGHT,
      .rst_gpio_num = 1,
      .int_gpio_num = 0,
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
  };

  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
}

static int frame_done_ctx(Ctx *ctx, void *user_data) {
  esp_lcd_touch_read_data(tp);
  static uint16_t touch_x[1];
  static uint16_t touch_y[1];
  static uint16_t touch_strength[1];
  uint8_t touch_cnt = 0;
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(
      tp, touch_x, touch_y, touch_strength, &touch_cnt, 1);
  float x = touch_x[0];
  float y = touch_y[0];
  x -= 10;
  y -= 10;
  if (y < 0)
    y = 0;
  if (x < 0)
    x = 0;
  y *= 1.07;
  x *= 1.05;
  if (y > DISPLAY_HEIGHT)
    y = DISPLAY_HEIGHT;
  if (x > DISPLAY_WIDTH)
    x = DISPLAY_WIDTH;

  static bool was_pressed = false;
  if (touchpad_pressed) {
    if (!was_pressed)
      ctx_pointer_press(ctx, x, y, 0, 0);
    else
      ctx_pointer_motion(ctx, x, y, 0, 0);
  } else if (was_pressed) {
    ctx_pointer_release(ctx, x, y, 0, 0);
  }
  was_pressed = touchpad_pressed;

  vTaskDelay(1);

  return 0;
}

void esp_backlight(int percent) {
  if (percent < 7)
    percent = 7;
  GC9A01_SetBL(percent);
}

static void lcd_init(void) {
  GC9A01_Init();
  esp_backlight(30);
}

void ctx_set_pixels(Ctx *ctx, void *user_data, int x, int y, int w, int h,
                    void *buf) {
  uint8_t *pixels = (uint8_t *)buf;
  GC9A01_SetWindow(x, y, x + w - 1, y + h - 1);
  lcd_data(pixels, w * h * 2);
}

#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "esp_vfs_usb_serial_jtag.h"

void usb_serial_jtag_init(void) {
  /* Disable buffering on stdin */

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  /* Enable non-blocking mode on stdin and stdout */
  usb_serial_jtag_driver_config_t usb_serial_jtag_config =
      USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

  esp_err_t ret = ESP_OK;
  /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
  ret = usb_serial_jtag_driver_install(&usb_serial_jtag_config);
  if (ret != ESP_OK) {
    printf("console eek!\n");
    return;
  }

  esp_vfs_usb_serial_jtag_use_driver();
#if 0
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);
#endif
}

void flash_init(void)
{   
    // Handle of the wear levelling library instance
    static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount("/flash", "vfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    }
}


Ctx *ctx_host(void) {
  static Ctx *ctx = NULL;
  if (ctx)
    return ctx;

  //  uart_init();
  flash_init();
  usb_serial_jtag_init();
  lcd_init();
  touch_init();
  ctx = ctx_new_cb(DISPLAY_WIDTH, DISPLAY_HEIGHT, CTX_FORMAT_RGB565_BYTESWAPPED,
                   ctx_set_pixels, NULL, frame_done_ctx, NULL, sizeof(scratch),
                   scratch,
                   // CTX_FLAG_HASH_CACHE|CTX_FLAG_LOWFI
                   CTX_FLAG_HASH_CACHE | CTX_FLAG_KEEP_DATA);

  return ctx;
}

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define EXAMPLE_ESP_MAXIMUM_RETRY 4

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

#undef printf

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    // ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    printf("got ip: " IPSTR " \n", IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

static void nvs_init(void) {
  static int inited = 0;
  if (!inited) {
    inited = 1;
    s_wifi_event_group = xEventGroupCreate();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
  }
}

#define DEFAULT_SCAN_LIST_SIZE 16

static esp_netif_t *sta_netif = NULL;
char **wifi_scan(void) {
  nvs_init();

  if (!sta_netif) {
    sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  }
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  int totlen = 0;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_scan_start(NULL, true);
  ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u",
           ap_count, number);

  for (int i = 0; i < number; i++) {
    ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
    ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
    // print_auth_mode(ap_info[i].authmode);
#if 0
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
#endif
    ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
    totlen += strlen((char *)ap_info[i].ssid) + 1;
  }
  int head_len = sizeof(void *) * (number + 1);

  char **ret = calloc(head_len + totlen, 1);
  int pos = head_len;
  for (int i = 0; i < number; i++) {
    ret[i] = (((char *)ret) + pos);
    strcpy(ret[i], (char *)ap_info[i].ssid);
    pos += strlen((char *)ap_info[i].ssid) + 1;
  }
  return ret;
}

int wifi_init_sta(const char *ssid_arg, const char *password_arg) {
  nvs_init();

  if (!sta_netif) {
    sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  }

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  strncpy((char *)&wifi_config.sta.ssid[0], ssid_arg,
          sizeof(wifi_config.sta.ssid) - 1);
  strncpy((char *)&wifi_config.sta.password[0], password_arg,
          sizeof(wifi_config.sta.password) - 1);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid_arg,
             password_arg);
    return 0;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ssid_arg,
             password_arg);
    return -1;
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return -1;
  }
}
