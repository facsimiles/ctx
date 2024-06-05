#pragma once

// Highest-level driver for captouch on the flow3r badge. Uses 2x AD7147.

// The flow3r has 10 touch petals. 5 petals on the top layer, 5 petals on the
// bottom layer.
//
// Top petals have three capacitive pads. Bottom petals have two capacitive
// pads.
//
// The petals are numbered from 0 to 9 (inclusive). Petal 0 is next to the USB
// port, and is a top petal. Petal 1 is a bottom petal to its right. Petal 2 is
// a top petal to its right, and the rest continue clockwise accordingly.

#include "esp_err.h"

#include <stdbool.h>

// One of the four possible touch points (pads) on a petal. Top petals have
// base/cw/ccw. Bottom petals have base/tip.
typedef enum {
    // Pad away from centre of badge.
    petal_pad_tip = 0,
    // Pad going counter-clockwise around the badge.
    petal_pad_ccw = 1,
    // Pad going clockwise around the badge.
    petal_pad_cw = 2,
    // Pad going towards the centre of the badge.
    petal_pad_base = 3,
} petal_pad_kind_t;

// Each petal can be either top or bottom.
typedef enum {
    // Petal on the top layer. Has base, cw, ccw pads.
    petal_top = 0,
    // petal on the bottom layer. Has base and tip fields.
    petal_bottom = 1,
} petal_kind_t;

// State of a petal's pad.
typedef struct {
    // Is it a top or bottom petal?
    petal_pad_kind_t kind;
    // Raw value, compensated for ambient value.
    uint16_t raw;
    // Configured threshold for touch detection.
    uint16_t threshold;
} flow3r_bsp_captouch_petal_pad_state_t;

// State of a petal. Only the fields relevant to the petal kind (tip/base or
// base/cw/ccw) are present.
typedef struct {
    petal_kind_t kind;
    flow3r_bsp_captouch_petal_pad_state_t tip;
    flow3r_bsp_captouch_petal_pad_state_t ccw;
    flow3r_bsp_captouch_petal_pad_state_t cw;
    flow3r_bsp_captouch_petal_pad_state_t base;
} flow3r_bsp_captouch_petal_state_t;

// State of all petals of the badge.
typedef struct {
    flow3r_bsp_captouch_petal_state_t petals[10];
} flow3r_bsp_captouch_state_t;

typedef void (*flow3r_bsp_captouch_callback_t)(
    const flow3r_bsp_captouch_state_t *state);

// Initialize captouch subsystem with a given callback. This callback will be
// called any time new captouch data is available. The given data will be valid
// for the lifetime of the function, so should be copied by users.
//
// An interrupt and task will be set up to handle the data processing.
esp_err_t flow3r_bsp_captouch_init(flow3r_bsp_captouch_callback_t callback);

// Get a given petal's pad data for a given petal kind.
const flow3r_bsp_captouch_petal_pad_state_t *
flow3r_bsp_captouch_pad_for_petal_const(
    const flow3r_bsp_captouch_petal_state_t *petal, petal_pad_kind_t kind);
flow3r_bsp_captouch_petal_pad_state_t *flow3r_bsp_captouch_pad_for_petal(
    flow3r_bsp_captouch_petal_state_t *petal, petal_pad_kind_t kind);

// Request captouch calibration.
void flow3r_bsp_captouch_calibrate();

// Returns true if captouch is currently calibrating.
bool flow3r_bsp_captouch_calibrating();

// Set/get calibration data. data[] should be at least 52 entries long.
void flow3r_bsp_captouch_get_calibration_data(int32_t *data);
void flow3r_bsp_captouch_set_calibration_data(int32_t *data);
