#include "flow3r_bsp.h"

#include "flow3r_bsp_rmtled.h"

esp_err_t flow3r_bsp_leds_init(void) {
    return flow3r_bsp_rmtled_init(FLOW3R_BSP_LED_COUNT);
}

void flow3r_bsp_leds_set_pixel(uint32_t index, uint32_t red, uint32_t green,
                               uint32_t blue) {
    flow3r_bsp_rmtled_set_pixel(index, red, green, blue);
}

esp_err_t flow3r_bsp_leds_refresh(TickType_t timeout_ms) {
    return flow3r_bsp_rmtled_refresh(timeout_ms);
}
