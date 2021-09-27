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
#include <esp_log.h>
#include <audio_board.h>
#include <led_pattern.h>
#include <led_driver.h>
#include <button_driver.h>
#include <va_board.h>
#include <esp_dsp.h>
#include <va_dsp_hal.h>
#include <media_hal.h>
#include <media_hal_playback.h>

#define VA_TAG "AUDIO_BOARD"

#define I2S_PORT_NUM I2S_NUM_0

static esp_err_t va_board_button_init()
{
    button_driver_config_t button_driver_config = {
        .button_val = {
            [BUTTON_EVENT_TAP_TO_TALK] = ((int64_t)1) << 36,
            [BUTTON_EVENT_MIC_MUTE] = ((int64_t)1) << 39,
            [BUTTON_EVENT_VOLUME_UP] = -1,
            [BUTTON_EVENT_VOLUME_DOWN] = -1,
            [BUTTON_EVENT_FACTORY_RST] = ((int64_t)1) << 36 | ((int64_t)1) << 39,
            [BUTTON_EVENT_CUSTOM_1] = -1,
            [BUTTON_EVENT_CUSTOM_2] = -1,
            [BUTTON_EVENT_IDLE] = -1,
        },
    };
    // button_driver_enable_debug(true);
    button_driver_init(&button_driver_config);
    return ESP_OK;
}

esp_err_t va_board_led_init()
{
    led_pattern_init();
    led_driver_config_t led_driver_config = {
        .type = LED_DRIVER_TYPE_SINGLE,
        .led_order = LED_ORDER_BLUE_RED_GREEN,
        .red_gpio = -1,
        .green_gpio = 22,
        .blue_gpio = -1,
    };
    led_driver_init(&led_driver_config);
    return ESP_OK;
}

int va_board_init()
{
    int ret;

    i2s_config_t i2s_cfg = {};
    audio_board_i2s_init_default(&i2s_cfg);
    ret = i2s_driver_install(I2S_PORT_NUM, &i2s_cfg, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(VA_TAG, "Error installing i2s driver for stream");
    } else {
        i2s_pin_config_t ab_i2s_pin = {0};
        audio_board_i2s_pin_config(I2S_PORT_NUM, &ab_i2s_pin);
        i2s_set_pin(I2S_PORT_NUM, &ab_i2s_pin);
    }
    ret = i2s_zero_dma_buffer(I2S_PORT_NUM);

    static media_hal_config_t media_hal_cfg = MEDIA_HAL_DEFAULT();
    static media_hal_playback_cfg_t media_hal_playback_cfg = DEFAULT_MEDIA_HAL_PLAYBACK_CONFIG();
    media_hal_init(&media_hal_cfg, &media_hal_playback_cfg);

    va_board_button_init();
    va_board_led_init();

    esp_dsp_config_t esp_dsp_cfg = ESP_DSP_CONFIG_DEFAULT();
    va_dsp_hal_configure((void *)&esp_dsp_cfg);

    return ret;
}
