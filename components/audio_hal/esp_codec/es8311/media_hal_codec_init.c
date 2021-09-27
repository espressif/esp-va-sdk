/*
*
* Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include <esp_codec.h>
#include <media_hal_codec_init.h>

#define HAL_TAG "MEDIA_HAL_CODEC_INIT"

static void media_hal_func_init(media_hal_t* media_hal)
{
    media_hal->audio_codec_initialize = es8311_init;
    media_hal->audio_codec_deinitialize = es8311_deinit;
    media_hal->audio_codec_set_state = es8311_ctrl_state;
    media_hal->audio_codec_set_i2s_clk = es8311_set_bits_per_sample;
    media_hal->audio_codec_config_format = es8311_config_format;
    media_hal->audio_codec_control_volume = es8311_set_volume;
    media_hal->audio_codec_get_volume = es8311_get_volume;
    media_hal->audio_codec_set_mute = es8311_set_mute;
    media_hal->audio_codec_powerup = es8311_powerup;
    media_hal->audio_codec_powerdown = es8311_powerdown;
}

esp_err_t media_hal_codec_init(media_hal_t *media_hal, media_hal_config_t *media_hal_conf)
{
    esp_err_t ret = ESP_FAIL;
    if (media_hal && media_hal_conf) {
        media_hal_func_init(media_hal);
        ret  = media_hal->audio_codec_initialize(media_hal_conf);
        ret |= media_hal->audio_codec_config_format(media_hal_conf->codec_mode, 0);
        ret |= media_hal->audio_codec_set_i2s_clk(media_hal_conf->codec_mode, media_hal_conf->bit_length);
        ret |= media_hal->audio_codec_set_state(media_hal_conf->codec_mode, MEDIA_HAL_START_STATE);
        ret |= media_hal->audio_codec_control_volume(MEDIA_HAL_VOL_DEFAULT);
    } else {
        ESP_LOGW(HAL_TAG, "Codec handle or config is NULL");
    }
    return ret;
}
