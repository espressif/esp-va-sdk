#pragma once

#include "esp_types.h"
#include "driver/i2c.h"
#include "media_hal.h"


/**
 * @brief Initialize audio codec
 *
 * @param media_hal_conf configuration of the codec
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_init(media_hal_config_t *media_hal_conf);

/**
 * @brief De-initialize audio codec
 *
 * @param port_num i2c port number
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_deinit(int port_num);

/**
 * @brief Configure codec data format either I2S, PCM or left/right justified
 *
 * @param mode select mode for codec either ADC, DAC, both ADC and DAC or LINE
 * @param fmt  select audio data format
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt);

/**
 * @brief Set I2S clock for codec. In slave mode the SCLK and LRCLK frequencies are auto-detected
 *
 * @param mode select mode for codec either ADC, DAC, both ADC and DAC or LINE
 * @param media_hal_bit_length set bit length for each audio sample
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length);

/**
 * @brief Start/stop selected mode of codec
 *
 * @param mode select mode for codec either ADc, DAC, both ADC and DAC or LINE
 * @param media_hal_state select start stop state for specific mode
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);

/**
 * @brief Set voice volume for audio output
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent(%)
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_control_volume(uint8_t volume);

/**
 * @brief get voice volume
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent returned(%)
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_get_volume(uint8_t *volume);

/**
 * @brief Set mute
 *
 * @param mute enable or disable
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_set_mute(bool bmute);

/**
 * @brief Power up audio codec
 *
 * @param none
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_powerup();

/**
 * @brief Power down audio codec
 *
 * @param none
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t hollow_codec_powerdown();
