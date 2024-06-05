#include "flow3r_bsp_ad7147.h"
#include "flow3r_bsp_ad7147_hw.h"
#include "flow3r_bsp_captouch.h"

#include "esp_err.h"
#include "esp_log.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "flow3r-bsp-ad7147";
static const int32_t _calib_target = 6000;
static const int32_t _calib_incr_cap = 1000;

#define COMPLAIN(c, ...)                \
    do {                                \
        if (!c->failed) {               \
            ESP_LOGE(TAG, __VA_ARGS__); \
            c->failed = true;           \
        }                               \
    } while (0)

// Length of sequence, assuming sequence is right-padded with -1.
static size_t _captouch_sequence_length(int8_t *sequence) {
    for (size_t i = 0; i < 12; i++) {
        if (sequence[i] == -1) {
            return i;
        }
    }
    return 12;
}

// Request current sequence from captouch chip.
static esp_err_t _sequence_request(ad7147_chip_t *chip, bool reprogram) {
    int8_t *seq = chip->sequences[chip->seq_position];
    ad7147_sequence_t seq_out = {
        .len = _captouch_sequence_length(seq),
    };
    for (size_t i = 0; i < seq_out.len; i++) {
        int8_t channel = seq[i];
        int8_t offset = chip->channels[channel].afe_offset;
        seq_out.channels[i] = channel;
        // seq_out.idle_to_bias[i] = !(chip->is_bot && (channel < 10)); // jumpy
        // petal 2
        seq_out.idle_to_bias[i] = !chip->is_bot;
        if (offset < 63) {
            seq_out.neg_afe_offsets[i] = offset;
            seq_out.pos_afe_offsets[i] = 0;
        } else {
            seq_out.neg_afe_offsets[i] = 63;
            seq_out.pos_afe_offsets[i] = offset - 63;
        }
    }

    esp_err_t ret;
    if ((ret = ad7147_hw_configure_stages(&chip->dev, &seq_out, reprogram)) !=
        ESP_OK) {
        return ret;
    }
    return ESP_OK;
}

// Advance internal sequence pointer to next sequence. Returns true if advance
// occurred (in other words, false when there is only one sequence).
static bool _sequence_advance(ad7147_chip_t *chip) {
    // Advance to next sequence.
    size_t start = chip->seq_position;
    chip->seq_position++;
    if (chip->seq_position >= _AD7147_SEQ_MAX) {
        chip->seq_position = 0;
    } else {
        if (_captouch_sequence_length(chip->sequences[chip->seq_position]) ==
            0) {
            chip->seq_position = 0;
        }
    }
    return start != chip->seq_position;
}

// Queue a calibration request until the next _chip_process call picks it up.
static void _calibration_request(ad7147_chip_t *chip) {
    chip->calibration_pending = true;
}

static int _uint16_sort(const void *va, const void *vb) {
    uint16_t a = *((uint16_t *)va);
    uint16_t b = *((uint16_t *)vb);
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return 1;
    }
    return 0;
}

// Calculate median of 16 measurements.
static uint16_t _average_calib_measurements(uint16_t *measurements) {
    qsort(measurements, _AD7147_CALIB_CYCLES, sizeof(uint16_t), _uint16_sort);
    size_t ix = _AD7147_CALIB_CYCLES / 2;
    return measurements[ix];
}

static size_t _channel_from_readout(const ad7147_chip_t *chip, size_t ix) {
    size_t six = chip->seq_position;
    const int8_t *seq = chip->sequences[six];
    assert(seq[ix] >= 0);
    return seq[ix];
}

// Check if a channel's AFE offset can be tweaked to reach a wanted amb value.
// True is returned if a tweak was performed, false otherwise.
static bool _channel_afe_tweak(ad7147_chip_t *chip, size_t cix) {
    int32_t cur = chip->channels[cix].amb;
    int32_t target = _calib_target;
    int32_t diff = (cur - target) / _calib_incr_cap;
    if (diff < 1 && diff > -1) {
        // Close enough.
        return false;
    }
    int32_t offset = chip->channels[cix].afe_offset;
    if (offset <= 0 && diff < 0) {
        // Saturated, can't do anything.
        return false;
    }
    if (offset >= 126 && diff > 0) {
        // Saturated, can't do anything.
        return false;
    }

    offset += diff;
    if (offset < 0) {
        offset = 0;
    }
    if (offset > 126) {
        offset = 126;
    }

#if 0
    // keep around for debugging pls~
    int32_t old_offset = chip->channels[cix].afe_offset;
    int32_t chan = cix;
    ESP_LOGE(TAG, "%s, channel %ld: new afe: %ld, old afe: %ld, reading: %ld", chip->name, chan,
    offset, old_offset, cur);
#endif

    chip->channels[cix].afe_offset = offset;
    return true;
}

// Called when a sequence is completed by the low-level layer.
static void _on_data(void *user, uint16_t *data, size_t len) {
    ad7147_chip_t *chip = (ad7147_chip_t *)user;

    if (chip->calibration_cycles > 0) {
        // We're doing a calibration cycle on our channels. Instead of writing
        // the data to channel->cdc, write it to channel->amb_meas.
        int8_t j = chip->calibration_cycles - 1;
        if (j < _AD7147_CALIB_CYCLES) {  // throw away first few datapoints
            for (int8_t i = 0; i < len; i++) {
                chip->channels[_channel_from_readout(chip, i)].amb_meas[j] =
                    data[i];
            }
        }
    } else {
        // Normal measurement, apply to channel->cdc.
        for (size_t i = 0; i < len; i++) {
            chip->channels[_channel_from_readout(chip, i)].cdc = data[i];
        }
    }

    bool reprogram = _sequence_advance(chip) || chip->calibration_external;

    // Synchronize on beginning of sequence for calibration logic.
    if (chip->seq_position == 0) {
        // Deal with calibration pending flag, possibly starting calibration.
        if (chip->calibration_pending) {
            if (!chip->calibration_active) {
                ESP_LOGI(TAG, "%s: calibration starting...", chip->name);
                chip->calibration_cycles = _AD7147_CALIB_CYCLES + 2;
                chip->calibration_active = true;
            }
            chip->calibration_pending = false;
        }

        if (chip->calibration_active) {
            // Deal with active calibration.
            chip->calibration_cycles--;
            if (chip->calibration_cycles <= 0) {
                // Calibration measurements done. Calculate average amb data for
                // each channel.
                for (size_t i = 0; i < chip->nchannels; i++) {
#if 1
                    uint16_t avg =
                        _average_calib_measurements(chip->channels[i].amb_meas);
                    chip->channels[i].amb = avg;
#else
                    // TODO: compare performance
                    uint32_t avg = 0;
                    for (uint8_t j = 0; j < _AD7147_CALIB_CYCLES; j++) {
                        avg += chip->channels[i].amb_meas[j];
                    }
                    chip->channels[i].amb = avg / _AD7147_CALIB_CYCLES;
#endif
                }

                char msg[256];
                char *wr = msg;
                for (size_t i = 0; i < chip->nchannels; i++) {
                    if (wr != msg) {
                        wr += snprintf(wr, 256 - (wr - msg), ", ");
                    }
                    wr += snprintf(wr, 256 - (wr - msg), "%04d/%02d",
                                   chip->channels[i].amb,
                                   chip->channels[i].afe_offset);
                }
                ESP_LOGD(TAG, "%s: calibration: %s.", chip->name, msg);

                // Can we tweak the AFE to get a better measurement?
                uint16_t rerun = 0;
                for (size_t i = 0; i < chip->nchannels; i++) {
                    if (_channel_afe_tweak(chip, i)) {
                        rerun |= (1 << i);
                    }
                }
                if (rerun != 0) {
                    // Rerun calibration again,
                    ESP_LOGI(TAG,
                             "%s: calibration cycle complete, but can do "
                             "better (%04x). "
                             "Retrying.",
                             chip->name, rerun);
                    chip->calibration_cycles = _AD7147_CALIB_CYCLES + 2;
                    reprogram = true;
                } else {
                    chip->calibration_active = false;
                    ESP_LOGI(TAG, "%s: calibration done.", chip->name);
                }
            }
        } else {
            // Submit data to higher level for processing.
            if (chip->callback != NULL) {
                uint16_t val[13];
                for (size_t i = 0; i < chip->nchannels; i++) {
                    int32_t cdc = chip->channels[i].cdc;
                    int32_t amb = chip->channels[i].amb;
                    int32_t diff = cdc - amb;
                    if (diff < 0) {
                        val[i] = 0;
                    } else if (diff > 65535) {
                        val[i] = 65535;
                    } else {
                        val[i] = diff;
                    }
                }
                chip->callback(chip->user, val, chip->nchannels);
            }
        }
    }

    // _sequence_request also writes the AFE registers which just got tweaked
    if (reprogram) {
        esp_err_t ret;
        if ((ret = _sequence_request(chip, reprogram)) != ESP_OK) {
            COMPLAIN(chip, "%s: requesting next sequence failed: %s",
                     chip->name, esp_err_to_name(ret));
        }
        if (chip->calibration_external) {
            chip->calibration_external = false;
            ESP_LOGI(TAG, "%s: captouch calibration updated", chip->name);
        }
    }
}

esp_err_t flow3r_bsp_ad7147_chip_init(ad7147_chip_t *chip,
                                      flow3r_i2c_address address) {
    esp_err_t ret;
    for (size_t i = 0; i < chip->nchannels; i++) {
        chip->channels[i].amb = 0;
    }
    if ((ret = ad7147_hw_init(&chip->dev, address, _on_data, chip)) != ESP_OK) {
        return ret;
    }
    _calibration_request(chip);
    if ((ret = _sequence_request(chip, false)) != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t flow3r_bsp_ad7147_chip_process(ad7147_chip_t *chip) {
    return ad7147_hw_process(&chip->dev);
}
