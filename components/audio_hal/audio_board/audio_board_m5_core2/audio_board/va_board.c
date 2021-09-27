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
#include <display_driver.h>
#include <va_board.h>
#include <esp_dsp.h>
#include <va_dsp_hal.h>
#include <media_hal.h>
#include <media_hal_playback.h>

#include <core2forAWS.h>

#define VA_TAG "AUDIO_BOARD"

extern SemaphoreHandle_t spi_mutex;

static esp_err_t va_board_button_init()
{
    /*
    button_left   == 0
    button_middle == 1
    button_right  == 2
    */
    button_driver_config_t button_driver_config = {
        .button_val = {
            [BUTTON_EVENT_TAP_TO_TALK] = ((int64_t)1) << 0,     /* button_left */
            [BUTTON_EVENT_MIC_MUTE] = ((int64_t)1) << 1,        /* button_middle */
            [BUTTON_EVENT_VOLUME_UP] = -1,
            [BUTTON_EVENT_VOLUME_DOWN] = -1,
            [BUTTON_EVENT_FACTORY_RST] = ((int64_t)1) << 0 | ((int64_t)1) << 1, /* button_left + button_middle */
            [BUTTON_EVENT_CUSTOM_1] = -1,
            [BUTTON_EVENT_CUSTOM_2] = -1,
            [BUTTON_EVENT_IDLE] = -1,
        },
    };
    // button_driver_enable_debug(true);
    button_driver_init(&button_driver_config);
    return ESP_OK;
}

static esp_err_t va_board_led_init()
{
    led_pattern_init();
    led_driver_config_t led_driver_config = {
        .num_of_leds = 10,
        .leds_in_set = 5,
    };
    led_driver_init(&led_driver_config);
    return ESP_OK;
}

static esp_err_t va_board_display_init()
{
    return display_driver_init();
}

int va_board_init()
{
    int ret;
    i2s_config_t i2s_cfg = {};
    i2s_pin_config_t ab_i2s_pin = {0};

    ESP_LOGE(VA_TAG, "Installing M5Stack Core2 board i2s driver for Speaker");
    audio_board_i2s_init_default(&i2s_cfg);
    ret = i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(VA_TAG, "Error installing i2s driver for stream");
        return ret;
    }

    // Write I2S0 pin config
    audio_board_i2s_pin_config(I2S_NUM_0, &ab_i2s_pin);
    i2s_set_pin(I2S_NUM_0, &ab_i2s_pin);

    static media_hal_config_t media_hal_cfg = MEDIA_HAL_DEFAULT();
    static media_hal_playback_cfg_t media_hal_playback_cfg = DEFAULT_MEDIA_HAL_PLAYBACK_CONFIG();
    media_hal_playback_cfg.channels = 1;
    media_hal_init(&media_hal_cfg, &media_hal_playback_cfg);

    spi_mutex = xSemaphoreCreateMutex();        // Used internally in Core2ForAWS
    Core2ForAWS_Init();

    /* The buttons for this board are on the display itself. Make sure to initialize the buttons only after initializing the display. */
    va_board_display_init();
    va_board_button_init();
    va_board_led_init();

    esp_dsp_config_t esp_dsp_cfg = ESP_DSP_CONFIG_DEFAULT();
    esp_dsp_cfg.set_i2s_clk = false;
    esp_dsp_cfg.channels = 1;
    va_dsp_hal_configure((void *)&esp_dsp_cfg);

    return ret;
}
