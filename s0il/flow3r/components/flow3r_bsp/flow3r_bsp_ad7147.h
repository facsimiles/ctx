#pragma once

// High-level AD7147 captouch functions. Includes support for switching
// sequences and has basic software calibration routines.
//
// Only takes care of one captouch controller at once. Both captouch controllers
// are put together into a single view in flow3r_bsp_captouch.

// The AD7147 captouch chip is weird.
//
// It has 13 input channels, and can perform arbitrarily sequenced reads from
// these channels (then report the results of that sequence at once), but the
// maximum sequence length is 12 stages.
//
// That means getting 13 independent captouch channel readouts is tricky, as you
// need to change these sequences around to get all channels.

#include "flow3r_bsp_ad7147_hw.h"

#define _AD7147_SEQ_MAX 2
#define _AD7147_CALIB_CYCLES 16

// State of an AD7147 channel. Each AD7147 has 13 channels, but can only access
// 12 of them at once in a single sequence.
typedef struct {
    // Positive AFE offset currently programmed. [0,64).
    volatile int8_t afe_offset;
    // Last measurement.
    uint16_t cdc;

    // Ambient value used for offset when checking for touch presence. Written
    // by calibration, and attempts to reach a preset calibration setpoint.
    volatile uint16_t amb;
    // Calibration samples gathered during the calibraiton process.
    uint16_t amb_meas[_AD7147_CALIB_CYCLES];
} ad7147_channel_t;

// State and configuration of an AD7147 chip. Wraps the low-level structure in
// everything required to manage multiple sequences and perform calibration.
typedef struct {
    // Opaque name used to prefix log messages.
    const char *name;
    // True for bottom chip, false for top
    bool is_bot;

    // [0, n_channels) are the expected connected channels to the inputs of the
    // chip.
    size_t nchannels;
    ad7147_channel_t channels[13];

    // Sequences to be handled by this chip. Each sequence is a -1 right-padded
    // list of channel numbers that the sequence will be programmed to. If a
    // sequence is all -1, it will be skipped.
    int8_t sequences[_AD7147_SEQ_MAX][12];

    // Current position within the sequences list.
    size_t seq_position;

    // Called when all sequences have scanned through.
    ad7147_data_callback_t callback;
    void *user;

    ad7147_hw_t dev;
    bool failed;

    // Request applying external calibration
    volatile bool calibration_pending;
    // True if calibration is running or pending
    volatile bool calibration_active;
    // Set true if external calibration is to be written to hw
    volatile bool calibration_external;
    int8_t calibration_cycles;
} ad7147_chip_t;

// Call to initialize the chip at a given address. Structure must be zeroed, and
// callback must be configured.
//
// The chip will be configured to pull its interrupt line low when a sequence
// has finished and thus when _chip_process should be called.
esp_err_t flow3r_bsp_ad7147_chip_init(ad7147_chip_t *chip,
                                      flow3r_i2c_address address);

// Call to poll the chip and perform any necessary actions. Can be called from
// an interrupt.
esp_err_t flow3r_bsp_ad7147_chip_process(ad7147_chip_t *chip);
