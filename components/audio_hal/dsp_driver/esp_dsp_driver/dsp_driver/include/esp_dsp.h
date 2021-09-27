// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdlib.h>
#include <driver/i2s.h>

#define SAMPLE_RATE_DEFAULT 48000UL
#define CHANNELS_DEFAULT 2
#define I2S_NUMBER_DEFAULT I2S_NUM_0

#define ESP_DSP_CONFIG_DEFAULT()            \
    {                                       \
        .sample_rate = SAMPLE_RATE_DEFAULT, \
        .channels = CHANNELS_DEFAULT,       \
        .i2s_number = I2S_NUMBER_DEFAULT,   \
        .set_i2s_clk = true,                \
    }

typedef struct esp_dsp_config {
    int sample_rate;    /* Typically 48K / 16K etc. */
    int channels;       /* 1 or 2 */
    int i2s_number;     /* 0 or 1 */
    bool set_i2s_clk;
} esp_dsp_config_t;

esp_err_t esp_dsp_audio_stream_pause();
esp_err_t esp_dsp_audio_stream_resume();
void esp_dsp_configure(esp_dsp_config_t *cfg);
void esp_dsp_init();
