#pragma GCC optimize("jump-tables,tree-switch-conversion")

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCRATCH_BUF_BYTES (240*240*2)

#define CTX_DITHER                         1
#define CTX_PROTOCOL_U8_COLOR              1
#define CTX_LIMIT_FORMATS                  0
#define CTX_32BIT_SEGMENTS                 0
#define CTX_RASTERIZER                     1
#define CTX_RASTERIZER_AA                  3
#define CTX_ENABLE_GRAY1                   1
#define CTX_ENABLE_GRAY2                   1
#define CTX_ENABLE_GRAY4                   1
#define CTX_ENABLE_GRAY8                   1
#define CTX_ENABLE_GRAYA8                  1
#define CTX_ENABLE_RGB8                    1
#define CTX_ENABLE_RGBA8                   1
#define CTX_ENABLE_BGRA8                   1
//#define CTX_GET_CONTENTS                   1
#define CTX_ENABLE_RGB332                  1
#define CTX_ENABLE_RGB565                  1
#define CTX_ENABLE_RGB565_BYTESWAPPED      1
#define CTX_COMPOSITING_GROUPS             0
#define CTX_ENABLE_CM                      0
#define CTX_ALWAYS_USE_NEAREST_FOR_SCALE1  1
#define CTX_EVENTS                         1
#define CTX_FORCE_INLINES                  0
#define CTX_RAW_KB_EVENTS                  0
#define CTX_THREADS                        0
#define CTX_TILED                          0
#define CTX_VT                             1
#define CTX_PTY                            0
#define CTX_PARSER                         1
#define CTX_BAREMETAL                      1
#define CTX_ONE_FONT_ENGINE                1
#define CTX_ESP                            1
#define CTX_MAX_SCANLINE_LENGTH            480
#define CTX_MAX_FRAMEBUFFER_WIDTH CTX_MAX_SCANLINE_LENGTH
#define CTX_MAX_JOURNAL_SIZE               (1024*512)
// is also max and limits complexity
// of paths that can be filled
#define CTX_MIN_EDGE_LIST_SIZE             512

#define CTX_HASH_COLS                      5
#define CTX_HASH_ROWS                      5

#define CTX_MAX_DASHES                     32
#define CTX_MAX_GRADIENT_STOPS             10
#define CTX_MAX_STATES                     10
#define CTX_MAX_EDGES                      127
#define CTX_MAX_PENDING                    64
#define CTX_FORMATTER                      0
#define CTX_GRADIENT_CACHE_ELEMENTS        128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 64
#define CTX_MAX_KEYDB                      16
#define CTX_MAX_TEXTURES                   16
#define CTX_PARSER_MAXLEN                  512
#define CTX_PARSER_FIXED_TEMP              1
#define CTX_STRINGPOOL_SIZE                256
#define CTX_MAX_DEVICES                    1
#define CTX_MAX_KEYBINDINGS                32
#define CTX_MAX_CBS                        8
#define CTX_MAX_LISTEN_FDS                 1
#define CTX_TERMINAL_EVENTS                0
#define CTX_FRAGMENT_SPECIALIZE            1
#define CTX_GSTATE_PROTECT                 1
#define CTX_COMPOSITE_O2                   1
#define CTX_RASTERIZER_SWITCH_DISPATCH     0
#define CTX_NATIVE_GRAYA8                  0
#define CTX_AVOID_CLIPPED_SUBDIVISION      0
#define CTX_AUDIO                          1
#define CTX_AUDIO_HOST                     1
#define CTX_STB_IMAGE                   1
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_JPEG


#include <stdint.h>
#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))

#include "Arimo-Regular.h"
#include "Cousine-Regular.h"
#define CTX_FONT_1   CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_2   CTX_STATIC_FONT(Cousine_Regular)

#define CTX_IMPLEMENTATION
#include "ctx.h"

#include "gc9a01.h"
#include "driver/i2c.h"
#include "flow3r_bsp.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"

#include "nvs_flash.h"
#include "flow3r_bsp_captouch.h"


static const char *TAG = "ctx";


#define CT_DOWN_THRESHOLD     3000


static uint8_t scratch[SCRATCH_BUF_BYTES];

typedef struct petal_info_t
{
  uint16_t ccw;
  uint16_t cw;
  uint16_t base;
  uint16_t tip; /* (cw + ccw)/2  - or actual tip  */


  unsigned int sum;
  uint16_t pos_radial;   /* base = 0    tip = 65535  */
  int16_t  pos_angular;  /* ccw = 0     cw  = 65535  */
} petal_info_t;

static petal_info_t _captouch[10];

bool bsp_captouch_down (int petal)
{
  return _captouch[petal].sum > CT_DOWN_THRESHOLD;
}
int16_t  bsp_captouch_angular (int petal)
{
  if (bsp_captouch_down (petal))
    return _captouch[petal].pos_angular;
  return 0;
}
uint16_t bsp_captouch_radial  (int petal)
{
  if (bsp_captouch_down (petal))
    return _captouch[petal].pos_radial;
  return 65536/2;
}
static bool ct_screen_touched = false;
bool bsp_captouch_screen_touched (void)
{
  return ct_screen_touched;
}

static float ct_angle = -1000.0f;
static float ct_quant_angle = -1000.0f;
static float ct_radial = 0.0f;

static float ct_angle_i = -1000.0f;
static float ct_angle_o = -1000.0f;

static uint16_t ct_petal_mask = (1<<10)-1;

// return < 0 if no touch on upper petals,
// otherwise a value that is 0 for top going to
// 1.0 for full rotation.
//
// if quantize is non-0 quantize to given number of levels,
// there is currently special handling for 5, 10 and 20 steps.
//
float bsp_captouch_angle (float *radial_pos, int quantize, uint16_t petal_mask)
{
  if (petal_mask == 0)
    ct_petal_mask = (1<<10)-1;
  else
    ct_petal_mask = petal_mask;

  if (radial_pos) *radial_pos = ct_radial;
  if (ct_angle < -100.0) return -1000.0f;

  switch (quantize)
  {
    case -1: return ct_angle_i;
    case -2: return ct_angle_o;
    case 20:
    case 10:
    case 5:
    case 4:
    case 2:
      return floorf(ct_quant_angle*quantize+0.5f)/quantize;
    case 0:
    case 1:
      return ct_angle;
    // TODO: make better and specific attempt at 12 and thus 6
    default:
      return ((int)(ct_angle*quantize+0.5f))/(quantize * 1.0f);
  }
  return ct_angle;
}

int first_repeat_time      = 1000 * 800;
int subsequent_repeat_time = 1000 * 200;

static bool petal_was_down[10]={false,};
static unsigned long petal_down_time[10] = {0,};    
static unsigned int petal_repeats[10]    = {0,};

int  flow3r_synthesize_key_events = 2;
void     bsp_captouch_key_events (int level)
{
  flow3r_synthesize_key_events = level;
}

static int frame_done_ctx (Ctx *ctx, void *user_data)
{
    unsigned long ticks = ctx_ticks ();
    static flow3r_bsp_tripos_state_t _app_button_state;
    static flow3r_bsp_tripos_state_t _os_button_state;
    static unsigned long app_down_time = 0;    
    static unsigned int app_repeats    = 0;
    static unsigned long os_down_time  = 0;
    static unsigned int os_repeats     = 0;

    const int debounce_time          = subsequent_repeat_time;

    flow3r_bsp_spio_update();
    flow3r_bsp_tripos_state_t app_button_state =
    flow3r_bsp_spio_left_button_get();

#if 0
    printf ("%.3f %.3f    ", _captouch[0].pos_radial/65535.0f, _captouch[0].pos_angular/65535.0-0.5f);
    printf ("\n");
#endif
   
    if (flow3r_synthesize_key_events)
    for (int i = 0; i < 10; i++)
    {
      bool down = _captouch[i].sum > CT_DOWN_THRESHOLD * 1.5f;
      if ((down != petal_was_down[i]) ||
         (((petal_repeats[i] == 1) && ((ticks-petal_down_time[i]) > first_repeat_time)) ||
         ((petal_repeats[i] >  1) && ((ticks-petal_down_time[i]) > subsequent_repeat_time))))
      {
        if (down)
        {
          if (!petal_was_down[i])
            petal_repeats[i] = 1;
          else
            petal_repeats[i]++;
          // pressed
          switch (i)
          {
            case 9:
              ctx_key_press (ctx, 0, "backspace", 0);
              break;
            case 7:
              ctx_key_press (ctx, 0, "left", 0);
              break;
            case 1:
              ctx_key_press (ctx, 0, "return", 0);
              break;
            case 3:
              ctx_key_press (ctx, 0, "right", 0);
              break;
            case 0:
              ctx_key_press (ctx, 0, "shift-space", 0);
              break;
            case 6:
              if (flow3r_synthesize_key_events>1)
                ctx_key_press (ctx, 0, "down", 0);
              break;
            case 8:
              if (flow3r_synthesize_key_events>1)
                ctx_key_press (ctx, 0, "up", 0);
              break;
            case 5:
              ctx_key_press (ctx, 0, "space", 0);
              break;
          }
          petal_down_time[i] = ticks;
        }
        else
        {
          // released
        }
        petal_was_down[i] = down;
      }
    }
    if ((_app_button_state != app_button_state) ||
         ((app_repeats == 1) && ((ticks-app_down_time) > first_repeat_time)) ||
         ((app_repeats >  1) && ((ticks-app_down_time) > subsequent_repeat_time))
       )
    {
      switch (app_button_state)
      {
        case flow3r_bsp_tripos_none: // button release
          if (ticks-app_down_time < debounce_time) // debounce...
            return 0;
          break;
        case flow3r_bsp_tripos_left:
          ctx_key_press (ctx, 0, "left", 0);
          break;
        case flow3r_bsp_tripos_right:
          ctx_key_press (ctx, 0, "right", 0);
          break;
        case flow3r_bsp_tripos_mid:
          ctx_key_press (ctx, 0, "return", 0);
          break;
      }
      if (_app_button_state != app_button_state)
        app_repeats = 0;
      app_down_time = ticks;
      app_repeats++;
      _app_button_state = app_button_state;
    }

    flow3r_bsp_tripos_state_t os_button_state =
     flow3r_bsp_spio_right_button_get();
    if ((_os_button_state != os_button_state) ||
         ((os_repeats =  1) && ((ticks-os_down_time) > first_repeat_time)) ||
         ((os_repeats >  1) && ((ticks-os_down_time) > subsequent_repeat_time))
       )
    {
      switch (os_button_state)
      {
        case flow3r_bsp_tripos_none:
          if (ticks-os_down_time < debounce_time) // debounce...
            return 0;
          break;
        case flow3r_bsp_tripos_left:
          ctx_key_press (ctx, 0, "page-up", 0);
          break;
        case flow3r_bsp_tripos_right:
          ctx_key_press (ctx, 0, "page-down", 0);
          break;
        case flow3r_bsp_tripos_mid:
          ctx_key_press (ctx, 0, "escape", 0);
          break;
      }
      if (_os_button_state != os_button_state)
        os_repeats = 0;
      os_down_time = ticks;
      os_repeats++;
      _os_button_state = os_button_state;
    }

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
  esp_backlight(100);
}

void ctx_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{   
    uint8_t *pixels = (uint8_t*)buf; 
    GC9A01_SetWindow(x,y,x+w-1,y+h-1);
    lcd_data(pixels,w*h*2);
}

static SemaphoreHandle_t ct_mutex = NULL; 
static void on_captouch_data(const flow3r_bsp_captouch_state_t *st);

#if 0
static void uart_init(void)
{
  int uart_num = 0;
setvbuf(stdin, NULL, _IONBF, 0);
setvbuf(stdout, NULL, _IONBF, 0);
ESP_ERROR_CHECK(uart_driver_install((uart_port_t)uart_num,
						256, 0, 0, NULL, 0));
esp_vfs_dev_uart_port_set_rx_line_endings(0, ESP_LINE_ENDINGS_CR);
esp_vfs_dev_uart_port_set_tx_line_endings(0, ESP_LINE_ENDINGS_CRLF);
esp_vfs_dev_uart_use_driver(uart_num);
}

#endif

void usb_serial_jtag_init(void)
{
  /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  /* Enable non-blocking mode on stdin and stdout */
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

  usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

  esp_err_t ret = ESP_OK;
  /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
  ret = usb_serial_jtag_driver_install(&usb_serial_jtag_config);
  if (ret != ESP_OK) {
    printf ("console eek!\n");
    return;
  }

  esp_vfs_usb_serial_jtag_use_driver();
}

void sd_init (void);
void board_init (void)
{
  flow3r_bsp_i2c_init();
  flow3r_bsp_spio_init();
  lcd_init();
  //touch_init();
}

Ctx *ctx_host(void)
{
  static Ctx *ctx = NULL;
  if (ctx)
    return ctx;

  board_init();
  ctx = ctx_new_cb(DISPLAY_WIDTH, DISPLAY_HEIGHT, CTX_FORMAT_RGB565_BYTESWAPPED,
                   ctx_set_pixels,
                   NULL,
                   frame_done_ctx,
                   NULL, 
 //                sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE|CTX_FLAG_LOWFI);
                   sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE|CTX_FLAG_KEEP_DATA);

  ctx_start_frame (ctx);
  ctx_rectangle (ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
  ctx_gray(ctx,0);
  ctx_fill (ctx);
  ctx_logo (ctx, ctx_width(ctx)/2, ctx_height(ctx)/2, ctx_height(ctx) * 0.7);
  ctx_end_frame (ctx);
  ct_mutex = xSemaphoreCreateMutex();

  esp_err_t ret = flow3r_bsp_captouch_init (on_captouch_data);
  if (ret != ESP_OK) ESP_LOGE(TAG, "captouch init failed\n");

  sd_init();
  usb_serial_jtag_init();
  
  return ctx;
}

void vPortCleanUpTCB ( void *pxTCB )
{
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

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static int s_retry_num = 0;


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        //ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        printf("got ip: " IPSTR " \n", IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static
void nvs_init (void)
{
  static int inited = 0;
  if (!inited) { inited = 1;
    s_wifi_event_group = xEventGroupCreate();

  //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
   }
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
  }
}

#define DEFAULT_SCAN_LIST_SIZE 16

static esp_netif_t *sta_netif = NULL;
char **wifi_scan(void)
{
  nvs_init ();


  if (!sta_netif)
  {
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
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        //print_auth_mode(ap_info[i].authmode);
#if 0
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
#endif
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
        totlen += strlen ((char*)ap_info[i].ssid) + 1;
    }
    int head_len = sizeof(void*)*(number+1);

    char **ret = calloc (head_len + totlen, 1);
    int pos = head_len;
    for (int i = 0; i < number; i++) {
       ret[i] = (((char*)ret)+pos);
       strcpy(ret[i], (char*)ap_info[i].ssid);
       pos += strlen ((char*)ap_info[i].ssid)+1;
    }
    return ret;
}

int wifi_init_sta(const char *ssid_arg, const char *password_arg)
{
  nvs_init ();

  if (!sta_netif)
  {
    sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = { .threshold.authmode = WIFI_AUTH_WPA2_PSK, },
    };
   strncpy((char*)&wifi_config.sta.ssid[0], ssid_arg, sizeof (wifi_config.sta.ssid)-1);
   strncpy((char*)&wifi_config.sta.password[0], password_arg, sizeof (wifi_config.sta.password)-1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_stop() );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid_arg, password_arg);
        return 0;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid_arg, password_arg);
        return -1;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return -1;
    }
}


#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define MOUNT_POINT "/sd"

void sd_init (void)
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 1;

    slot_config.clk = GPIO_NUM_47;
    slot_config.cmd = GPIO_NUM_48;
    slot_config.d0 = GPIO_NUM_21;

    slot_config.flags |= SDMMC_SLOT_NO_CD | SDMMC_SLOT_NO_WP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);

    if (ret != ESP_OK) 
    {
       return;
    }

  return;
}

#define CT_OUTHER_EXPONENT    1.0f
#define CT_INNER_EXPONENT     1.0f

static void on_captouch_data(const flow3r_bsp_captouch_state_t *st)
{
  static int calibrated =0;
  xSemaphoreTake (ct_mutex, portMAX_DELAY);
  if (!calibrated)
  {
    flow3r_bsp_captouch_calibrate ();
    calibrated = 1;
  }

  for (int i = 0; i < 10; i++)
  {
     if ((i % 2) == 0)
     {
        _captouch[i].cw   = st->petals[i].cw.raw;
        _captouch[i].ccw  = st->petals[i].ccw.raw;
        _captouch[i].base = st->petals[i].base.raw;
        _captouch[i].tip  = (_captouch[i].cw + (uint32_t)_captouch[i].ccw)/2;
     }
     else
     {
        _captouch[i].tip  = st->petals[i].tip.raw;
        _captouch[i].base = st->petals[i].base.raw;
     }
  }
  xSemaphoreGive (ct_mutex);

  uint16_t best_sum = 0;
  int best_angular = 32767;
  int best_petal = -1;

  for (int i = 0; i < 10; i++)
  {
     float pos_angular = 0;
     int   pos_radial  = 32767;

     if ((i % 2) == 0)
     {
        float angular_sum = (_captouch[i].cw + _captouch[i].ccw);
        
        if (angular_sum == 0)
          pos_angular = 0.0f;
        else if (_captouch[i].cw < _captouch[i].ccw)
        {
          pos_angular = -((0.5f - _captouch[i].cw / angular_sum ))*2;
        }
        else
        {
          pos_angular = ((0.5f - _captouch[i].ccw / angular_sum))*2;
        }
        pos_angular *= 32767;

        if (pos_angular < -32767) pos_angular = -32767;
        else if (pos_angular > 32767) pos_angular = 32767;
     }

     uint32_t radial_sum = (_captouch[i].tip + (uint32_t)_captouch[i].base)/2;
     if (radial_sum == 0)
       pos_radial = 32767;
     else if (_captouch[i].tip < _captouch[i].base)
     {
       pos_radial = (65535*_captouch[i].tip) / (radial_sum*2);
     }
     else
     {
       pos_radial = 65535-((65535*_captouch[i].base) / (radial_sum*2));
     }

     if (((i % 2) == 0))
     {
       if (radial_sum > best_sum && ((ct_petal_mask & (1<<i))!=0))
       {
         best_sum = radial_sum;
         best_angular = pos_angular;
         best_petal = i;
       }
     }
     if (radial_sum < CT_DOWN_THRESHOLD)
     {
        pos_angular = 0;
        pos_radial = 32767;
     }
     _captouch[i].sum         = radial_sum;
     _captouch[i].pos_angular = pos_angular;
     _captouch[i].pos_radial  = pos_radial;
  }


  if (best_sum < CT_DOWN_THRESHOLD)
  {
    ct_radial = 0;
    ct_angle =
    ct_angle_o =
    ct_angle_i = 
    ct_quant_angle = -1000.0f;;
  }
  else
  {
    float rel_angle = (best_angular/32767.0f);

    float radial = _captouch[best_petal].pos_radial/65535.0f;

    int prev_petal = (best_petal + 10 - 2) % 10;
    int next_petal = (best_petal + 2) % 10;

    float this_sum = _captouch[best_petal].sum;

    {
      float new_rel_angle;


      ct_angle_o = best_petal + rel_angle;
      if (rel_angle < -1.0f) rel_angle = -1.0f;
      if (rel_angle > 1.0f) rel_angle = 1.0f;
      if (ct_angle_o < 0) ct_angle_o += 10;
      ct_angle_o /= 10.0f;

      if (rel_angle < -0.0f)
      {
        float prev_sum = _captouch[prev_petal].sum;
        new_rel_angle = -(2*(prev_sum / ((prev_sum+this_sum))));

      } 
      else
      {
        float next_sum = _captouch[next_petal].sum;
        new_rel_angle = (2*(next_sum / ((next_sum+this_sum))));
      }

      ct_angle_i = best_petal + new_rel_angle;

      if (new_rel_angle < -1.0f) new_rel_angle = -1.0f;
      if (new_rel_angle > 1.0f) new_rel_angle = 1.0f;
      if (ct_angle_i < 0) ct_angle_i += 10;
      ct_angle_i /= 10.0f;

#if 0
      float factor = (1.0f-(radial / CT_INNER_RADIUS)) * (1.0f-fabs(rel_angle));
      if (factor < 0.0f) factor = 0.0f;
      if (factor > 1.0f) factor = 1.0f;
#endif
      float factor = 0.5f;
      rel_angle = rel_angle * (1.0f-factor) + new_rel_angle * factor;
    }

    if (rel_angle < -1.0f) rel_angle = -1.0f;
    if (rel_angle > 1.0f) rel_angle = 1.0f;

    if (fabsf(rel_angle) > 0.7f)
       radial *= 1.5;
    else if (fabsf(rel_angle) < 0.2f)
       radial *= 1.5;

    float angle = best_petal + rel_angle;
    if (angle < 0) angle += 10;
    angle /= 10.0f;
       
    ct_angle  = angle;
    ct_radial = radial;

    float threshold = 0.125;
    if      (rel_angle < -0.5-threshold) rel_angle = -1.0f;
    else if (rel_angle < -0.5+threshold) rel_angle = -0.5f;
    else if (rel_angle <  0.5-threshold) rel_angle =  0.0f;
    else if (rel_angle <  0.5+threshold) rel_angle =  0.5f;
    else rel_angle = 1.0f;

    angle = best_petal + rel_angle;
    if (angle < 0) angle += 10;
    angle /= 10.0f;
       
    ct_quant_angle  = angle;
  }
}

#if CTX_AUDIO_HOST

#define AUDIO_CHUNK_SIZE   128

static uint16_t outbuf[AUDIO_CHUNK_SIZE*2];

static void audio_task (void *data)
{
  while (true)
  {
     int client_channels = ctx_pcm_channels (ctx_client_format);
     bool is_float = (ctx_client_format == CTX_F32 || ctx_client_format == CTX_F32S);

     int c = AUDIO_CHUNK_SIZE;
     uint16_t left = 0, right = 0;
     int i = 0;
     for (i =0; i < c && ctx_pcm_cur_left; i++)
     {
       uint32_t *packet_size_p = (ctx_pcm_list->data);
       uint32_t packet_size = *packet_size_p;

       if (is_float)
       {
          float *packet = (ctx_pcm_list->data);
          packet += 4;
          packet += (packet_size - ctx_pcm_cur_left) * client_channels;
          left = right = packet[0] * (1<<15);
          if (client_channels > 1)
            right = packet[0] * (1<<15);
       }
       else
       {
          uint16_t *packet = (ctx_pcm_list->data);
          packet += 8;
          packet += (packet_size - ctx_pcm_cur_left) * client_channels;
          left = right = packet[0];
          if (client_channels > 1)
            right = packet[1];
       }
       outbuf[i * 2] = left;
       outbuf[i * 2 + 1] = right;
       ctx_pcm_cur_left --;
       ctx_pcm_queued --;
       if (ctx_pcm_cur_left == 0)
       {
            void *old = ctx_pcm_list->data;
            pthread_mutex_lock (&ctx_audio_mutex);

            ctx_list_remove (&ctx_pcm_list, old);
            pthread_mutex_unlock (&ctx_audio_mutex);
            ctx_free (old);
            ctx_pcm_cur_left = 0;
            if (ctx_pcm_list)
            {
              uint32_t *packet_sizep = (ctx_pcm_list->data);
              uint32_t packet_size = *packet_sizep;
              ctx_pcm_cur_left = packet_size;
            }
       }
     }
      for (;i < c; i ++)
      {
         /* slight click protection in case we were not left at dc */
         outbuf[i * 2 + 0] = (left /= 2);
         outbuf[i * 2 + 1] = (right /= 2);
      }
     
     size_t bytes_written;
     flow3r_bsp_audio_write(outbuf, AUDIO_CHUNK_SIZE*4, &bytes_written, 1000);
     vTaskDelay(0);
  }
}

int ctx_host_audio_init (int hz, CtxPCM format)
{
  flow3r_bsp_audio_init();
  xTaskCreate(&audio_task, "audio", 2048, NULL, 6, NULL);

  flow3r_bsp_audio_speaker_set_volume(false, -10.0);
  return 0;
}

#endif

//#include "symbols.inc"
