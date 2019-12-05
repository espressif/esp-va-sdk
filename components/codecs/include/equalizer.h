// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _EQUALIZER_H_
#define _EQUALIZER_H_

#include "esp_err.h"
#include "audio_codec.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EQUALIZER_TASK_STACK       (4 * 1024)
#define EQUALIZER_TASK_PRIO        (5)

#define BUF_SIZE (100)
#define NUMBER_BAND (10)
#define USE_XMMS_ORIGINAL_FREQENT (0)

/**
 * @brief      Equalizer Configuration
 */
typedef struct equalizer {
    audio_codec_t base;
    void *eq_handle; /* Holds equalizer handle */
    int samplerate;  /*!< Audio sample rate (in Hz)*/
    int channel;     /*!< Number of audio channels (Mono=1, Dual=2) */
    int *set_gain;   /*!< Equalizer gain */

    int num_bands;
    int use_xmms_original_freqs;
    unsigned total_bytes;

    int16_t *buf;
} equalizer_t;

/**
 * @brief      Set the audio sample rate and the number of channels to be processed by the equalizer.
 *
 * @param      equalizer    Audio element handle
 * @param      gain_val     Gain values array for NUMBER_BAND bands for 2 channels. (2 * NUMBER_BAND values)
 * @param      rate         Audio sample rate
 * @param      ch           Audio channel
 *
 * @return
 *             ESP_OK
 *             ESP_FAIL
 */
esp_err_t equalizer_set_info(equalizer_t *equalizer, int gain_val[], int rate, int ch);

/**
 * @brief      Create an Audio Element handle that equalizes incoming data.
 *
 * @param      config  The configuration
 *
 * @return     The created audio element handle
 */
equalizer_t *equalizer_create();

esp_err_t equalizer_destroy(equalizer_t *codec);

#ifdef __cplusplus
}
#endif

#endif