#pragma once

// MAX98091 audio codec support.

#include "stdbool.h"
#include "stdint.h"

#include "flow3r_bsp.h"

typedef struct {
    bool is_active;
    float gain;
    float a1, a2, b0, b1, b2;

} flow3r_bsp_max98091_biquad_t;

void flow3r_bsp_max98091_init(void);
float flow3r_bsp_max98091_headphones_set_volume(bool mute, float dB);
float flow3r_bsp_max98091_speaker_set_volume(bool mute, float dB);
int8_t flow3r_bsp_max98091_headset_set_gain_dB(int8_t gain_dB);
void flow3r_bsp_max98091_read_jacksense(flow3r_bsp_audio_jacksense_state_t *st);
void flow3r_bsp_max98091_input_set_source(
    flow3r_bsp_audio_input_source_t source);
void flow3r_bsp_max98091_headphones_line_in_set_hardware_thru(bool enable);
void flow3r_bsp_max98091_speaker_line_in_set_hardware_thru(bool enable);
void flow3r_bsp_max98091_line_in_set_hardware_thru(bool enable);
void flow3r_bsp_max98091_register_poke(uint8_t reg, uint8_t data);
void flow3r_bsp_max98091_set_speaker_eq(bool enable);
