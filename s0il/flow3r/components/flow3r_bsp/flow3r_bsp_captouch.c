#include "flow3r_bsp_captouch.h"
#include "flow3r_bsp_ad7147.h"
#include "flow3r_bsp_i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_log.h"

static const char *TAG = "flow3r-bsp-captouch";

typedef struct {
    size_t petal_number;
    petal_kind_t pad_kind;
} pad_mapping_t;

#if defined(CONFIG_FLOW3R_HW_GEN_P3)
static const pad_mapping_t _map_top[12] = {
    { 0, petal_pad_tip },  // 0
    { 0, petal_pad_ccw },  // 1
    { 0, petal_pad_cw },   // 2
    { 8, petal_pad_cw },   // 3
    { 8, petal_pad_ccw },  // 4
    { 8, petal_pad_tip },  // 5
    { 4, petal_pad_tip },  // 6
    { 4, petal_pad_ccw },  // 7
    { 4, petal_pad_cw },   // 8
    { 6, petal_pad_cw },   // 9
    { 6, petal_pad_ccw },  // 10
    { 6, petal_pad_tip },  // 11
};
static const pad_mapping_t _map_bot[13] = {
    { 9, petal_pad_base },  // 0
    { 9, petal_pad_tip },   // 1

    { 7, petal_pad_base },  // 2
    { 7, petal_pad_tip },   // 3

    { 5, petal_pad_base },  // 4
    { 5, petal_pad_tip },   // 5

    { 3, petal_pad_tip },   // 6
    { 3, petal_pad_base },  // 7

    { 1, petal_pad_tip },   // 8
    { 1, petal_pad_base },  // 9

    { 2, petal_pad_tip },  // 10
    { 2, petal_pad_cw },   // 11
    { 2, petal_pad_ccw },  // 12
};
static gpio_num_t _interrupt_gpio_top = GPIO_NUM_15;
static gpio_num_t _interrupt_gpio_bot = GPIO_NUM_15;
static bool _interrupt_shared = true;
#elif defined(CONFIG_FLOW3R_HW_GEN_P4) || defined(CONFIG_FLOW3R_HW_GEN_C23)
static const pad_mapping_t _map_top[12] = {
    { 0, petal_pad_ccw },   // 0
    { 0, petal_pad_base },  // 1
    { 0, petal_pad_cw },    // 2
    { 8, petal_pad_cw },    // 3
    { 8, petal_pad_base },  // 4
    { 8, petal_pad_ccw },   // 5
    { 4, petal_pad_ccw },   // 6
    { 4, petal_pad_base },  // 7
    { 4, petal_pad_cw },    // 8
    { 6, petal_pad_ccw },   // 9
    { 6, petal_pad_base },  // 10
    { 6, petal_pad_cw },    // 11
};
static const pad_mapping_t _map_bot[13] = {
    { 9, petal_pad_base },  // 0
    { 9, petal_pad_tip },   // 1

    { 7, petal_pad_base },  // 2
    { 7, petal_pad_tip },   // 3

    { 5, petal_pad_base },  // 4
    { 5, petal_pad_tip },   // 5

    { 3, petal_pad_tip },   // 6
    { 3, petal_pad_base },  // 7

    { 1, petal_pad_tip },   // 8
    { 1, petal_pad_base },  // 9

    { 2, petal_pad_ccw },   // 10
    { 2, petal_pad_cw },    // 11
    { 2, petal_pad_base },  // 12
};
#if defined(CONFIG_FLOW3R_HW_GEN_P4)
static gpio_num_t _interrupt_gpio_top = GPIO_NUM_15;
static gpio_num_t _interrupt_gpio_bot = GPIO_NUM_15;
static bool _interrupt_shared = true;
#else
static gpio_num_t _interrupt_gpio_top = GPIO_NUM_15;
static gpio_num_t _interrupt_gpio_bot = GPIO_NUM_16;
static bool _interrupt_shared = false;
#endif

#else
#error "captouch not implemented for this badge generation"
#endif

static ad7147_chip_t _top = {
    .name = "top",
    .is_bot = false,
    .nchannels = 12,
    .sequences = {
        {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        },
        {
            -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1,
        },
    },
};

static ad7147_chip_t _bot = {
    .name = "bot",
    .is_bot = true,
    .nchannels = 13,
    .sequences = {
        /// This is the ideal sequence we want. First, all the bottom sensors.
        /// Then the top petal.

        //{
        //     0,  1,  2,  3,  4,  5,
        //     6,  7,  8,  9, -1, -1,
        //},
        //{
        //    10, 11, 12, -1, -1, -1,
        //    -1, -1, -1, -1, -1, -1
        //},
        
        /// However, that causes extreme glitches. This seems to make it
        /// slightly better:

        //{
        //     0,  1,  2,  3,  4,  5,
        //     9, -1, -1, -1, -1, -1,
        //},
        //{
        //    10, 11, 12,  6,  7,  8,
        //     9, -1, -1, -1, -1, -1
        //},

        /// However again, that's still too glitchy for my taste. So we end up
        /// just ignoring one of the bottom petal pads and hope to figure this
        /// out later (tm).
        /// BUG(q3k): whyyyyyyyyyyyyyyy
        {
            0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12
        },
        {
            -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1,
        },
    },
};

static flow3r_bsp_captouch_callback_t _callback = NULL;

static flow3r_bsp_captouch_state_t _state = {};

static bool _processed = false;

static void _on_chip_data(void *user, uint16_t *data, size_t len) {
    ad7147_chip_t *chip = (ad7147_chip_t *)user;
    assert(chip == &_top || chip == &_bot);
    bool top = chip == &_top;
    const pad_mapping_t *map = top ? _map_top : _map_bot;

    for (size_t i = 0; i < len; i++) {
        flow3r_bsp_captouch_petal_state_t *petal =
            &_state.petals[map[i].petal_number];
        flow3r_bsp_captouch_petal_pad_state_t *pad =
            flow3r_bsp_captouch_pad_for_petal(petal, map[i].pad_kind);
        pad->raw = data[i];
    }

    _processed = true;
}

static QueueHandle_t _q = NULL;

static void _bot_isr(void *data) {
    (void)data;
    bool bot = true;
    xQueueSendFromISR(_q, &bot, NULL);
}

static void _top_isr(void *data) {
    (void)data;
    bool bot = false;
    xQueueSendFromISR(_q, &bot, NULL);
}

static void _kickstart(void) {
    bool bot = false;
    xQueueSend(_q, &bot, portMAX_DELAY);
    bot = true;
    xQueueSend(_q, &bot, portMAX_DELAY);
}

static void _task(void *data) {
    (void)data;

    bool top_ok = true;
    bool bot_ok = true;
    esp_err_t ret;
    for (;;) {
#if defined(CONFIG_FLOW3R_HW_GEN_P4)
        bool top = true, bot = true;
        vTaskDelay(10 / portTICK_PERIOD_MS);
#else
        bool bot = false;
        if (xQueueReceive(_q, &bot, portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Queue receive failed");
            return;
        }
        bool top = !bot;

        if (_interrupt_shared) {
            // No way to know which captouch chip triggered the interrupt, so
            // process both.
            top = true;
            bot = true;
        }
#endif
        if (bot) {
            if ((ret = flow3r_bsp_ad7147_chip_process(&_bot)) != ESP_OK) {
                if (bot_ok) {
                    ESP_LOGE(TAG,
                             "Bottom captouch processing failed: %s (silencing "
                             "future warnings)",
                             esp_err_to_name(ret));
                    bot_ok = false;
                }
            } else {
                bot_ok = true;
            }
        }
        if (top) {
            if ((ret = flow3r_bsp_ad7147_chip_process(&_top)) != ESP_OK) {
                if (top_ok) {
                    ESP_LOGE(TAG,
                             "Top captouch processing failed: %s (silencing "
                             "future warnings)",
                             esp_err_to_name(ret));
                    top_ok = false;
                }
            } else {
                top_ok = true;
            }
        }

        _callback(&_state);
    }
}

esp_err_t _gpio_interrupt_setup(gpio_num_t num, gpio_isr_t isr) {
    esp_err_t ret;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pin_bit_mask = (1 << num),
    };
    if ((ret = gpio_config(&io_conf)) != ESP_OK) {
        return ret;
    }
    if ((ret = gpio_isr_handler_add(num, isr, NULL)) != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t flow3r_bsp_captouch_init(flow3r_bsp_captouch_callback_t callback) {
    assert(_callback == NULL);
    assert(callback != NULL);
    _callback = callback;

    _q = xQueueCreate(2, sizeof(bool));
    assert(_q != NULL);

    esp_err_t ret;

    for (size_t i = 0; i < 10; i++) {
        bool top = (i % 2) == 0;
        _state.petals[i].kind = top ? petal_top : petal_bottom;
        _state.petals[i].ccw.kind = petal_pad_ccw;
        _state.petals[i].ccw.threshold = top ? 8000 : 12000;
        _state.petals[i].cw.kind = petal_pad_cw;
        _state.petals[i].cw.threshold = top ? 8000 : 12000;
        _state.petals[i].tip.kind = petal_pad_tip;
        _state.petals[i].tip.threshold = top ? 8000 : 12000;
        _state.petals[i].base.kind = petal_pad_base;
        _state.petals[i].base.threshold = top ? 8000 : 12000;
    }

    _top.callback = _on_chip_data;
    _top.user = &_top;
    _bot.callback = _on_chip_data;
    _bot.user = &_bot;

    if ((ret = flow3r_bsp_ad7147_chip_init(
             &_top, flow3r_i2c_addresses.touch_top)) != ESP_OK) {
        return ret;
    }
    if ((ret = flow3r_bsp_ad7147_chip_init(
             &_bot, flow3r_i2c_addresses.touch_bottom)) != ESP_OK) {
        return ret;
    }
    ESP_LOGI(TAG, "Captouch initialized");

    if ((ret = gpio_install_isr_service(ESP_INTR_FLAG_SHARED |
                                        ESP_INTR_FLAG_LOWMED)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service");
        return ret;
    }
    if ((ret = _gpio_interrupt_setup(_interrupt_gpio_bot, _bot_isr)) !=
        ESP_OK) {
        ESP_LOGE(TAG, "Failed to add bottom captouch ISR");
        return ret;
    }
    if (!_interrupt_shared) {
        // On badges with shared interrupts, only install the 'bot' ISR as a
        // shared ISR.
        if ((ret = _gpio_interrupt_setup(_interrupt_gpio_top, _top_isr)) !=
            ESP_OK) {
            ESP_LOGE(TAG, "Failed to add top captouch ISR");
            return ret;
        }
    }

    xTaskCreate(&_task, "captouch", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    _kickstart();
    return ESP_OK;
}

const flow3r_bsp_captouch_petal_pad_state_t *
flow3r_bsp_captouch_pad_for_petal_const(
    const flow3r_bsp_captouch_petal_state_t *petal, petal_pad_kind_t kind) {
    switch (kind) {
        case petal_pad_tip:
            return &petal->tip;
        case petal_pad_base:
            return &petal->base;
        case petal_pad_cw:
            return &petal->cw;
        case petal_pad_ccw:
            return &petal->ccw;
    }
    assert(0);
}

flow3r_bsp_captouch_petal_pad_state_t *flow3r_bsp_captouch_pad_for_petal(
    flow3r_bsp_captouch_petal_state_t *petal, petal_pad_kind_t kind) {
    switch (kind) {
        case petal_pad_tip:
            return &petal->tip;
        case petal_pad_base:
            return &petal->base;
        case petal_pad_cw:
            return &petal->cw;
        case petal_pad_ccw:
            return &petal->ccw;
    }
    assert(0);
}

void flow3r_bsp_captouch_calibrate() {
    _bot.calibration_pending = true;
    _top.calibration_pending = true;
}

bool flow3r_bsp_captouch_calibrating() {
    bool bot = _bot.calibration_pending || _bot.calibration_active;
    bool top = _top.calibration_pending || _top.calibration_active;
    return bot || top;
}

void flow3r_bsp_captouch_get_calibration_data(int32_t *data) {
    while (flow3r_bsp_captouch_calibrating()) {
    };
    for (uint8_t i = 0; i < 13; i++) {
        data[2 * i] = _top.channels[i].afe_offset;
        data[2 * i + 1] = _top.channels[i].amb;
        data[2 * i + 26] = _bot.channels[i].afe_offset;
        data[2 * i + 27] = _bot.channels[i].amb;
    }
}

static uint16_t amb_limit(int32_t data) {
    return data > 65535 ? 65535 : (data < 0 ? 0 : data);
}

static uint8_t afe_limit(int32_t data) {
    return data > 126 ? 126 : (data < 0 ? 0 : data);
}

void flow3r_bsp_captouch_set_calibration_data(int32_t *data) {
    while (flow3r_bsp_captouch_calibrating()) {
    };
    for (uint8_t i = 0; i < 13; i++) {
        _top.channels[i].afe_offset = afe_limit(data[2 * i]);
        _top.channels[i].amb = amb_limit(data[2 * i + 1]);
        _bot.channels[i].afe_offset = afe_limit(data[2 * i + 26]);
        _bot.channels[i].amb = amb_limit(data[2 * i + 27]);
    }
    _top.calibration_external = true;
    _bot.calibration_external = true;
}
