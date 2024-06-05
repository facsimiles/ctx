#pragma once

// Low-level AD7147 (captouch controller) interfacing functions. Currently only
// implements sequences where each channel is connected to the positive CDC
// input, and only the raw data is read out.

#include "esp_err.h"
#include "flow3r_bsp_i2c.h"

#include <stdint.h>

// 'Global' configuration for the AD7147 captouch controller.
typedef struct ad7147_device_config {
    unsigned int power_mode : 2;
    unsigned int lp_conv_delay : 2;
    unsigned int sequence_stage_num : 4;
    unsigned int decimation : 2;
    unsigned int sw_reset : 1;
    unsigned int int_pol : 1;
    unsigned int ext_source : 1;
    unsigned int cdc_bias : 2;

    unsigned int stage0_cal_en : 1;
    unsigned int stage1_cal_en : 1;
    unsigned int stage2_cal_en : 1;
    unsigned int stage3_cal_en : 1;
    unsigned int stage4_cal_en : 1;
    unsigned int stage5_cal_en : 1;
    unsigned int stage6_cal_en : 1;
    unsigned int stage7_cal_en : 1;
    unsigned int stage8_cal_en : 1;
    unsigned int stage9_cal_en : 1;
    unsigned int stage10_cal_en : 1;
    unsigned int stage11_cal_en : 1;
    unsigned int avg_fp_skip : 2;
    unsigned int avg_lp_skip : 2;

    unsigned int stage0_high_int_enable : 1;
    unsigned int stage1_high_int_enable : 1;
    unsigned int stage2_high_int_enable : 1;
    unsigned int stage3_high_int_enable : 1;
    unsigned int stage4_high_int_enable : 1;
    unsigned int stage5_high_int_enable : 1;
    unsigned int stage6_high_int_enable : 1;
    unsigned int stage7_high_int_enable : 1;
    unsigned int stage8_high_int_enable : 1;
    unsigned int stage9_high_int_enable : 1;
    unsigned int stage10_high_int_enable : 1;
    unsigned int stage11_high_int_enable : 1;

    bool stageX_complete_int_enable[12];
} ad7147_device_config_t;

// Per sequencer stage configuration.
typedef struct {
    unsigned int cinX_connection_setup[13];
    unsigned int se_connection_setup : 2;
    unsigned int neg_afe_offset_disable : 1;
    unsigned int pos_afe_offset_disable : 1;
    unsigned int neg_afe_offset : 6;
    unsigned int neg_afe_offset_swap : 1;
    unsigned int pos_afe_offset : 6;
    unsigned int pos_afe_offset_swap : 1;
    unsigned int neg_threshold_sensitivity : 4;
    unsigned int neg_peak_detect : 3;
    unsigned int pos_threshold_sensitivity : 4;
    unsigned int pos_peak_detect : 3;
} ad7147_stage_config_t;

typedef void (*ad7147_data_callback_t)(void *user, uint16_t *data, size_t len);

// AD7147 low level configuration/access structure. Doesn't know anything about
// calibration or high-level sequencing, just talks to a chip to configure
// stages and can be called to poll the chip for new CDC data.
typedef struct {
    // I2C address of chip.
    flow3r_i2c_address addr;

    // Function and user-controlled argument that will be called when the
    // sequence has finished and new data is available.
    ad7147_data_callback_t callback;
    void *user;

    ad7147_stage_config_t stage_config[12];
    ad7147_device_config_t dev_config;
    uint8_t num_stages;
} ad7147_hw_t;

// Initialize the AD7147 captouch controller with a given callback. This
// callback will be called when new data has been acquired.
esp_err_t ad7147_hw_init(ad7147_hw_t *device, flow3r_i2c_address addr,
                         ad7147_data_callback_t callback, void *user);

// Sequencer configuration, high-level.
typedef struct {
    // Number of sequencer stages, [1, 12].
    size_t len;
    // For each sequencer stage, channel number that this stage should sample.
    int8_t channels[12];
    // For each sequencer stage, AFE offset that this stage should use when
    // sampling the configured channel.
    int8_t pos_afe_offsets[12];
    // Whether idle pads are supposed to be connected to bias. Awful hack.
    bool idle_to_bias[12];
    int8_t neg_afe_offsets[12];
} ad7147_sequence_t;

// Configure sequencer stages.
//
// If reprogram is true, the sequencer will be restarted. This should be true if
// the sequence channels/lengths changed from the previous call.
esp_err_t ad7147_hw_configure_stages(ad7147_hw_t *device,
                                     const ad7147_sequence_t *seq,
                                     bool reprogram);

// Polls sequencer status from the chip and calls the user callback if new data
// is available / the sequence finished.
esp_err_t ad7147_hw_process(ad7147_hw_t *device);
