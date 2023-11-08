#pragma GCC optimize("jump-tables,tree-switch-conversion")

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240




#define SCRATCH_BUF_BYTES (240*240*2)

#define CTX_DITHER                         1
#define CTX_PROTOCOL_U8_COLOR              1
#define CTX_LIMIT_FORMATS                  1
#define CTX_32BIT_SEGMENTS                 0
#define CTX_RASTERIZER                     1
#define CTX_RASTERIZER_AA                  5
#define CTX_ENABLE_GRAY1                   1
#define CTX_ENABLE_GRAY2                   1
#define CTX_ENABLE_GRAY4                   1
#define CTX_ENABLE_GRAY8                   1
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
#define CTX_PARSER                         0
#define CTX_FORMATTER                      0
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
#define CTX_NATIVE_GRAYA8                  0
#define CTX_AVOID_CLIPPED_SUBDIVISION      0

#include <stdint.h>
#include "Arimo-Regular.h"
#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))

#define CTX_FONT_1   CTX_STATIC_FONT(Arimo_Regular)

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

#include "nvs_flash.h"
#include "flow3r_bsp_captouch.h"


static const char *TAG = "ctx";


#define CT_DOWN_THRESHOLD 3000
#define CT_SMOOTHING 2   // 1 is disabled   2 is fast 3 is probably decent 
                         // what is too high for interaction, is it needed?


#define CT_RADIAL_EXPONENT    0.5f
#define CT_RADIAL_FACTOR      1.0f


#define CT_ANGULAR_EXPONENT   1.0f 
#define CT_ANGULAR_FACTOR     1.0f

#define CT_RADIAL_FLOOR       0.04f

// least tuned constants, last added:
#define CT_INNER_EXPONENT     0.333f

#define CT_INNER_RADIUS       0.3f
#define CT_INNER_MID_RADIUS   1.2f

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

static float ct_angle = -1.0f;
static float ct_radial = 0.0f;

// return < 0 if no touch on upper petals,
// otherwise a value that is 0 for top going to
// 1.0 for full rotation.
float bsp_captouch_angle (float *radial_pos)
{
   if (radial_pos) *radial_pos = ct_radial;
  return ct_angle;
}

int first_repeat_time      = 1000 * 800;
int subsequent_repeat_time = 1000 * 200;

static bool petal_was_down[10]={false,};
static unsigned long petal_down_time[10] = {0,};    
static unsigned int petal_repeats[10]    = {0,};

bool flow3r_synthesize_key_events = true;

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

static void set_pixels_ctx (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{   
    uint8_t *pixels = (uint8_t*)buf; 
    GC9A01_SetWindow(x,y,x+w-1,y+h-1);
    lcd_data(pixels,w*h*2);
}

static SemaphoreHandle_t mutex = NULL; // XXX
static void on_captouch_data(const flow3r_bsp_captouch_state_t *st)
{
  static int calibrated =0;
  xSemaphoreTake (mutex, portMAX_DELAY);
  if (!calibrated)
  {
    flow3r_bsp_captouch_calibrate ();
    calibrated = 1;
  }
  uint16_t best_sum = 0;
  float best_angular = 32767;
  int best_petal = -1;

  for (int i = 0; i < 10; i++)
  {
     if ((i % 2) == 0)
     {
#if CT_SMOOTHING==1
        _captouch[i].cw = st->petals[i].cw.raw;
        _captouch[i].ccw = st->petals[i].ccw.raw;
        _captouch[i].base = st->petals[i].base.raw;
#else
        _captouch[i].cw =
           (_captouch[i].cw * (CT_SMOOTHING-1) + st->petals[i].cw.raw) / CT_SMOOTHING;
        _captouch[i].ccw =
           (_captouch[i].ccw * (CT_SMOOTHING-1) + st->petals[i].ccw.raw) / CT_SMOOTHING;
        _captouch[i].base =
           (_captouch[i].base * (CT_SMOOTHING-1) + st->petals[i].base.raw) / CT_SMOOTHING;
#endif
        _captouch[i].tip = (_captouch[i].cw + (uint32_t)_captouch[i].ccw)/2;
     }
     else
     {
#if CT_SMOOTHING==1
        _captouch[i].tip  = st->petals[i].tip.raw;
        _captouch[i].base = st->petals[i].base.raw;
#else
        _captouch[i].tip =
           (_captouch[i].tip * (CT_SMOOTHING-1) + st->petals[i].tip.raw) / CT_SMOOTHING;
        _captouch[i].base =
           (_captouch[i].base * (CT_SMOOTHING-1) + st->petals[i].base.raw) / CT_SMOOTHING;
#endif

     }
  }
  xSemaphoreGive (mutex);


  for (int i = 0; i < 10; i++)
  {
     float pos_angular = 0.0f;
     float pos_radial  = 0.5f;

     if ((i % 2) == 0)
     {
        int prev_petal = (i - 2 + 10) % 10;
        int next_petal = (i + 2) % 10;
        float angular_sum = (_captouch[i].cw + (uint32_t)_captouch[i].ccw +
                             _captouch[prev_petal].sum+
                             _captouch[next_petal].sum);
        
        if (angular_sum == 0)
          pos_angular = 0.0f;
        else if (_captouch[i].cw + _captouch[next_petal].sum <
                 _captouch[i].ccw + _captouch[prev_petal].sum)
        {
          pos_angular = -powf(((0.5f - (_captouch[i].cw + _captouch[next_petal].sum) / angular_sum)*2), CT_ANGULAR_EXPONENT);
        }
        else
        {
          pos_angular = powf(((0.5f - (_captouch[i].ccw + _captouch[prev_petal].sum) / angular_sum)*2), CT_ANGULAR_EXPONENT);
        }
        pos_angular = (pos_angular) * 32767.0f;
        //(CT_ANGULAR_FACTOR * 32767.0f);
        //if (pos_angular < -32767) pos_angular = -32767;
        //else if (pos_angular > 32767) pos_angular = 32767;

     }

     float radial_sum = (_captouch[i].tip + (uint32_t)_captouch[i].base)/2;
     if (radial_sum == 0.0f)
       pos_radial = 0.5f;
     else if (_captouch[i].tip < _captouch[i].base)
     {
       pos_radial = (0.5f * powf( _captouch[i].tip / radial_sum, CT_RADIAL_EXPONENT));
     }
     else
     {
       pos_radial = 1.0f-(0.5f * powf(_captouch[i].base / radial_sum, CT_RADIAL_EXPONENT));
     }
     pos_radial -= CT_RADIAL_FLOOR;
     pos_radial -= 0.5f;
     pos_radial *= CT_RADIAL_FACTOR;
     pos_radial += 0.5f;
     if (pos_radial < 0.0f) pos_radial = 0.0f;
     if (pos_radial > 1.0f) pos_radial = 1.0f;

     if (((i % 2) == 0))
     {
       if (radial_sum > best_sum)
       {
         best_sum = radial_sum;
         best_angular = pos_angular;
         best_petal = i;
       }
     }
     if (radial_sum < CT_DOWN_THRESHOLD)
     {
        pos_angular = 0.0f;
        pos_radial = 0.5f;
     }
     _captouch[i].sum         = radial_sum;
     _captouch[i].pos_angular = pos_angular;
     _captouch[i].pos_radial  = pos_radial * 65535;
  }


  if (best_sum < CT_DOWN_THRESHOLD)
    ct_angle = -1.0f;
  else
  {
    float rel_angle = (best_angular/32767.0f);

    float radial = _captouch[best_petal].pos_radial/65535.0f;

    int prev_petal = (best_petal + 10 - 2) % 10;
    int next_petal = (best_petal + 2) % 10;


    float this_sum = _captouch[best_petal].sum;


    if (radial < CT_INNER_RADIUS)
    {
      float new_rel_angle;
      float dist = 0.0f;

      if (rel_angle < -1.0f) rel_angle = -1.0f;
      if (rel_angle > 1.0f) rel_angle = 1.0f;

      if (rel_angle < -0.0f)
      {
        float prev_sum = _captouch[prev_petal].sum;
        new_rel_angle = -powf(2*(prev_sum / ((prev_sum+this_sum))), CT_INNER_EXPONENT);

      } 
      else
      {
        float next_sum = _captouch[next_petal].sum;
        new_rel_angle = powf(2*(next_sum / ((next_sum+this_sum))), CT_INNER_EXPONENT);
      }



      dist = radial / CT_INNER_MID_RADIUS;

      float factor = 1.0f-dist;
      if (factor < 0.0f) factor = 0.0f;
      if (factor > 1.0f) factor = 1.0f;
      //factor = 1.0f;

      rel_angle = rel_angle * (1.0f-factor) + new_rel_angle * factor;


    }

    if (rel_angle < -1.0f) rel_angle = -1.0f;
    if (rel_angle > 1.0f) rel_angle = 1.0f;


    float angle = best_petal + rel_angle;
    angle = best_petal + rel_angle;
    if (angle < 0) angle += 10;
    angle /= 10.0f;
       
    ct_angle =  angle;
    ct_radial = radial;
  }
}


void sd_init (void);
void board_init (void)
{
  sd_init();
  
  flow3r_bsp_i2c_init();
  flow3r_bsp_spio_init();
  mutex = xSemaphoreCreateMutex();

  esp_err_t ret = flow3r_bsp_captouch_init (on_captouch_data);
  if (ret != ESP_OK) ESP_LOGE(TAG, "captouch init failed\n");

  lcd_init();
  //touch_init();
}

Ctx *esp_ctx(void)
{
  static Ctx *ctx = NULL;
  if (ctx) return ctx;
  board_init();
  ctx = ctx_new_cb(DISPLAY_WIDTH, DISPLAY_HEIGHT, CTX_FORMAT_RGB565_BYTESWAPPED,
                   set_pixels_ctx,
                   NULL,
                   frame_done_ctx,
                   NULL, 
 //                sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE|CTX_FLAG_LOWFI);
                   sizeof(scratch), scratch, CTX_FLAG_HASH_CACHE|CTX_FLAG_KEEP_DATA);

  
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
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

int wifi_init_sta(const char *ssid_arg, const char *password_arg)
{
  static int inited = 0;
  if (ssid_arg[0]==0) return -1;
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
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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
    }

    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
   strncpy((char*)&wifi_config.sta.ssid[0], ssid_arg, sizeof (wifi_config.sta.ssid)-1);
   strncpy((char*)&wifi_config.sta.password[0], password_arg, sizeof (wifi_config.sta.password)-1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
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


