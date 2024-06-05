#include "flow3r_bsp_max98091.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "flow3r_bsp_i2c.h"
#include "flow3r_bsp_max98091_hwregs.h"

#define TIMEOUT_MS 1000

static const char *TAG = "flow3r-bsp-max98091";

/* values generated with
 * https://www.earlevel.com/main/2021/09/02/biquad-calculator-v3/ note: a and b
 * are flipped in this calculator. a0 must be 1.0
 */

static flow3r_bsp_max98091_biquad_t speaker_eq[7] = {
    // soft bass cut below 250Hz. The speakers can't go much
    // lower so you just burn power and add unnecessary distortion
    // at high volumes. q = 0.71 for a mild resonance around cutoff.
    { .is_active = true,
      .gain = 1,
      .a1 = -1.953907993635898,
      .a2 = 0.9549547008658794,
      .b0 = 0.9772156736254444,
      .b1 = -1.9544313472508887,
      .b2 = 0.9772156736254444 },
    // bass boost test, 300Hz q = 2 resonant low pass, doesn't do much
    { .is_active = false,
      .gain = 0.5,
      .a1 = -1.9790339454615058,
      .a2 = 0.9805608861277517,
      .b0 = 0.9898987078973144,
      .b1 = -1.9797974157946288,
      .b2 = 0.9898987078973144 },
    // first order high shelf at 1.4KHz/-4.5dB to mellow out
    { .is_active = true,
      .gain = 1.,
      .a1 = -0.8962133020718939,
      .a2 = 0.,
      .b0 = 0.6166445890142367,
      .b1 = -0.5128578910861306,
      .b2 = 0. },
    // 2nd order high shelf at 9kHz/-9dB to remove fizz
    { .is_active = true,
      .gain = 1.,
      .a1 = -0.49825909435330995,
      .a2 = 0.,
      .b0 = 0.6263246182012638,
      .b1 = -0.12458371255457386,
      .b2 = 0. },
    { .is_active = false,
      .gain = 1.,
      .a1 = 0.,
      .a2 = 0.,
      .b0 = 0.,
      .b1 = 0.,
      .b2 = 0. },
    { .is_active = false,
      .gain = 1.,
      .a1 = 0.,
      .a2 = 0.,
      .b0 = 0.,
      .b1 = 0.,
      .b2 = 0. },
    { .is_active = false,
      .gain = 1.,
      .a1 = 0.,
      .a2 = 0.,
      .b0 = 0.,
      .b1 = 0.,
      .b2 = 0. }
};

static uint8_t max98091_read(const uint8_t reg) {
    const uint8_t tx[] = { reg };
    uint8_t rx[1];
    flow3r_bsp_i2c_write_read_device(flow3r_i2c_addresses.codec, tx, sizeof(tx),
                                     rx, sizeof(rx),
                                     TIMEOUT_MS / portTICK_PERIOD_MS);
    // TODO(q3k): handle error
    return rx[0];
}

static esp_err_t max98091_write(const uint8_t reg, const uint8_t data) {
    const uint8_t tx[] = { reg, data };
    return flow3r_bsp_i2c_write_to_device(flow3r_i2c_addresses.codec, tx,
                                          sizeof(tx),
                                          TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t max98091_check(const uint8_t reg, const uint8_t data) {
    esp_err_t ret = max98091_write(reg, data);
    if (ret != ESP_OK) {
        return ret;
    }

    switch (reg) {
        // Do not attempt to read back registers that are write-only.
        case MAX98091_SOFTWARE_RESET:
        case MAX98091_DAI_INTERFACE:
        case MAX98091_DAC_PATH:
        case MAX98091_LINE_TO_ADC:
            return ESP_OK;
    }

    // EQ registers are write only
    if ((reg >= 0x46) && (reg <= 0xAE)) return ESP_OK;

    uint8_t readback = max98091_read(reg);
    if (readback != data) {
        ESP_LOGE(TAG, "Write of %02X failed: wanted %02x, got %02x", reg, data,
                 readback);
    }
    return ESP_OK;
}

void flow3r_bsp_max98091_set_speaker_eq(bool enable) {
    if (!enable) {
        max98091_check(0x41, 0);
        return;
    }
    uint8_t reg = 0x46;
    for (uint8_t i = 0; i < 7; i++) {
        flow3r_bsp_max98091_biquad_t *bq = &(speaker_eq[i]);
        float a1 = 0;
        float a2 = 0;
        float b0 = 1;
        float b1 = 0;
        float b2 = 0;
        if (bq->is_active && enable) {
            a1 = bq->a1;
            a2 = bq->a2;
            b0 = bq->b0 * bq->gain;
            b1 = bq->b1 * bq->gain;
            b2 = bq->b2 * bq->gain;
        }
        for (uint8_t j = 0; j < 5; j++) {
            uint32_t data;
            switch (j) {
                case 0:
                    data = b0 * (1UL << 20);
                    break;
                case 1:
                    data = b1 * (1UL << 20);
                    break;
                case 2:
                    data = b2 * (1UL << 20);
                    break;
                case 3:
                    data = a1 * (1UL << 20);
                    break;
                case 4:
                    data = a2 * (1UL << 20);
                    break;
            }
            for (uint8_t k = 0; k < 3; k++) {
                uint8_t shift = 16 - (k * 8);
                if ((reg >= 0x46) && (reg <= 0xAE)) {
                    // ^tiny safety net
                    max98091_check(reg, (data >> shift) & 0xFF);
                }
                reg++;
            }
        }
    }
    max98091_check(0x41, 1);
}

void flow3r_bsp_max98091_register_poke(uint8_t reg, uint8_t data) {
    max98091_check(reg, data);
}

static void onboard_mic_set_power(bool enable) {
    if (enable) {
        // XXX: The codec outputs a DC bias on th speakers
        // if the microphone clock is active and the hardware through
        // is active. Make sure this can't happen.
        flow3r_bsp_max98091_speaker_line_in_set_hardware_thru(false);
    }

    // DMICCLK = 3 => f_DMC = f_PCLK / 5
    max98091_check(MAX98091_DIGITAL_MIC_ENABLE,
                   MAX98091_BITS(DIGITAL_MIC_ENABLE_DMICCLK, 3) |
                       MAX98091_BOOL(DIGITAL_MIC_ENABLE_DIGMIC1L, enable) |
                       MAX98091_BOOL(DIGITAL_MIC_ENABLE_DIGMIC1R, enable));
}

void flow3r_bsp_max98091_headphones_line_in_set_hardware_thru(bool enable) {
    if (enable) {
        // XXX: The codec outputs a DC bias on th speakers
        // if the microphone clock is active and the hardware through
        // is active. Make sure this can't happen.
        // TODO(schneider): Not sure if this actually applies to the
        // HP through as well. Don't want to chances now.
        onboard_mic_set_power(false);
    }

    // Enable headphone mixer.
    max98091_check(MAX98091_HPCONTROL, MAX98091_ON(HPCONTROL_MIXHP_RSEL) |
                                           MAX98091_ON(HPCONTROL_MIXHP_LSEL));
    // Line A + DAC_L -> Left HP
    max98091_check(MAX98091_LEFT_HP_MIXER,
                   MAX98091_BOOL(LEFT_HP_MIXER_LINE_A, enable) |
                       MAX98091_ON(LEFT_HP_MIXER_LEFT_DAC));
    // Line B + DAC_R -> Right HP
    max98091_check(MAX98091_RIGHT_HP_MIXER,
                   MAX98091_BOOL(RIGHT_HP_MIXER_LINE_B, enable) |
                       MAX98091_ON(RIGHT_HP_MIXER_RIGHT_DAC));
}

void flow3r_bsp_max98091_speaker_line_in_set_hardware_thru(bool enable) {
    if (enable) {
        // XXX: The codec outputs a DC bias on th speakers
        // if the microphone clock is active and the hardware through
        // is active. Make sure this can't happen.
        onboard_mic_set_power(false);
    }

    // The badge speakers are not stereo so we don't care about channel
    // assignment here Line A -> Speaker 1
    max98091_check(MAX98091_LEFT_SPK_MIXER,
                   MAX98091_BOOL(LEFT_SPK_MIXER_LINE_A, enable) |
                       MAX98091_ON(LEFT_SPK_MIXER_LEFT_DAC));
    // Line B -> Speaker 2
    max98091_check(MAX98091_RIGHT_SPK_MIXER,
                   MAX98091_BOOL(RIGHT_SPK_MIXER_LINE_B, enable) |
                       MAX98091_ON(RIGHT_SPK_MIXER_RIGHT_DAC));
}

void flow3r_bsp_max98091_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_max98091_speaker_line_in_set_hardware_thru(enable);
    flow3r_bsp_max98091_headphones_line_in_set_hardware_thru(enable);
}

void flow3r_bsp_max98091_input_set_source(
    flow3r_bsp_audio_input_source_t source) {
    switch (source) {
        case flow3r_bsp_audio_input_source_none:
            onboard_mic_set_power(false);
            max98091_check(MAX98091_LEFT_ADC_MIXER, 0);
            max98091_check(MAX98091_RIGHT_ADC_MIXER, 0);
            break;
        case flow3r_bsp_audio_input_source_line_in:
            onboard_mic_set_power(false);
            max98091_check(MAX98091_LEFT_ADC_MIXER,
                           MAX98091_ON(LEFT_ADC_MIXER_LINE_IN_A));
            max98091_check(MAX98091_RIGHT_ADC_MIXER,
                           MAX98091_ON(RIGHT_ADC_MIXER_LINE_IN_B));
            break;
        case flow3r_bsp_audio_input_source_headset_mic:
            onboard_mic_set_power(false);
            max98091_check(MAX98091_LEFT_ADC_MIXER,
                           MAX98091_ON(LEFT_ADC_MIXER_MICROPHONE_1));
            max98091_check(MAX98091_RIGHT_ADC_MIXER,
                           MAX98091_ON(RIGHT_ADC_MIXER_MICROPHONE_1));
            break;
        case flow3r_bsp_audio_input_source_onboard_mic:
            onboard_mic_set_power(true);
            max98091_check(MAX98091_LEFT_ADC_MIXER, 0);
            max98091_check(MAX98091_RIGHT_ADC_MIXER, 0);
            break;
    }
}

int8_t flow3r_bsp_max98091_headset_set_gain_dB(int8_t gain_dB) {
    if (gain_dB > 50) gain_dB = 50;
    if (gain_dB < 0) gain_dB = 0;
    uint8_t g = gain_dB;

    uint8_t pa1en = 0b01;
    if (g > 30) {
        g -= 30;
        pa1en = 0b11;
    } else if (g > 20) {
        g -= 20;
        pa1en = 0b10;
    }
    max98091_check(MAX98091_MIC1_INPUT_LEVEL,
                   MAX98091_BITS(MIC1_INPUT_LEVEL_PGAM1, 0x14 - g) |
                       MAX98091_BITS(MIC1_INPUT_LEVEL_PA1EN, pa1en));
    return gain_dB;
}

void flow3r_bsp_max98091_read_jacksense(
    flow3r_bsp_audio_jacksense_state_t *st) {
    st->line_in = flow3r_bsp_spio_jacksense_right_get();

    uint8_t jck = max98091_read(MAX98091_JACK_STATUS);
    switch (jck) {
        case 6:
            st->headphones = false;
            st->headset = false;
            return;
        case 0:
            st->headphones = true;
            st->headset = false;
            return;
        case 2:
            st->headphones = true;
            st->headset = true;
    }
}

typedef struct {
    uint8_t raw_value;
    float volume_dB;
} volume_step_t;

static const uint8_t speaker_map_len = 40;
static const volume_step_t speaker_map[] = {
    { 0x3F, +14 }, { 0x3E, +13.5 }, { 0x3D, +13 }, { 0x3C, +12.5 },
    { 0x3B, +12 }, { 0x3A, +11.5 }, { 0x39, +11 }, { 0x38, +10.5 },
    { 0x37, +10 }, { 0x36, +9.5 },  { 0x35, +9 },  { 0x34, +8 },
    { 0x33, +7 },  { 0x32, +6 },    { 0x31, +5 },  { 0x30, +4 },
    { 0x2F, +3 },  { 0x2E, +2 },    { 0x2D, +1 },  { 0x2C, +0 },
    { 0x2B, -1 },  { 0x2A, -2 },    { 0x29, -3 },  { 0x28, -4 },
    { 0x27, -5 },  { 0x26, -6 },    { 0x25, -8 },  { 0x24, -10 },
    { 0x23, -12 }, { 0x22, -14 },   { 0x21, -17 }, { 0x20, -20 },
    { 0x1F, -23 }, { 0x1E, -26 },   { 0x1D, -29 }, { 0x1C, -32 },
    { 0x1B, -36 }, { 0x1A, -40 },   { 0x19, -44 }, { 0x18, -48 }
};

static const volume_step_t *find_speaker_volume(float vol_dB) {
    uint8_t map_index = speaker_map_len - 1;
    for (; map_index; map_index--) {
        if (speaker_map[map_index].volume_dB >= vol_dB)
            return &speaker_map[map_index];
    }
    return &speaker_map[0];
}

static const uint8_t headphones_map_len = 32;
static const volume_step_t headphones_map[] = {
    { 0x1F, +3 },  { 0x1E, +2.5 }, { 0x1D, +2 },  { 0x1C, +1.5 }, { 0x1B, +1 },
    { 0x1A, +0 },  { 0x19, -1 },   { 0x18, -2 },  { 0x17, -3 },   { 0x16, -4 },
    { 0x15, -5 },  { 0x14, -7 },   { 0x13, -9 },  { 0x12, -11 },  { 0x11, -13 },
    { 0x10, -15 }, { 0x0F, -17 },  { 0x0E, -19 }, { 0x0D, -22 },  { 0x0C, -25 },
    { 0x0B, -28 }, { 0x0A, -31 },  { 0x09, -34 }, { 0x08, -37 },  { 0x07, -40 },
    { 0x06, -43 }, { 0x06, -47 },  { 0x04, -51 }, { 0x03, -55 },  { 0x02, -59 },
    { 0x01, -63 }, { 0x00, -67 }
};

static const volume_step_t *find_headphones_volume(float vol_dB) {
    uint8_t map_index = headphones_map_len - 1;
    for (; map_index; map_index--) {
        if (headphones_map[map_index].volume_dB >= vol_dB)
            return &headphones_map[map_index];
    }
    return &headphones_map[0];
}

float flow3r_bsp_max98091_headphones_set_volume(bool mute, float dB) {
    const volume_step_t *step = find_headphones_volume(dB);
    ESP_LOGI(TAG, "Setting headphones volume: %d/%02x", mute, step->raw_value);
    max98091_check(MAX98091_LEFT_HP_VOLUME,
                   MAX98091_BOOL(LEFT_HP_VOLUME_HPLM, mute) |
                       MAX98091_BITS(LEFT_HP_VOLUME_HPVOLL, step->raw_value));
    max98091_check(MAX98091_RIGHT_HP_VOLUME,
                   MAX98091_BOOL(RIGHT_HP_VOLUME_HPRM, mute) |
                       MAX98091_BITS(RIGHT_HP_VOLUME_HPVOLR, step->raw_value));
    return step->volume_dB;
}

float flow3r_bsp_max98091_speaker_set_volume(bool mute, float dB) {
    const volume_step_t *step = find_speaker_volume(dB);
    ESP_LOGI(TAG, "Setting speakers volume: %d/%02x", mute, step->raw_value);
    max98091_check(MAX98091_LEFT_SPK_VOLUME,
                   MAX98091_BOOL(LEFT_SPK_VOLUME_SPLM, mute) |
                       MAX98091_BITS(LEFT_SPK_VOLUME_SPVOLL, step->raw_value));
    max98091_check(MAX98091_RIGHT_SPK_VOLUME,
                   MAX98091_BOOL(RIGHT_SPK_VOLUME_SPRM, mute) |
                       MAX98091_BITS(RIGHT_SPK_VOLUME_SPVOLR, step->raw_value));
    return step->volume_dB;
}
void flow3r_bsp_max98091_init(void) {
#define POKE(n, v) ESP_ERROR_CHECK(max98091_check(MAX98091_##n, v))
    ESP_LOGI(TAG, "Codec initializing...");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    POKE(SOFTWARE_RESET, 0x80);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    POKE(DEVICE_SHUTDOWN, 0);
    // pclk = mclk / 1
    POKE(SYSTEM_CLOCK, MAX98091_BITS(SYSTEM_CLOCK_PSCLK, 1));
    // music, dc filter in record and playback
    POKE(FILTER_CONFIGURATION, (1 << 7) | (1 << 6) | (1 << 5) | (1 << 2));
    // Sets up DAI for left-justified slave mode operation.
    POKE(DAI_INTERFACE, 1 << 2);
    // Sets up the DAC to speaker path
    POKE(DAC_PATH, 1 << 5);

    // Somehow this was needed to get an input signal to the ADC, even though
    // all other registers should be taken care of later. Don't know why.
    // Sets up the line in to adc path
    POKE(LINE_TO_ADC, 1 << 6);

    // SDOUT, SDIN enabled
    POKE(IO_CONFIGURATION, (1 << 1) | (1 << 0));
    // bandgap bias
    POKE(BIAS_CONTROL, 1 << 0);
    // high performane mode
    POKE(DAC_CONTROL, 1 << 0);

    // enable micbias, line input amps, ADCs
    POKE(INPUT_ENABLE, (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
    // IN3 SE -> Line A, IN4 SE -> Line B
    POKE(LINE_INPUT_CONFIG, (1 << 3) | (1 << 2));
    // 64x oversampling, dithering, high performance ADC
    POKE(ADC_CONTROL, (1 << 1) | (1 << 0));

    POKE(DIGITAL_MIC_ENABLE, 0);
    // IN5/IN6 to MIC1
    POKE(INPUT_MODE, (1 << 0));

    flow3r_bsp_max98091_line_in_set_hardware_thru(0);
    flow3r_bsp_max98091_headset_set_gain_dB(0);
    flow3r_bsp_max98091_input_set_source(flow3r_bsp_audio_input_source_none);

    // output enable: enable dacs
    POKE(OUTPUT_ENABLE, (1 << 1) | (1 << 0));
    // power up
    POKE(DEVICE_SHUTDOWN, 1 << 7);
    // enable outputs, dacs
    POKE(OUTPUT_ENABLE,
         (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 1) | (1 << 0));
    // disable all digital filters except for dc blocking
    POKE(DSP_FILTER_ENABLE, 0x0);
    // jack detect enable
    POKE(JACK_DETECT, 1 << 7);

    // TODO(q3k): mute this
    ESP_LOGI(
        TAG,
        "Codec initilialied! Don't worry about the readback errors above.");
#undef POKE
}
