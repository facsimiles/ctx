#include "flow3r_bsp.h"

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "flow3r-spio";

typedef enum {
    flow3r_bsp_iochip_esp32 = 0,
    flow3r_bsp_iochip_portexp = 1,
    flow3r_bsp_iochip_dummy = 2,
} flow3r_bsp_iochip_t;

typedef struct {
    flow3r_bsp_iochip_t chip;
    uint32_t pin;
    bool output;
    bool pullup;
    bool invert;
} flow3r_bsp_iopin_t;

#define IESP(pinno, pullup_, ...)                                       \
    {                                                                   \
        .chip = flow3r_bsp_iochip_esp32, .pin = pinno, .output = false, \
        .pullup = pullup_, __VA_ARGS__                                  \
    }
#define IPEX(pinno, pexno, ...)                                      \
    {                                                                \
        .chip = flow3r_bsp_iochip_portexp, .pin = pinno + pexno * 8, \
        .output = false, __VA_ARGS__                                 \
    }
#define OPEX(pinno, pexno, ...)                                      \
    {                                                                \
        .chip = flow3r_bsp_iochip_portexp, .pin = pinno + pexno * 8, \
        .output = true, __VA_ARGS__                                  \
    }
#define IODUMMY \
    { .chip = flow3r_bsp_iochip_dummy }

typedef struct {
    flow3r_bsp_iopin_t left;
    flow3r_bsp_iopin_t mid;
    flow3r_bsp_iopin_t right;
} flow3r_bsp_iodef_tripos_t;

typedef struct {
    flow3r_bsp_iopin_t tip_badgelink_enable;
    flow3r_bsp_iopin_t tip_badgelink_data;
    flow3r_bsp_iopin_t ring_badgelink_enable;
    flow3r_bsp_iopin_t ring_badgelink_data;
} flow3r_bsp_iodef_trrs_t;

typedef struct {
    flow3r_bsp_iodef_tripos_t tripos_left;
    flow3r_bsp_iodef_tripos_t tripos_right;
    flow3r_bsp_iodef_trrs_t trrs_left;
    flow3r_bsp_iodef_trrs_t trrs_right;

    flow3r_bsp_iopin_t charger_state;
    flow3r_bsp_iopin_t jacksense_right;
} flow3r_bsp_iodef_t;

#if defined(CONFIG_FLOW3R_HW_GEN_P4) || defined(CONFIG_FLOW3R_HW_GEN_P3)
static const flow3r_bsp_iodef_t iodef = {
    .tripos_left =
        {
            .left = IESP(3, true, .invert = true),
            .mid = IESP(0, false, .invert = true),
            .right = IPEX(0, 1, .invert = true),
        },
    .tripos_right =
        {
            .left = IPEX(6, 1, .invert = true),
            .mid = IPEX(5, 1, .invert = true),
            .right = IPEX(7, 1, .invert = true),
        },
    .trrs_left =
        {
            .tip_badgelink_enable = OPEX(6, 0, .invert = true),
            .ring_badgelink_enable = OPEX(7, 0, .invert = true),
        },
    .trrs_right =
        {
            .tip_badgelink_enable = OPEX(5, 0, .invert = true),
            .ring_badgelink_enable = OPEX(4, 0, .invert = true),
        },
    .charger_state = IODUMMY,
    .jacksense_right = IODUMMY,
};
const flow3r_bsp_spio_programmable_pins_t flow3r_bsp_spio_programmable_pins = {
    .badgelink_left_tip = 6,
    .badgelink_left_ring = 7,
    .badgelink_right_tip = 4,
    .badgelink_right_ring = 5,
};
#define PORTEXP_MAX7321S
#elif defined(CONFIG_FLOW3R_HW_GEN_C23)
static const flow3r_bsp_iodef_t iodef = {
    .tripos_left =
        {
            .left = IPEX(7, 1, .invert = true),
            .mid = IESP(0, false, .invert = true),
            .right = IPEX(0, 1, .invert = true),
        },
    .tripos_right =
        {
            .left = IPEX(4, 1, .invert = true),
            .mid = IESP(3, true, .invert = true),
            .right = IPEX(5, 1, .invert = true),
        },
    .trrs_left =
        {
            .tip_badgelink_enable = OPEX(6, 0),
            .ring_badgelink_enable = OPEX(5, 0),
        },
    .trrs_right =
        {
            .tip_badgelink_enable = OPEX(4, 0),
            .ring_badgelink_enable = OPEX(3, 0),
        },
    .charger_state = IPEX(2, 1),
    .jacksense_right = IPEX(6, 1, .invert = true),
};
const flow3r_bsp_spio_programmable_pins_t flow3r_bsp_spio_programmable_pins = {
    .badgelink_left_tip = 7,
    .badgelink_left_ring = 6,
    .badgelink_right_tip = 4,
    .badgelink_right_ring = 5,
};
#define PORTEXP_MAX7321S
#else
#error "spio unimplemented for this badge generation"
#endif

#ifdef PORTEXP_MAX7321S
#include "flow3r_bsp_max7321.h"
static flow3r_bsp_max7321_t portexps[2];

static esp_err_t _portexp_init(void) {
    esp_err_t ret =
        flow3r_bsp_max7321_init(&portexps[0], flow3r_i2c_addresses.portexp[0]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize portexp 0: %s",
                 esp_err_to_name(ret));
        return ret;
    }
    ret =
        flow3r_bsp_max7321_init(&portexps[1], flow3r_i2c_addresses.portexp[1]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize portexp 1: %s",
                 esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

static uint32_t _portexp_pin(const flow3r_bsp_iopin_t *iopin) {
    return iopin->pin % 8;
}

static flow3r_bsp_max7321_t *_portexp_max(const flow3r_bsp_iopin_t *iopin) {
    uint32_t ix = (iopin->pin >> 3) % 2;
    return &portexps[ix];
}

static void _iopin_portexp_init(const flow3r_bsp_iopin_t *iopin) {
    flow3r_bsp_max7321_set_pinmode_output(_portexp_max(iopin),
                                          _portexp_pin(iopin), iopin->output);
}

static bool _iopin_portexp_get_pin(const flow3r_bsp_iopin_t *iopin) {
    return flow3r_bsp_max7321_get_pin(_portexp_max(iopin), _portexp_pin(iopin));
}

static void _iopin_portexp_set_pin(const flow3r_bsp_iopin_t *iopin, bool on) {
    flow3r_bsp_max7321_set_pin(_portexp_max(iopin), _portexp_pin(iopin), on);
}

static esp_err_t _portexp_update(void) {
    esp_err_t ret = flow3r_bsp_max7321_update(&portexps[0]);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = flow3r_bsp_max7321_update(&portexps[1]);
    if (ret != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}
#endif

static esp_err_t _iopin_esp32_init(const flow3r_bsp_iopin_t *iopin) {
    gpio_config_t cfg = { .pin_bit_mask = 1 << iopin->pin,
                          .mode = GPIO_MODE_INPUT,
                          .pull_up_en = GPIO_PULLUP_DISABLE,
                          .pull_down_en = GPIO_PULLDOWN_DISABLE,
                          .intr_type = GPIO_INTR_DISABLE };
    if (iopin->output) {
        // Untested, so unimplemented.
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (iopin->pullup) {
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    }
    return gpio_config(&cfg);
}

static esp_err_t _iopin_init(const flow3r_bsp_iopin_t *iopin) {
    switch (iopin->chip) {
        case flow3r_bsp_iochip_esp32:
            return _iopin_esp32_init(iopin);
        case flow3r_bsp_iochip_portexp:
            _iopin_portexp_init(iopin);
            return ESP_OK;
        case flow3r_bsp_iochip_dummy:
            return ESP_OK;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

static bool _iopin_esp32_get_pin(const flow3r_bsp_iopin_t *iopin) {
    return gpio_get_level(iopin->pin);
}

static bool _iopin_get_pin(const flow3r_bsp_iopin_t *iopin) {
    bool res = false;
    switch (iopin->chip) {
        case flow3r_bsp_iochip_esp32:
            res = _iopin_esp32_get_pin(iopin);
            break;
        case flow3r_bsp_iochip_portexp:
            res = _iopin_portexp_get_pin(iopin);
            break;
        case flow3r_bsp_iochip_dummy:
            res = false;
    }
    if (iopin->invert) {
        res = !res;
    }
    return res;
}

static void _iopin_set_pin(const flow3r_bsp_iopin_t *iopin, bool on) {
    if (iopin->invert) {
        on = !on;
    }

    switch (iopin->chip) {
        case flow3r_bsp_iochip_esp32:
            // Not implemented.
            break;
        case flow3r_bsp_iochip_portexp:
            _iopin_portexp_set_pin(iopin, on);
            break;
        case flow3r_bsp_iochip_dummy:
            break;
    }
}

#define INITIO(name)                \
    ret = _iopin_init(&iodef.name); \
    if (ret != ESP_OK) {            \
        return ret;                 \
    }
esp_err_t flow3r_bsp_spio_init(void) {
    esp_err_t ret = _portexp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Port Expander initialzation failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }
    INITIO(tripos_left.left);
    INITIO(tripos_left.mid);
    INITIO(tripos_left.right);
    INITIO(tripos_right.left);
    INITIO(tripos_right.mid);
    INITIO(tripos_right.right);
    INITIO(trrs_left.tip_badgelink_enable);
    INITIO(trrs_left.ring_badgelink_enable);
    INITIO(trrs_right.tip_badgelink_enable);
    INITIO(trrs_right.ring_badgelink_enable);

    return _portexp_update();
}

esp_err_t flow3r_bsp_spio_update(void) { return _portexp_update(); }

static flow3r_bsp_tripos_state_t _tripos_get(
    const flow3r_bsp_iodef_tripos_t *io) {
    if (_iopin_get_pin(&io->mid)) {
        return flow3r_bsp_tripos_mid;
    }
    if (_iopin_get_pin(&io->left)) {
        return flow3r_bsp_tripos_left;
    }
    if (_iopin_get_pin(&io->right)) {
        return flow3r_bsp_tripos_right;
    }
    return flow3r_bsp_tripos_none;
}

flow3r_bsp_tripos_state_t flow3r_bsp_spio_left_button_get(void) {
    return _tripos_get(&iodef.tripos_left);
}

flow3r_bsp_tripos_state_t flow3r_bsp_spio_right_button_get(void) {
    return _tripos_get(&iodef.tripos_right);
}

bool flow3r_bsp_spio_charger_state_get(void) {
    return _iopin_get_pin(&iodef.charger_state);
}

bool flow3r_bsp_spio_jacksense_right_get(void) {
    return _iopin_get_pin(&iodef.jacksense_right);
}

void flow3r_bsp_spio_badgelink_left_enable(bool tip_on, bool ring_on) {
    _iopin_set_pin(&iodef.trrs_left.ring_badgelink_enable, ring_on);
    _iopin_set_pin(&iodef.trrs_left.tip_badgelink_enable, tip_on);
}

void flow3r_bsp_spio_badgelink_right_enable(bool tip_on, bool ring_on) {
    _iopin_set_pin(&iodef.trrs_right.ring_badgelink_enable, ring_on);
    _iopin_set_pin(&iodef.trrs_right.tip_badgelink_enable, tip_on);
}
