/*
*
* Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

/*  ---------------------------------------------------------------------------------------
*   |                                                                                       |
*   |   The file includes functions and variables which do nothing!!!                       |
*   |                                                                                       |
*   ----------------------------------------------------------------------------------------
*/
//#include <string.h>
#include "esp_log.h"
#include <esp_codec.h>

#define TAG "esp_codec_hollow_codec"

#include "esp_types.h"
#include "media_hal.h"

esp_err_t hollow_codec_init(media_hal_config_t *media_hal_conf)
{
    ESP_LOGI(TAG, "Initialising");
    return ESP_OK;
}

esp_err_t hollow_codec_deinit(int port_num)
{
    ESP_LOGI(TAG, "Deinitialising");
    return ESP_OK;
}

esp_err_t hollow_codec_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    ESP_LOGI(TAG, "Configuring format");
    return ESP_OK;
}

esp_err_t hollow_codec_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length)
{
    ESP_LOGI(TAG, "Setting i2s clock");
    return ESP_OK;
}

esp_err_t hollow_codec_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    ESP_LOGI(TAG, "Setting state");
    return ESP_OK;
}

esp_err_t hollow_codec_control_volume(uint8_t volume)
{
    ESP_LOGI(TAG, "Changing volume to %d", volume);
    return ESP_OK;
}

esp_err_t hollow_codec_get_volume(uint8_t *volume)
{
    ESP_LOGI(TAG, "Getting volume");
    return ESP_OK;
}

esp_err_t hollow_codec_set_mute(bool mute)
{
    ESP_LOGI(TAG, "Setting mute to: %d", mute);
    return ESP_OK;
}

esp_err_t hollow_codec_powerup()
{
    ESP_LOGI(TAG, "Powering up");
    return ESP_OK;
}

esp_err_t hollow_codec_powerdown()
{
    ESP_LOGI(TAG, "Powering down");
    return ESP_OK;
}
