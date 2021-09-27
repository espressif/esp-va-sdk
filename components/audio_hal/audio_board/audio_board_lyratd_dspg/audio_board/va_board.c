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
#include <dspg_init.h>
#include <va_dsp_hal.h>
#include <media_hal.h>
#include <media_hal_playback.h>

#define VA_TAG "AUDIO_BOARD"

#define I2S_PORT_NUM I2S_NUM_0

#define DBMD5_CONFIG_DEFAULT()              \
{                                           \
    .pin_conf.apr_rst_pin = 21,             \
    .pin_conf.vt_rst_pin = 4,               \
    .pin_conf.apr_wakeup_pin = 2,           \
    .pin_conf.vt_intr_pin = 36,             \
    .pin_conf.pa_pin = 22,                  \
    .pin_conf.level_shift = -1,             \
    .pin_conf.spi_mosi_pin = 19,            \
    .pin_conf.spi_miso_pin = 27,            \
    .pin_conf.spi_clk_pin = 13,             \
    .pin_conf.apr_cs_pin = 32,              \
    .pin_conf.vt_cs_pin = 33,               \
    .fw_conf.total_mics_num = TOTAL_3_MICS, \
    .fw_conf.enable_low_power = false,      \
};

static esp_err_t va_board_button_init()
{
    button_driver_config_t button_driver_config = {
        .ch_num = ADC1_CHANNEL_3,
        .tolerance = 80,
        .button_val = {
            [BUTTON_EVENT_TAP_TO_TALK] = 600,
            [BUTTON_EVENT_MIC_MUTE] = 1230,
            [BUTTON_EVENT_VOLUME_UP] = 2480,
            [BUTTON_EVENT_VOLUME_DOWN] = 1830,
            [BUTTON_EVENT_FACTORY_RST] = 1530,
            [BUTTON_EVENT_CUSTOM_1] = -1,
            [BUTTON_EVENT_CUSTOM_2] = -1,
            [BUTTON_EVENT_IDLE] = 2700,
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
        .num_of_leds = 12,
        .start_gpio_pin = 0,
        .led_order = LED_ORDER_RED_GREEN_BLUE,
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
        i2s_pin_config_t pf_i2s_pin = {0};
        audio_board_i2s_pin_config(I2S_PORT_NUM, &pf_i2s_pin);
        i2s_set_pin(I2S_PORT_NUM, &pf_i2s_pin);
    }
    ret = i2s_zero_dma_buffer(I2S_PORT_NUM);

    static dbmd5_config_t dbmd5_config = DBMD5_CONFIG_DEFAULT();
#ifdef CONFIG_PM_ENABLE
    dbmd5_config.fw_conf.enable_low_power = true;
    dbmd5_config.fw_conf.grace_period = 5;
#endif
    va_dsp_hal_configure((void *)&dbmd5_config);

    static media_hal_config_t media_hal_cfg = MEDIA_HAL_DEFAULT();
    static media_hal_playback_cfg_t media_hal_playback_cfg = DEFAULT_MEDIA_HAL_PLAYBACK_CONFIG();
    media_hal_init(&media_hal_cfg, &media_hal_playback_cfg);

    va_board_button_init();
    va_board_led_init();

    return ret;
}
