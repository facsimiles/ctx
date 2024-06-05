// Driver for WS2812-style LEDs, using RMT peripheral.
//
// Based on espressif__led_strip.
//
// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "flow3r_bsp_rmtled.h"

static const char *TAG = "flow3r-rmtled";

#include "driver/rmt.h"
#include "esp_log.h"

#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (1000)
#define WS2812_T1H_NS (1000)
#define WS2812_T1L_NS (350)
#define WS2812_RESET_US (280)

typedef struct {
    uint16_t num_leds;
    rmt_channel_t chan;
    uint8_t *buffer;
    bool transmitting;

    uint32_t ws2812_t0h_ticks;
    uint32_t ws2812_t0l_ticks;
    uint32_t ws2812_t1h_ticks;
    uint32_t ws2812_t1l_ticks;
} flow3r_bsp_rmtled_t;

static flow3r_bsp_rmtled_t rmtled = { 0 };

static void IRAM_ATTR _rmtled_adapter(const void *src, rmt_item32_t *dest,
                                      size_t src_size, size_t wanted_num,
                                      size_t *translated_size,
                                      size_t *item_num) {
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {
        { { rmtled.ws2812_t0h_ticks, 1, rmtled.ws2812_t0l_ticks, 0 } }
    };  // Logical 0
    const rmt_item32_t bit1 = {
        { { rmtled.ws2812_t1h_ticks, 1, rmtled.ws2812_t1l_ticks, 0 } }
    };  // Logical 1
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val = bit1.val;
            } else {
                pdest->val = bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

esp_err_t flow3r_bsp_rmtled_init(uint16_t num_leds) {
    if (rmtled.buffer != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(14, 0);
    // set counter clock to 40MHz
    config.clk_div = 2;

    esp_err_t ret;
    if ((ret = rmt_config(&config)) != ESP_OK) {
        ESP_LOGE(TAG, "rmt_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if ((ret = rmt_driver_install(
             config.channel, 0, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED)) !=
        ESP_OK) {
        ESP_LOGE(TAG, "rmt_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    uint32_t counter_clk_hz = 0;
    if ((ret = rmt_get_counter_clock(config.channel, &counter_clk_hz)) !=
        ESP_OK) {
        ESP_LOGE(TAG, "rmt_get_counter_clock failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // ns -> ticks
    float ratio = (float)counter_clk_hz / 1e9;
    rmtled.ws2812_t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    rmtled.ws2812_t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    rmtled.ws2812_t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    rmtled.ws2812_t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);
    rmtled.num_leds = num_leds;
    rmtled.chan = config.channel;
    rmtled.buffer = calloc(3, num_leds);
    if (rmtled.buffer == NULL) {
        ESP_LOGE(TAG, "buffer allocation failed");
        return ESP_ERR_NO_MEM;
    }
    rmtled.transmitting = false;

    if ((ret = rmt_translator_init(config.channel, _rmtled_adapter)) !=
        ESP_OK) {
        ESP_LOGE(TAG, "rmt_translator_init: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void flow3r_bsp_rmtled_set_pixel(uint32_t index, uint32_t red, uint32_t green,
                                 uint32_t blue) {
    if (rmtled.buffer == NULL) {
        return;
    }
    if (index >= rmtled.num_leds) {
        return;
    }
    uint32_t start = index * 3;
    rmtled.buffer[start + 0] = green & 0xFF;
    rmtled.buffer[start + 1] = red & 0xFF;
    rmtled.buffer[start + 2] = blue & 0xFF;
}

esp_err_t flow3r_bsp_rmtled_refresh(int32_t timeout_ms) {
    if (rmtled.buffer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (rmtled.transmitting) {
        esp_err_t ret =
            rmt_wait_tx_done(rmtled.chan, pdMS_TO_TICKS(timeout_ms));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "rmt_wait_tx_done: %s", esp_err_to_name(ret));
        }
        rmtled.transmitting = false;
    }

    esp_err_t ret =
        rmt_write_sample(rmtled.chan, rmtled.buffer, rmtled.num_leds * 3, true);
    rmtled.transmitting = true;
    return ret;
}