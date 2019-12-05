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
#include "zl38063.h"
#include "tw_spi_access.h"
#include <media_hal_codec_init.h>

#define HAL_TAG "MEDIA_HAL_CODEC_INIT"

#define mutex_create() \
            xSemaphoreCreateMutex()

#define mutex_lock(x) \
            xSemaphoreTake(x, portMAX_DELAY)

#define mutex_unlock(x) \
            xSemaphoreGive(x)

#define mutex_destroy(x) \
            vSemaphoreDelete(x)

static void media_hal_func_init(media_hal_t* media_hal)
{
    media_hal->audio_codec_initialize = zl38063_init;
    media_hal->audio_codec_deinitialize = zl38063_deinit;
    media_hal->audio_codec_set_state = zl38063_set_state;
    media_hal->audio_codec_set_i2s_clk = zl38063_set_i2s_clk;
    media_hal->audio_codec_config_format = zl38063_config_format;
    media_hal->audio_codec_control_volume = zl38063_control_volume;
    media_hal->audio_codec_get_volume = zl38063_get_volume;
    media_hal->audio_codec_set_mute = zl38063_set_mute;
    media_hal->audio_codec_powerup = zl38063_powerup;
    media_hal->audio_codec_powerdown = zl38063_powerdown;
}

esp_err_t media_hal_codec_init(media_hal_t *media_hal, media_hal_config_t *media_hal_conf)
{
    esp_err_t ret = ESP_FAIL;
    if (media_hal && media_hal_conf) {
        zl38063_init_firmware(2);
        media_hal_func_init(media_hal);
        mutex_lock(media_hal->media_hal_lock);
        ret  = media_hal->audio_codec_initialize(media_hal_conf);
        ret |= media_hal->audio_codec_config_format(media_hal_conf->codec_mode, 0);
        ret |= media_hal->audio_codec_set_i2s_clk(media_hal_conf->codec_mode, media_hal_conf->bit_length);
        ret |= media_hal->audio_codec_control_volume(MEDIA_HAL_VOL_DEFAULT);
        mutex_unlock(media_hal->media_hal_lock);
    } else {
        ESP_LOGW(HAL_TAG, "Codec handle or config is NULL");
    }
    return ret;
}