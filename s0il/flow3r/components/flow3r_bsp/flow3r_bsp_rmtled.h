#pragma once

#include "esp_err.h"

esp_err_t flow3r_bsp_rmtled_init(uint16_t num_leds);
void flow3r_bsp_rmtled_set_pixel(uint32_t index, uint32_t red, uint32_t green,
                                 uint32_t blue);
esp_err_t flow3r_bsp_rmtled_refresh(int32_t timeout_ms);