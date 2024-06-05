#include "flow3r_bsp.h"

#include <string.h>

#include "driver/i2s.h"
#include "esp_log.h"

esp_err_t flow3r_bsp_audio_write(const void *src, size_t size,
                                 size_t *bytes_written,
                                 TickType_t ticks_to_wait) {
    return i2s_write(0, src, size, bytes_written, ticks_to_wait);
}

#include "flow3r_bsp_max98091.h"

void flow3r_bsp_audio_init(void) {
    flow3r_bsp_max98091_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);  // dunno if necessary
    static const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = FLOW3R_BSP_AUDIO_SAMPLE_RATE,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
        .intr_alloc_flags = 0,  // default interrupt priority
        .dma_buf_count = FLOW3R_BSP_AUDIO_DMA_BUFFER_COUNT,
        .dma_buf_len = FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE,
        .use_apll = false
    };
    static const i2s_pin_config_t pin_config = {
        .bck_io_num = 10,
        .mck_io_num = 18,
        .ws_io_num = 11,
        .data_out_num = 12,
        .data_in_num = 13,
    };
    ESP_ERROR_CHECK(i2s_driver_install(0, &i2s_config, 0, NULL));
    i2s_set_pin(0, &pin_config);
}
float flow3r_bsp_audio_headphones_set_volume(bool mute, float dB) {
    return flow3r_bsp_max98091_headphones_set_volume(mute, dB);
}

float flow3r_bsp_audio_speaker_set_volume(bool mute, float dB) {
    return flow3r_bsp_max98091_speaker_set_volume(mute, dB);
}

int8_t flow3r_bsp_audio_headset_set_gain_dB(int8_t gain_dB) {
    return flow3r_bsp_max98091_headset_set_gain_dB(gain_dB);
}

void flow3r_bsp_audio_read_jacksense(flow3r_bsp_audio_jacksense_state_t *st) {
    flow3r_bsp_max98091_read_jacksense(st);
}

void flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_t source) {
    flow3r_bsp_max98091_input_set_source(source);
}

void flow3r_bsp_audio_headphones_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_max98091_headphones_line_in_set_hardware_thru(enable);
}

void flow3r_bsp_audio_speaker_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_max98091_speaker_line_in_set_hardware_thru(enable);
}

void flow3r_bsp_audio_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_max98091_line_in_set_hardware_thru(enable);
}

bool flow3r_bsp_audio_has_hardware_mute(void) { return true; }

void flow3r_bsp_audio_register_poke(uint8_t reg, uint8_t data) {
    flow3r_bsp_max98091_register_poke(reg, data);
}

esp_err_t flow3r_bsp_audio_read(void *dest, size_t size, size_t *bytes_read,
                                TickType_t ticks_to_wait) {
    return i2s_read(0, dest, size, bytes_read, ticks_to_wait);
}
