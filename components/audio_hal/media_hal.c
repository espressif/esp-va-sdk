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
#include <media_hal.h>
#include <media_hal_codec_init.h>
#include <media_hal_playback.h>

#define HAL_TAG "MEDIA_HAL"

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

uint8_t media_hal_port_num;
static media_hal_t *media_hal_handle = NULL;

static uint8_t volume_prv; //This currently is needed due to bt case.

media_hal_t* media_hal_init(media_hal_config_t *media_hal_cfg, media_hal_playback_cfg_t *media_hal_playback_cfg)
{
    if (!media_hal_handle) {
        //esp_err_t ret  = 0;
        /* Initialize playback */
        if (media_hal_playback_cfg) {
            media_hal_init_playback(media_hal_playback_cfg);
        }

        /* Initialize codec */
        media_hal_t *media_hal = (media_hal_t *) calloc(1, sizeof(media_hal_t));
        media_hal->media_hal_lock = xSemaphoreCreateMutex();
        assert(media_hal->media_hal_lock);
        xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
        media_hal_codec_init(media_hal, media_hal_cfg);
        xSemaphoreGive(media_hal->media_hal_lock);
        volume_prv = MEDIA_HAL_VOL_DEFAULT;
        media_hal_handle = media_hal;
    } else {
        ESP_LOGW(HAL_TAG, "Codec already initialized, returning initialized handle");
    }
    return media_hal_handle;
}

media_hal_t* media_hal_get_handle()
{
    if (!media_hal_handle) {
        ESP_LOGE(HAL_TAG, "Media Hal not initialized");
    }
    return media_hal_handle;
}

esp_err_t media_hal_deinit(media_hal_t* media_hal)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    vSemaphoreDelete(media_hal->media_hal_lock);
    ret = media_hal->audio_codec_deinitialize(media_hal_port_num);
    media_hal->media_hal_lock = NULL;
    free(media_hal);
    media_hal_handle = NULL;
    return ret;
}

esp_err_t media_hal_set_state(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ESP_LOGI(HAL_TAG, "Codec mode is %d", mode);
    ret = media_hal->audio_codec_set_state(mode, media_hal_state);
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

/**
 * @brief Register volume change callback
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param callback volume change callback function to register
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_register_volume_change_cb(media_hal_t *media_hal, void (*callback) (int volume))
{
    int pos = 0;
    while (pos < VOLUME_CHANGE_CB_MAX) {
        if (media_hal->volume_change_notify_cb[pos]) {
            if (media_hal->volume_change_notify_cb[pos] == callback) {
                ESP_LOGW(HAL_TAG, "Already registered");
                return ESP_FAIL;
            }
            pos++;
        } else {
            break;
        }
    }

    if (pos < VOLUME_CHANGE_CB_MAX) {
        media_hal->volume_change_notify_cb[pos] = callback;
        return ESP_OK;
    }

    ESP_LOGW(HAL_TAG, "CB array is full!");
    return ESP_FAIL;
}

/**
 * @brief Deregister volume change callback
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param callback volume change callback function to deregister
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_deregister_volume_change_cb(media_hal_t *media_hal, void (*callback) (int volume))
{
    int pos = 0;
    while (pos < VOLUME_CHANGE_CB_MAX) {
        if (media_hal->volume_change_notify_cb[pos]) {
            if (media_hal->volume_change_notify_cb[pos] == callback) {
                media_hal->volume_change_notify_cb[pos] = NULL;
                break;
            }
        }
        pos++;
    }

    while (pos < VOLUME_CHANGE_CB_MAX - 1) {
        media_hal->volume_change_notify_cb[pos] = media_hal->volume_change_notify_cb[pos + 1];
        pos++;
    }
    return ESP_OK;
}

static esp_err_t media_hal_notify_volume_change(media_hal_t *media_hal, int volume)
{
    for (int i = 0; i < VOLUME_CHANGE_CB_MAX; i++) {
        if (media_hal->volume_change_notify_cb[i]) {
            media_hal->volume_change_notify_cb[i] (volume);
        } else {
            return ESP_OK;
        }
    }
    return ESP_OK;
}

static bool codec_mute = false; /* Intended mute */
static bool mute_by_bt = false; /* Mute is done by bt */

bool media_hal_is_codec_mute()
{
    return codec_mute;
}

esp_err_t media_hal_control_volume(media_hal_t* media_hal, uint8_t volume)
{
    if (mute_by_bt) {
        /* If codec is muted by BT, we need to save volume for later. */
        volume_prv = volume;
        return ESP_OK;
    }
    media_hal_notify_volume_change(media_hal, volume);
    if (!media_hal) {
        return ESP_FAIL;
    }
    volume_prv = volume;
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    if (volume == 0) {
        ret = media_hal->audio_codec_set_mute(true);
    } else {
        ret = media_hal->audio_codec_control_volume(volume);
    }
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

/**
 * Set codec to mute for if bluetooth is playing!
 */
esp_err_t media_hal_set_mute_bt(media_hal_t* media_hal, bool mute)
{
    esp_err_t ret = ESP_OK;
    mute_by_bt = mute;
    if (mute || (codec_mute && !mute)) {
        /**
         * This is request for bt unmute but codec mute is true.
         * Since there is no guarantee that we are already in mute state, do it.
         */
        xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
        ret = media_hal->audio_codec_set_mute(true);
        xSemaphoreGive(media_hal->media_hal_lock);
    } else {
        xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
        ret = media_hal->audio_codec_control_volume(volume_prv);
        xSemaphoreGive(media_hal->media_hal_lock);
    }
    return ret;
}

esp_err_t media_hal_set_mute(media_hal_t* media_hal, bool mute)
{
    if (mute_by_bt) {
        /* If codec is muted by BT, we need to save mute state for later. */
        codec_mute = mute;
        return ESP_OK;
    }
    if (mute) {
        media_hal_notify_volume_change(media_hal, 0);
    } else {
        media_hal_notify_volume_change(media_hal, volume_prv);
    }

    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ret = media_hal->audio_codec_set_mute(mute);
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_get_volume(media_hal_t* media_hal, uint8_t *volume)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    *volume = volume_prv;
    //ret = media_hal->audio_codec_get_volume(volume);
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_config_format(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ret = media_hal->audio_codec_config_format(mode, fmt);
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_set_clk(media_hal_t* media_hal, media_hal_codec_mode_t mode, uint32_t rate, media_hal_bit_length_t bits_per_sample)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ret = media_hal->audio_codec_set_i2s_clk(mode, bits_per_sample);
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_powerup(media_hal_t* media_hal)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ret = media_hal->audio_codec_powerup();
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}

esp_err_t media_hal_powerdown(media_hal_t* media_hal)
{
    if (!media_hal) {
        return ESP_FAIL;
    }
    esp_err_t ret;
    xSemaphoreTake(media_hal->media_hal_lock, portMAX_DELAY);
    ret = media_hal->audio_codec_powerdown();
    xSemaphoreGive(media_hal->media_hal_lock);
    return ret;
}
