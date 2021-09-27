/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/i2s.h>
#include <esp_err.h>

/**
 * Number of supported eq bands.
 */
#define MEDIA_HAL_EQ_BANDS 10

#define DEFAULT_MEDIA_HAL_PLAYBACK_CONFIG() {       \
    .channels = 2,                                  \
    .sample_rate = 48000,                           \
    .i2s_port_num = I2S_NUM_0,                      \
    .bits_per_sample = 16,                          \
}

/**
 * Structure holding media_playback characteristics.
 * Given as parameter to `media_hal_init_playback` function call, this decides output audio characteristics.
 */
typedef struct {
    int sample_rate; /** audio sample_rate in Hz */
    int channels;    /** Number of channels. (1 or 2)*/
    int bits_per_sample; /** Audio sample size. (This typically is 16 or 32) */
    int i2s_port_num;    /** Board specific port number */

    /**
     * Optional custom function pointer.
     * Returns: number of bytes written.
     */
    int (*write_callback) (int /* port */, void * /* buffer */, size_t /* buf size */,
                           int /* src_bps */, int /* dst_bps */);

    /**
     * Optional custom equalizer processor.
     *
     * If this function is provided, this will be called to do equalizer processing.
     * The function should take care of all the band value setting, enabling or disabling equalizer etc.
     *
     * Return: Number of bytes processed.
     */
    int (*equalizer_callback)(char *buffer /* in_place in/out buffer*/, int len /* length in bytes */,
                              int sample_rate /* sample rate of data in buffer */, int channels /* number of channels*/);
} media_hal_playback_cfg_t;

/**
 * Initialize media_hal_playback settings.
 *
 * Appropriate cfg should be passed to this function.
 * Config has parameters viz., sample_rate, channels, bits_per_sample, port number.
 * Also Following optional callback functions could be provided:
 *      `write_callback` could be set to make it call custom playback writer.
 *      `equalizer_callback` could be set to make it call custom equalizer.
 *
 * Return: handle for inited handle.
 *
 * Note: This function is typically called from i2s init of board, bluetooth init etc.
 *       By default, once inited media hal is enabled. To disable use `media_hal_disable_playback`.
 */
void *media_hal_init_playback(media_hal_playback_cfg_t *cfg);

/**
 * Enable media_hal_playback for a handle.
 * `playback_handle` is the handle returned by `media_hal_init_playback`
 *
 * Return: ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t media_hal_enable_playback(void *playback_handle);

/**
 * Disable media_hal_playback for a handle.
 * `playback_handle` is the handle returned by `media_hal_init_playback`
 *
 * Return: ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t media_hal_disable_playback(void *playback_handle);

/**
 * Structure holding characteristics of data to be played.
 * Must be provided to `media_hal_playback` call with data buffer and buffer length.
 */
typedef struct {
    int sample_rate;
    int channels;
    int bits_per_sample;
} media_hal_audio_info_t;

/**
 * Playback provided data.
 *
 * Plays data provided in `buf` with length `len`.
 * Characteristics of the audio are provided in audio_info.
 *
 * Return: Number of bytes written.
 */
int media_hal_playback(media_hal_audio_info_t *audio_info, void *buf, int len);

/**
 * Enable audio equalizer.
 *
 * Equalizes as per given gain value array.
 *
 * Return: ESP_OK on success, ESP_FAIL on error.
 *
 * Note: If `equalizer_callback` is provided, this has no effect.
 */
esp_err_t media_hal_enable_equalizer();

/**
 * Disable audio equalizer.
 *
 * Audio will not be equalized, freq won't be altered.
 *
 * Note: If `equalizer_callback` is provided, this has no effect.
 */
void media_hal_disable_equalizer();

/**
 * Initialize gain values for equalizer.
 *
 * 10 gain values are given to the equalizer. Equalizer will apply effects using these values.
 * Can be called anytime when equalizer is running/enabled state.
 * Same will be used for both the channels.
 *
 * Return: ESP_OK on success, ESP_FAIL on error.
 * Note: 1. Equalizer must be enabled first using `media_hal_enable_equalizer` before setting gain values.
 *       2. If `equalizer_callback` is provided, this has no effect.
 */
esp_err_t media_hal_equalizer_set_band_vals(const int8_t gain_vals[MEDIA_HAL_EQ_BANDS]);
