#pragma once

#include "freertos/FreeRTOS.h"

// Type definition for device addresses on the main I2C bus of flow3r badges.
// This is a 7-bit address, which does not contain an R/W bit.
typedef uint8_t flow3r_i2c_address;

// I2C addresses of different devices on a flow3r badge. Not all fields are
// applicable to all badge generations.
typedef struct {
    // Address of the audio codec.
    flow3r_i2c_address codec;
    // Address of the captouch controller on the top board.
    flow3r_i2c_address touch_top;
    // Address of the captouch controller on the bottom board.
    flow3r_i2c_address touch_bottom;
    // Addresses of the port expander chips.
    flow3r_i2c_address portexp[2];
} flow3r_i2c_addressdef;

// Global constant defining addresses for the badge generation that this
// firmware has been built for.
extern const flow3r_i2c_addressdef flow3r_i2c_addresses;

// Initialize the badge's I2C subsystem. Must be called before any other
// flow3r_bsp_i2c_* calls.
void flow3r_bsp_i2c_init(void);

// Perform a write transaction to a I2C device.
//
// This can be called concurrently from different tassks.
esp_err_t flow3r_bsp_i2c_write_to_device(flow3r_i2c_address address,
                                         const uint8_t *buffer,
                                         size_t write_size,
                                         TickType_t ticks_to_wait);

// Perform a write-then read transaction on an I2C device.
//
// This can be called concurrently from different tassks.
esp_err_t flow3r_bsp_i2c_write_read_device(
    flow3r_i2c_address address, const uint8_t *write_buffer, size_t write_size,
    uint8_t *read_buffer, size_t read_size, TickType_t ticks_to_wait);

typedef struct {
    uint32_t res[4];  // Bitfield of addresses from 0 to 127.
} flow3r_bsp_i2c_scan_result_t;

void flow3r_bsp_i2c_scan(flow3r_bsp_i2c_scan_result_t *res);