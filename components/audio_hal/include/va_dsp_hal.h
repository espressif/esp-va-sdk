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

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define DSP_NVS_NAMESPACE "dsp"

/*
 * API to configure dsp
 */
esp_err_t va_dsp_hal_configure(void *config);

/**
 * Typically, the initialisation will take a backup of the Queue handle.
 *
 * Later, whenever the DSP detects a Wake Word, it will send an event
 * to this queue as follows:
 *     struct dsp_event_data event_data = {
 *         .event = WW
 *     };
 *
 * On receiving this event, the va_dsp thread will later on query the
 * WW len by making a call to custom_dsp_get_ww_len() that should also
 * be implemented.
 */
esp_err_t va_dsp_hal_init(QueueHandle_t queue);

/*
 * API to reset dsp
 */
esp_err_t va_dsp_hal_reset();

/**
 * API to put dsp in low power mode
 * @note: Implementation of this api is optional and doesn't affect the Alexa Functionality
 */
esp_err_t va_dsp_hal_enter_low_power();

/*
 * Initilize acoustic echo cancellation on dsp
 */
esp_err_t va_dsp_hal_exit_low_power();

/*
 * Put DSP in Mute state(Microphones to be shut off)
 */
esp_err_t va_dsp_hal_mic_mute();

/*
 * Disable Mic Mute state, Microphones to be turned on
 */
esp_err_t va_dsp_hal_mic_unmute();

/*
 * API to indicate dsp to start sending microphone data
 */
esp_err_t va_dsp_hal_tap_to_talk();

/**
 * API to indicate dsp to start sending microphone data
 * @note This API is same as custom_dsp_tap_to_talk();
 */
esp_err_t va_dsp_hal_start_capture();

/*
 * API to indicate dsp to stop sending microphone data
 */
esp_err_t va_dsp_hal_stop_capture();

/*
 * API to get 'ALEXA' phrase length, returns phrase length in bytes
 */
size_t va_dsp_hal_get_ww_len();

/**
 * @brief   few milliseconds of preroll.
 */
esp_err_t va_dsp_hal_get_preroll(void *data);

/**
 * API for dsp to send audio data, should return number of bytes read from the DSP audio buffer
 * @note Audio data format is 16KHz, 16BIT, MonO Channel. Little Endian
 */
int va_dsp_hal_stream_audio(uint8_t *buffer, int size, int wait);

/**
 * API to pause the i2s audio_stream. This can be used before uninstalling i2s.
 * This might be useful for half-duplex boards.
 */
esp_err_t va_dsp_hal_stream_pause();

/**
 * API to resume the i2s audio_stream. This can be used after installing i2s again.
 * This might be useful for half-duplex boards.
 */
esp_err_t va_dsp_hal_stream_resume();
