#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "flow3r_bsp_i2c.h"

#include "esp_err.h"

typedef struct {
    flow3r_i2c_address addr;
    // mask for pins we wish to use as outputs
    uint8_t is_output_pin;
    uint8_t read_state;
    // goal output state
    uint8_t output_state;
} flow3r_bsp_max7321_t;

esp_err_t flow3r_bsp_max7321_init(flow3r_bsp_max7321_t *max,
                                  flow3r_i2c_address addr);
void flow3r_bsp_max7321_set_pinmode_output(flow3r_bsp_max7321_t *max,
                                           uint32_t pin, bool output);
bool flow3r_bsp_max7321_get_pin(flow3r_bsp_max7321_t *max, uint32_t pin);
void flow3r_bsp_max7321_set_pin(flow3r_bsp_max7321_t *max, uint32_t pin,
                                bool on);
esp_err_t flow3r_bsp_max7321_update(flow3r_bsp_max7321_t *max);