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
#include "es8388.h"
#include <media_hal.h>

#define HAL_TAG "MEDIA_HAL"

#define mutex_create() \
            xSemaphoreCreateMutex()

#define mutex_lock(x) \
            xSemaphoreTake(x, portMAX_DELAY)

#define mutex_unlock(x) \
            xSemaphoreGive(x)

#define mutex_destroy(x) \
            vSemaphoreDelete(x)

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }
uint8_t media_hal_port_num;
static media_hal_t *media_hal_handle;
static bool isfirst = true;

static uint8_t volume_prv;

static void media_hal_func_init(media_hal_t* media_hal)
{
    media_hal->audio_codec_initialize = es8388_init;
    media_hal->audio_codec_deinitialize = es8388_deinit;
    media_hal->audio_codec_set_state = es8388_set_state;
    media_hal->audio_codec_set_i2s_clk = es8388_set_i2s_clk;
    media_hal->audio_codec_config_format = es8388_config_format;
    media_hal->audio_codec_control_volume = es8388_control_volume;
    media_hal->audio_codec_get_volume = es8388_get_volume;
    media_hal->audio_codec_set_mute = es8388_set_mute;
    media_hal->audio_codec_powerup = es8388_powerup;
    media_hal->audio_codec_powerdown = es8388_powerdown;
}

media_hal_t* media_hal_init(media_hal_config_t *media_hal_conf)
{
    if (isfirst) {
        esp_err_t ret  = 0;
        media_hal_t *media_hal = (media_hal_t *) calloc(1, sizeof(media_hal_t));
        media_hal->media_hal_lock = mutex_create();
        assert(media_hal->media_hal_lock);
        media_hal_func_init(media_hal);
        mutex_lock(media_hal->media_hal_lock);
        ret  = media_hal->audio_codec_initialize(media_hal_conf->op_mode, media_hal_conf->adc_input, media_hal_conf->dac_output, media_hal_conf->port_num);
        ret |= media_hal->audio_codec_config_format(media_hal_conf->codec_mode, 0);
        ret |= media_hal->audio_codec_set_i2s_clk(media_hal_conf->codec_mode, media_hal_conf->bit_length);
        ret |= media_hal->audio_codec_control_volume(MEDIA_HAL_VOL_DEFAULT);
        // ret |= media_hal->audio_codec_powerdown();
        mutex_unlock(media_hal->media_hal_lock);
        volume_prv = MEDIA_HAL_VOL_DEFAULT;

        media_hal_handle = media_hal;
        isfirst = false;
    } else {
        ESP_LOGW(HAL_TAG, "Codec already initialized, returning initialized handle");
    }
    return media_hal_handle;
}

media_hal_t* media_hal_get_handle()
{
    if (!isfirst) {
        return media_hal_handle;
    } else {
        ESP_LOGE(HAL_TAG, "Media Hal not initialized");
        return NULL;
    }
}

esp_err_t media_hal_deinit(media_hal_t* media_hal)
{
    esp_err_t ret;
    mutex_destroy(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_deinitialize(media_hal_port_num);
    media_hal->media_hal_lock = NULL;
    free(media_hal);
    return ret;
}

esp_err_t media_hal_set_state(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ESP_LOGI(HAL_TAG, "Codec mode is %d", mode);
    ret = media_hal->audio_codec_set_state(mode, media_hal_state);
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_control_volume(media_hal_t* media_hal, uint8_t volume)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    if (volume == 0) {
        ret = es8388_set_mute(1);
    } else {
        ret = media_hal->audio_codec_control_volume(volume);
    }
    volume_prv = volume;
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_set_mute(media_hal_t* media_hal, bool mute)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_set_mute(mute);
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_get_volume(media_hal_t* media_hal, uint8_t *volume)
{
    esp_err_t ret;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
    mutex_lock(media_hal->media_hal_lock);
    //ret = media_hal->audio_codec_get_volume(volume);
    *volume = volume_prv;
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_config_format(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_config_format(mode, fmt);
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_set_clk(media_hal_t* media_hal, media_hal_codec_mode_t mode, uint32_t rate, media_hal_bit_length_t bits_per_sample)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_set_i2s_clk(mode, bits_per_sample);
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_powerup(media_hal_t* media_hal)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_powerup();
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_powerdown(media_hal_t* media_hal)
{
    esp_err_t ret;
    mutex_lock(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_powerdown();
    mutex_unlock(media_hal->media_hal_lock);
    return ret;
}
