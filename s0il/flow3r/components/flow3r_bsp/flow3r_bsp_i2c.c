#include "flow3r_bsp_i2c.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>

static SemaphoreHandle_t mutex;

static const char *TAG = "flow3r-bsp-i2c";

#if defined(CONFIG_FLOW3R_HW_GEN_P3)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
    .codec = 0x10,
    .touch_top = 0x2d,
    .touch_bottom = 0x2c,
    .portexp = { 0x6e, 0x6d },
};
#elif defined(CONFIG_FLOW3R_HW_GEN_P4)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
    .codec = 0x10,
    .touch_top = 0x2c,
    .touch_bottom = 0x2d,
    .portexp = { 0x6e, 0x6d },
};
#elif defined(CONFIG_FLOW3R_HW_GEN_C23)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
    .codec = 0x10,
    .touch_top = 0x2c,
    .touch_bottom = 0x2d,
    .portexp = { 0x6e, 0x6d },
};
#else
#error "i2c not implemented for this badge generation"
#endif

static i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 2,
    .scl_io_num = 1,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
};

void flow3r_bsp_i2c_init(void) {
    if (mutex != NULL) {
        return;
    }

    mutex = xSemaphoreCreateMutex();
    assert(mutex != NULL);

    assert(i2c_param_config(I2C_NUM_0, &i2c_conf) == ESP_OK);
    assert(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0,
                              ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED) ==
           ESP_OK);

    flow3r_bsp_i2c_scan(NULL);
}

// Take I2C bus lock.
static void flow3r_bsp_i2c_get(void) { xSemaphoreTake(mutex, portMAX_DELAY); }

// Release I2C bus lock.
static void flow3r_bsp_i2c_put(void) { xSemaphoreGive(mutex); }

esp_err_t flow3r_bsp_i2c_write_to_device(uint8_t address, const uint8_t *buffer,
                                         size_t write_size,
                                         TickType_t ticks_to_wait) {
    flow3r_bsp_i2c_get();
    esp_err_t res = i2c_master_write_to_device(I2C_NUM_0, address, buffer,
                                               write_size, ticks_to_wait);
    flow3r_bsp_i2c_put();
    return res;
}

esp_err_t flow3r_bsp_i2c_write_read_device(
    uint8_t address, const uint8_t *write_buffer, size_t write_size,
    uint8_t *read_buffer, size_t read_size, TickType_t ticks_to_wait) {
    flow3r_bsp_i2c_get();
    esp_err_t res = i2c_master_write_read_device(
        I2C_NUM_0, address, write_buffer, write_size, read_buffer, read_size,
        ticks_to_wait);
    flow3r_bsp_i2c_put();
    return res;
}

void flow3r_bsp_i2c_scan(flow3r_bsp_i2c_scan_result_t *res) {
    if (res != NULL) {
        memset(res, 0, sizeof(flow3r_bsp_i2c_scan_result_t));
    }
    ESP_LOGI(TAG, "Scan: starting...");
    for (uint8_t i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1);
        i2c_master_stop(cmd);
        esp_err_t ret =
            i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Scan: detected %02x", i);
            if (res != NULL) {
                size_t ix = i / 32;
                size_t offs = i % 32;
                res->res[ix] |= (1 << offs);
            }
        }
    }
    ESP_LOGI(TAG, "Scan: done.");
}
