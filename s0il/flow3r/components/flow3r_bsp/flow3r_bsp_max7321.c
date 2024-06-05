#include "flow3r_bsp_max7321.h"
#include "flow3r_bsp_i2c.h"

/* The MAX7321 doesn't have any input/output pin configuration methods. Instead,
 * it has a ~40k pullup resistor to VCC and a programmable shunt transistor to
 * VEE. Writing a byte to the IC closes each shunt with a LO at its index.
 * Reading a byte returns the state of each pin.
 *
 * This means that changing a single output bit cannot be changed without some
 * information about the other pins. Also a output pin set to HI can read as LO
 * if there's an outside shunt.
 */

// TODO(q3k): unhardcode
#define TIMEOUT_MS 1000

esp_err_t flow3r_bsp_max7321_init(flow3r_bsp_max7321_t *max,
                                  flow3r_i2c_address addr) {
    max->addr = addr;
    max->is_output_pin = 0;
    max->read_state = 0xff;
    max->output_state = 0xff;

    return flow3r_bsp_max7321_update(max);
}

void flow3r_bsp_max7321_set_pinmode_output(flow3r_bsp_max7321_t *max,
                                           uint32_t pin, bool output) {
    if (output) {
        max->is_output_pin |= (1 << pin);
    } else {
        max->is_output_pin &= ~(1 << pin);
    }
}

bool flow3r_bsp_max7321_get_pin(flow3r_bsp_max7321_t *max, uint32_t pin) {
    return ((max->read_state) >> pin) & 1;
}

void flow3r_bsp_max7321_set_pin(flow3r_bsp_max7321_t *max, uint32_t pin,
                                bool on) {
    if (((max->is_output_pin) >> pin) & 1) {
        if (on) {
            max->output_state |= 1 << pin;
        } else {
            max->output_state &= ~(1 << pin);
        }
    }
}

esp_err_t flow3r_bsp_max7321_update(flow3r_bsp_max7321_t *max) {
    uint8_t rx = 0;
    uint8_t tx = (~(max->is_output_pin)) | max->output_state;

    esp_err_t ret = flow3r_bsp_i2c_write_read_device(
        max->addr, &tx, sizeof(tx), &rx, sizeof(rx),
        TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        return ret;
    }
    max->read_state = rx;

    return ESP_OK;
}