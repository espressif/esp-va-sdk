#pragma once

#include "esp_types.h"
#include "driver/i2c.h"
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
esp_err_t no_codec_init(media_hal_config_t *media_hal_conf);

/**
 * @brief De-initialize no_codec audio codec
 *
 * @param port_num i2c port number
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_deinit(int port_num);

/**
 * @brief Configure no_codec data format either I2S, PCM or left/right justified
 *
 * @param mode select mode for no_codec either ADC, DAC, both ADC and DAC or LINE
 * @param fmt  select audio data format
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt);

/**
 * @brief Set I2S clock for no_codec. In slave mode the SCLK and LRCLK frequencies are auto-detected
 *
 * @param mode select mode for no_codec either ADC, DAC, both ADC and DAC or LINE
 * @param rate set frequency
 * @param media_hal_bit_length set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length);


/**
 * @brief Start/stop selected mode of no_codec
 *
 * @param mode select mode for no_codec either ADc, DAC, both ADC and DAC or LINE
 * @param media_hal_state select start stop state for specific mode
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);

/**
 * @brief Set voice volume for audio output
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_control_volume(uint8_t volume);

/**
 * @brief get voice volume
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent returned(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_get_volume(uint8_t *volume);

/**
 * @brief Set mute
 *
 * @param mute enable or disable
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_set_mute(bool bmute);

/**
 * @brief Power up no_codec audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_powerup();

/**
 * @brief Power down no_codec audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t no_codec_powerdown();
