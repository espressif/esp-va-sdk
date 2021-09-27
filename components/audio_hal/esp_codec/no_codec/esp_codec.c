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

#define TAG "esp_codec_no_codec"

#include "esp_types.h"
#include "media_hal.h"


/**
 * @brief Initialize no audio codec
 *
 * @param no_codec_mode set no_codec in master or slave mode
 * @param no_codec_adc_input select adc input channel
 * @param no_codec_dac_output select dac output channel
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_init(media_hal_config_t *media_hal_conf)
{
    ESP_LOGI(TAG, "Initialising esp_codec");
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief De-initialize no_codec audio codec
 *
 * @param port_num i2c port number
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_deinit(int port_num)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Configure no_codec data format either I2S, PCM or left/right justified
 *
 * @param mode select mode for no_codec either ADC, DAC, both ADC and DAC or LINE
 * @param fmt  select audio data format
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Set I2S clock for no_codec. In slave mode the SCLK and LRCLK frequencies are auto-detected
 *
 * @param mode select mode for no_codec either ADC, DAC, both ADC and DAC or LINE
 * @param rate set frequency
 * @param media_hal_bit_length set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Start/stop selected mode of no_codec
 *
 * @param mode select mode for no_codec either ADc, DAC, both ADC and DAC or LINE
 * @param media_hal_state select start stop state for specific mode
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Set voice volume for audio output
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_control_volume(uint8_t volume)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief get voice volume
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent returned(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_get_volume(uint8_t *volume)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Set mute
 *
 * @param mute enable or disable
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_mute(bool bmute)
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}


/**
 * @brief Power up no_codec audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_powerup()
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}

/**
 * @brief Power down no_codec audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_powerdown()
{
    ESP_LOGD(TAG, "Function %s not implemented", __FUNCTION__);
    return ESP_OK;
}
