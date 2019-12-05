/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _MEDIA_HAL_H_
#define _MEDIA_HAL_H_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEDIA_HAL_VOL_DEFAULT 60

/**
 * @brief Select media hal codec mode
 */
typedef enum {
    MEDIA_HAL_CODEC_MODE_ENCODE = 1,  //select adc
    MEDIA_HAL_CODEC_MODE_DECODE,      //select dac
    MEDIA_HAL_CODEC_MODE_BOTH,        //select adc and dac both
    MEDIA_HAL_CODEC_MODE_LINE_IN,
} media_hal_codec_mode_t;

/**
 * @brief Select adc channel for input mic signal
 */
typedef enum {
    MEDIA_HAL_ADC_INPUT_LINE1 = 0x00,  //mic input to adc channel 1
    MEDIA_HAL_ADC_INPUT_LINE2,         //mic input to adc channel 2
    //MEDIA_HAL_ADC_INPUT_ALL =
    MEDIA_HAL_ADC_INPUT_DIFFERENCE,
} media_hal_adc_input_t;

/**
 * @brief Select dac channel for output voice signal
 */
typedef enum {
    MEDIA_HAL_DAC_OUTPUT_LINE1 = 0x00,  //voice output to dac channel 1
    MEDIA_HAL_DAC_OUTPUT_LINE2,         //voive output to dac channel 2
    MEDIA_HAL_DAC_OUTPUT_ALL,
} media_hal_dac_output_t;

/**
 * @brief Select play speed (1x speed is for 16 bit sample)
 */
typedef enum {
    MEDIA_HAL_PLAY_SPEED_1X,  //play speed is normal(considering 16 bit per sample)
    MEDIA_HAL_PLAY_SPEED_2X,  //play speed is twice
    MEDIA_HAL_PLAY_SPEED_4X,  //set play speed to 4x
    MEDIA_HAL_PLAY_SPEED_6X,  //set play speed to 6x
} media_hal_play_speed_t;

/**
 * @brief Select operating mode i.e. master or slave for audio codec chip
 */
typedef enum {
    MEDIA_HAL_MODE_SLAVE = 0x00,   //set audio codec chip in slave  mode
    MEDIA_HAL_MODE_MASTER = 0x01,  //set audio codec chip in master mode
} media_hal_op_mode_t;

/**
 * @brief Select operating mode i.e. master or slave for audio codec chip
 */
typedef enum {
    MEDIA_HAL_STOP_STATE  = 0x00,  //stop  audio codec chip mode
    MEDIA_HAL_START_STATE = 0x01,  //start audio codec chip mode
} media_hal_sel_state_t;

/**
 * @brief Select voices samples per second
 */
typedef enum {
    MEDIA_HAL_8K_SAMPLES,    //set to  8k voice samples in one second
    MEDIA_HAL_16K_SAMPLES,   //set to 16k voice samples in one second
    MEDIA_HAL_24K_SAMPLES,   //set to 24k voice samples in one second
    MEDIA_HAL_44K_SAMPLES,   //set to 44k voice samples in one second
} media_hal_sel_samples_t;

/**
 * @brief Select number of bits per sample
 */
typedef enum {
    MEDIA_HAL_BIT_LENGTH_8BITS = 8,    //set  8 bits per sample
    MEDIA_HAL_BIT_LENGTH_16BITS = 16,  //set 16 bits per sample
    MEDIA_HAL_BIT_LENGTH_18BITS = 18,  //set 18 bits per sample
    MEDIA_HAL_BIT_LENGTH_20BITS = 20,  //set 20 bits per sample
    MEDIA_HAL_BIT_LENGTH_24BITS = 24,  //set 24 bits per sample
    MEDIA_HAL_BIT_LENGTH_32BITS = 32,  //set 32 bits per sample
} media_hal_bit_length_t;

/**
 * @brief Select i2s format for audio codec chip
 */
typedef enum {
    MEDIA_HAL_I2S_NORMAL = 0,  //set normal i2s format
    MEDIA_HAL_I2S_LEFT,        //set all left format
    MEDIA_HAL_I2S_RIGHT,       //set all right format
    MEDIA_HAL_I2S_DSP,         //set dsp/pcm format
} media_hal_format_t;

/**
 * @brief Configure media hal for initialization of audio codec chip
 */
typedef struct {
    media_hal_op_mode_t op_mode;        //set operarating mode of audio codec chip either master or slave
    media_hal_adc_input_t adc_input;    //set adc channel
    media_hal_dac_output_t dac_output;  //set dac channel
    media_hal_codec_mode_t codec_mode;  //select adc or dac or both mode to be configured
    media_hal_bit_length_t bit_length;  //set number of bits per sample
    media_hal_format_t  format;
    int port_num;
} media_hal_config_t;

#define VOLUME_CHANGE_CB_MAX 5
typedef  struct {
    esp_err_t (*audio_codec_initialize) (media_hal_config_t *media_hal_conf);
    esp_err_t (*audio_codec_deinitialize) (int port_num);
    esp_err_t (*audio_codec_set_state) (media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);
    esp_err_t (*audio_codec_set_i2s_clk) (media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length);
    esp_err_t (*audio_codec_config_format) (media_hal_codec_mode_t mode, media_hal_format_t fmt);
    esp_err_t (*audio_codec_control_volume) (uint8_t volume);
    esp_err_t (*audio_codec_get_volume) (uint8_t *volume);
    esp_err_t (*audio_codec_set_mute) (bool mute);
    esp_err_t (*audio_codec_powerup) ();
    esp_err_t (*audio_codec_powerdown) ();
    void (*volume_change_notify_cb[VOLUME_CHANGE_CB_MAX]) (int volume);
    xSemaphoreHandle media_hal_lock;
} media_hal_t;

/**
 * @brief Initialize media codec driver
 *
 * @praram media_hal_conf Configure structure media_hal_config_t
 *
 * @return  media_hal_t* - success, otherwise NULL
 */
media_hal_t* media_hal_init(media_hal_config_t *media_hal_conf);

/**
 * @brief Returns first initialized media_hal_t structure
 *
 * @praram none
 *
 * @return  media_hal_t* - success, otherwise NULL
 */
media_hal_t* media_hal_get_handle();

/**
 * @brief Uninitialize media codec driver
 *
 * @param media_hal reference function pointer for selected audio codec
 * @praram none
 *
 * @return  int, 0--success, others--fail
 */
esp_err_t media_hal_deinit(media_hal_t* media_hal);

/**
 * @brief Start/stop codec driver
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param mode select media hal codec mode either encode/decode/or both to start from media_hal_codec_mode_t
 * @param media_hal_state select start stop state for specific mode
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_set_state(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);

/**
 * @brief Register volume change callback
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param callback volume change callback function to register
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_register_volume_change_cb(media_hal_t *media_hal, void (*callback) (int volume));

/**
 * @brief Deregister volume change callback
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param callback volume change callback function to deregister
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_deregister_volume_change_cb(media_hal_t *media_hal, void (*callback) (int volume));

/**
 * @brief Set voice volume.
 *        @note if volume is 0, mute is enabled
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param volume value of volume in percent(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_control_volume(media_hal_t* media_hal, uint8_t volume);

/**
 * @brief get voice volume.
 *        @note if volume is 0, mute is enabled
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param volume value of volume in percent returned(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_get_volume(media_hal_t* media_hal, uint8_t *volume);

/**
 * @brief Set mute.
 *        @note if volume is 0, mute is enabled
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param mute bool true - mute
 *                  false - unmute
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_set_mute(media_hal_t* media_hal, bool mute);

/**
 * @brief Set medial hal format
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param mode select media hal codec mode either encode/decode/or both to start from media_hal_codec_mode_t
 * @param fmt select signal format either I2S or PCM/DSP
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t media_hal_config_format(media_hal_t* media_hal, media_hal_codec_mode_t mode, media_hal_format_t fmt);

/**
 * @brief Set clock & bit width used for I2S RX and TX.
 *
 * @param media_hal reference function pointer for selected audio codec
 * @param mode select media hal codec mode either encode/decode/or both to start from media_hal_codec_mode_t
 * @param media_hal_codec_mode I2S sample rate (ex: 8000, 44100...)
 * @param rate I2S bit width (16, 24, 32)
 *
 * @return
 *     - 0   Success
 *     - -1  Error
 */
esp_err_t media_hal_set_clk(media_hal_t* media_hal, media_hal_codec_mode_t mode, uint32_t rate, media_hal_bit_length_t bits_per_sample);

/**
 * @brief power up audio codec
 *
 * @param media_hal reference function pointer for selected audio codec
 *
 * @return
 *     - 0   Success
 *     - -1  Error
 */
esp_err_t media_hal_powerup(media_hal_t* media_hal);

/**
 * @brief power down audio codec
 *
 * @param media_hal reference function pointer for selected audio codec
 *
 * @return
 *     - 0   Success
 *     - -1  Error
 */
esp_err_t media_hal_powerdown(media_hal_t* media_hal);

#ifdef __cplusplus
}
#endif

#endif  //__MEDIA_HAL_H__

