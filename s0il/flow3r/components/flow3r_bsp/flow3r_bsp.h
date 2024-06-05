#pragma once

#include "flow3r_bsp_i2c.h"

#include <stdint.h>

// Initialize badge display. An error will be reported if the initialization
// failed.
//
// Must be called exactly once from a task and cannot be called cocurrently with
// any other flow3r_bsp_display_* functions.
//
// Side effects: initializes singleton flow3r display object. All other
// flow3r_bsp_display_* functions operate on same object.
void flow3r_bsp_display_init(void);

// Send a full framebuffer of 240x240 16bpp pixels to the display. No-op if
// display hasn't been succesfully initialized.
//
// Transfer will be performed using DMA/interrupts and will block the calling
// FreeRTOS task until finished.
//
// This must not be called if another transfer is alraedy being performed. The
// user code should sequence access and make sure not more than one transfer is
// performed simultaneously.
void flow3r_bsp_display_send_fb(void *fb_data, int bits);

void flow3r_bsp_display_send_fb_osd(void *fb_data, int bits, int scale,
                                    void *osd_data, int osd_x0, int osd_y0,
                                    int osd_x1, int osd_y1);

// Set display backlight, as integer percent value (from 0 to 100, clamped).
// No-op if display hasn't been succesfully initialized.
void flow3r_bsp_display_set_backlight(uint8_t percent);

// Currently same on all generations. Might change on future revisions.
#define FLOW3R_BSP_DISPLAY_WIDTH 240
#define FLOW3R_BSP_DISPLAY_HEIGHT 240

// Badge hardware generation name, human-readable.
extern const char *flow3r_bsp_hw_name;

#define FLOW3R_BSP_AUDIO_SAMPLE_RATE 48000
#define FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE 64
#define FLOW3R_BSP_AUDIO_DMA_BUFFER_COUNT 4

typedef enum {
    flow3r_bsp_audio_input_source_none = 0,
    // Line in on riht jack.
    flow3r_bsp_audio_input_source_line_in = 1,
    // Headset microphone on left jack.
    flow3r_bsp_audio_input_source_headset_mic = 2,
    // Onboard microphone (enabled red LED).
    flow3r_bsp_audio_input_source_onboard_mic = 3,
} flow3r_bsp_audio_input_source_t;

// Initialize the audio subsystem of the badge, including the codec and I2S data
// channel.
void flow3r_bsp_audio_init(void);

// Attempts to set target volume for the headphone output/onboard speakers
// respectively, clamps/rounds if necessary and returns the actual volume
// applied in hardware.
//
// Absolute reference arbitrary.
//
// Returns value actually applied in hardware, which might be significantly off
// from the requested volume. The caller must perform fine audio volume adjust
// in software.
float flow3r_bsp_audio_headphones_set_volume(bool mute, float dB);
float flow3r_bsp_audio_speaker_set_volume(bool mute, float dB);

// Hardware preamp gain, 0dB-50dB.
//
// TODO: figure out if int/float inconsistency is a good thing here compared to
// all other _dB functions.
int8_t flow3r_bsp_audio_headset_set_gain_dB(int8_t gain_dB);

typedef struct {
    bool headphones;
    bool headset;
    bool line_in;
} flow3r_bsp_audio_jacksense_state_t;

// Polls hardware to check if headphones, headset or line in are plugged into
// the 3.5mm jacks. The result is saved into `st`, which must be non-NULL.
void flow3r_bsp_audio_read_jacksense(flow3r_bsp_audio_jacksense_state_t *st);

// The codec can transmit audio data from different sources. This function
// enables one or no source as provided by the flow3r_bsp_audio_input_source_t
// enum.
//
// Note: The onboard digital mic turns on an LED on the top board if it receives
// a clock signal which is considered a good proxy for its capability of reading
// data.
//
// TODO: check if sources are available
void flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_t source);

// These route whatever is on the line in port directly to the headphones or
// speaker respectively (enable = 1), or don't (enable = 0). Is affected by mute
// and coarse hardware volume settings, however software fine volume is not
// applied.
//
// Good for testing, might deprecate later, idk~
void flow3r_bsp_audio_headphones_line_in_set_hardware_thru(bool enable);
void flow3r_bsp_audio_speaker_line_in_set_hardware_thru(bool enable);
void flow3r_bsp_audio_line_in_set_hardware_thru(bool enable);

// Returns true if the flow3r_bsp_audio_*_set_volume mute argument is being
// interpreted. If false, the user should perform muting in software.
//
// NOTE: this is only 'false' on very early hardware (P1). If you're
// implementing something for badges available at camp, assume this always
// returns true.
bool flow3r_bsp_audio_has_hardware_mute(void);

// Read or write I2S audio data from/to the codec.
//
// The output is sent to the speakers and/or headphones, depending on
// speaker/headphone volume/muting.
//
// The input is routed according to the configured source (via
// flow3r_bp_audio_input_set_source).
esp_err_t flow3r_bsp_audio_read(void *dest, size_t size, size_t *bytes_read,
                                TickType_t ticks_to_wait);
esp_err_t flow3r_bsp_audio_write(const void *src, size_t size,
                                 size_t *bytes_written,
                                 TickType_t ticks_to_wait);

// Write audio codec register. Obviously very unsafe. Have fun.
void flow3r_bsp_audio_register_poke(uint8_t reg, uint8_t data);

#define FLOW3R_BSP_LED_COUNT 40

// Initialize LEDs.
esp_err_t flow3r_bsp_leds_init(void);

// Set internal buffer for given LED to an RGB value.
//
// Index is a value in [0, FLOW3R_BSP_LED_COUNT).
//
// RGB values are in [0, 0xff].
void flow3r_bsp_leds_set_pixel(uint32_t index, uint32_t red, uint32_t green,
                               uint32_t blue);

// Transmit from internal buffer into LEDs. This will block in case there
// already is a previous transmission happening.
esp_err_t flow3r_bsp_leds_refresh(TickType_t timeout_ms);

// A 'tripos' button is what we're calling the shoulder buttons. As the name
// indicates, it has three positions: left, middle (a.k.a. down/press) and
// right.
typedef enum {
    // Not pressed.
    flow3r_bsp_tripos_none = 0,
    // Pressed towards the left.
    flow3r_bsp_tripos_left = -1,
    // Pressed down.
    flow3r_bsp_tripos_mid = 2,
    // Pressed towards the right.
    flow3r_bsp_tripos_right = 1,
} flow3r_bsp_tripos_state_t;

// Initialize 'special purpose i/o', which is like gpio, but more :). This means
// effectively all the buttons and on-board detect/enable signals.
esp_err_t flow3r_bsp_spio_init(void);

// Refresh the state of I/O: update outputs based on last called _set functions,
// and make _get return the newest hardware state.
esp_err_t flow3r_bsp_spio_update(void);

// Return the latest updated position of the left shoulder/tripos button.
//
// flow3r_bsp_spio_update must be called for this data to be up to date.
flow3r_bsp_tripos_state_t flow3r_bsp_spio_left_button_get(void);

// Return the latest updated position of the right shoulder/tripos button.
//
// flow3r_bsp_spio_update must be called for this data to be up to date.
flow3r_bsp_tripos_state_t flow3r_bsp_spio_right_button_get(void);

// Returns true if the device is charging.
//
// flow3r_bsp_spio_update must be called for this data to be up to date.
bool flow3r_bsp_spio_charger_state_get(void);

// Returns true if the device has something plugged into the right 3.5mm jack.
//
// flow3r_bsp_spio_update must be called for this data to be up to date.
bool flow3r_bsp_spio_jacksense_right_get(void);

// Switch tip/ring muxes to 'badgelink' digital I/O data pins on the left hand
// jack. As this is the same as the headphone jack, please observe caution when
// enabling this! See the comment at the bottom of st3m_audio.h.
//
// flow3r_bsp_spio_update must be called for this setting to actually propagate
// to hardware.
void flow3r_bsp_spio_badgelink_left_enable(bool tip_on, bool ring_on);

// Switch tip/ring muxes to 'badgelink' digital I/O data pins on the right hand
// jack.
//
// flow3r_bsp_spio_update must be called for this setting to actually propagate
// to hardware.
void flow3r_bsp_spio_badgelink_right_enable(bool tip_on, bool ring_on);

// Pin mapping information of programmable badge I/O. These are GPIO numbers
// that can be used with the ESP-IDF API.
typedef struct {
    // Left jack, headphone/line out.
    uint8_t badgelink_left_tip;
    uint8_t badgelink_left_ring;
    // Right jack, line in.
    uint8_t badgelink_right_tip;
    uint8_t badgelink_right_ring;
} flow3r_bsp_spio_programmable_pins_t;
extern const flow3r_bsp_spio_programmable_pins_t
    flow3r_bsp_spio_programmable_pins;
