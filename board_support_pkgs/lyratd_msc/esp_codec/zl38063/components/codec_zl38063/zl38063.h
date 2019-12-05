/*
*
* Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#ifndef __ZL38063_H__
#define __ZL38063_H__

#include "esp_types.h"
#include "driver/i2c.h"
#include "media_hal.h"

/*Power speaker enable pin for zl38063*/
#define GPIO_PA_EN           GPIO_NUM_22
#define GPIO_SEL_PA_EN       GPIO_SEL_22

/* Zl38063 address */
#define ZL38063_ADDR 0x20  // 0x22:CE=1;0x20:CE=0


typedef enum {
    ZL38063_BIT_LENGTH_MIN = -1,
    ZL38063_BIT_LENGTH_16BITS = 0x03,
    ZL38063_BIT_LENGTH_18BITS = 0x02,
    ZL38063_BIT_LENGTH_20BITS = 0x01,
    ZL38063_BIT_LENGTH_24BITS = 0x00,
    ZL38063_BIT_LENGTH_32BITS = 0x04,
    ZL38063_BIT_LENGTH_MAX,
} zl38063_bit_length_t;

typedef enum {
    ZL38063_SAMPLE_RATE_MIN = -1,
    ZL38063_SAMPLE_RATE_16K,
    ZL38063_SAMPLE_RATE_32K,
    ZL38063_SAMPLE_RATE_44_1K,
    ZL38063_SAMPLE_RATE_MAX,
} zl38063_sample_rate_t;

/**
 * @brief Select adc channel for input mic signal
 */
typedef enum {
    ZL38063_ADC_INPUT_LINE1 = 0x00,       //mic input to adc channel 1
    ZL38063_ADC_INPUT_LINE2 = 0x50,       //mic input to adc channel 2
    ZL38063_ADC_INPUT_DIFFERENCE = 0xf0,
} zl38063_adc_input_t;

/**
 * @brief Select dac channel for output voice signal
 */
typedef enum {
    ZL38063_DAC_OUTPUT_LINE1 = 0x14,  //voice output from dac channel 1
    ZL38063_DAC_OUTPUT_LINE2 = 0x28,  //voice output from dac channel 2
    ZL38063_DAC_OUTPUT_ALL = 0x3c,
    ZL38063_DAC_OUTPUT_MAX,
} zl38063_dac_output_t;

/**
 * @brief Select microphone gain for zl38063
 */
typedef enum {
    ZL38063_MIC_GAIN_0DB = 0,    //microphone gain set to 00DB
    ZL38063_MIC_GAIN_3DB = 3,    //microphone gain set to 03DB
    ZL38063_MIC_GAIN_6DB = 6,    //microphone gain set to 06DB
    ZL38063_MIC_GAIN_9DB = 9,    //microphone gain set to 09DB
    ZL38063_MIC_GAIN_12DB = 12,  //microphone gain set to 12DB
    ZL38063_MIC_GAIN_15DB = 15,  //microphone gain set to 15DB
    ZL38063_MIC_GAIN_18DB = 18,  //microphone gain set to 18DB
    ZL38063_MIC_GAIN_21DB = 21,  //microphone gain set to 21DB
    ZL38063_MIC_GAIN_24DB = 24,  //microphone gain set to 24DB
    ZL38063_MIC_GAIN_MAX,
} zl38063_mic_gain_t;

/**
 * @brief Select ZL38063 working module
 */
typedef enum {
    ZL38063_MODULE_ADC = 0x01,  //select adc mode
    ZL38063_MODULE_DAC,         //select dac mode
    ZL38063_MODULE_ADC_DAC,     //select both, adc and dac mode
    ZL38063_MODULE_LINE,        //select line mode
} zl38063_module_t;

/**
 * @brief Select mode for ZL38063
 */
typedef enum {
    ZL38063_MODE_SLAVE = 0x00,   //set zl38063 in slave mode
    ZL38063_MODE_MASTER = 0x01,  //set zl38063 in master mode
} zl38063_mode_t;

/**
 * @brief Select i2s format for ZL38063
 */
typedef enum {
    ZL38063_I2S_NORMAL = 0,  //i2s format
    ZL38063_I2S_LEFT,        //left justified
    ZL38063_I2S_RIGHT,       //right justified
    ZL38063_I2S_DSP,         //pcm/dsp format
    ZL38063_I2S_MAX
} zl38063_i2s_format_t;

typedef struct {
    zl38063_mode_t esMode;
    zl38063_dac_output_t dacOutput;
    zl38063_adc_input_t adcInput;
} zl38063_config_t;

typedef enum {
    ZL38063_SEL_DAC_CH_0,
    ZL38063_SEL_DAC_CH_1,
} zl38063_sel_dac_ch_t;

/**
 * @brief Initialize zl38063 audio codec
 *
 * @param zl38063_mode set zl38063 in master or slave mode
 * @param zl38063_adc_input select adc input channel
 * @param zl38063_dac_output select dac output channel
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_init(media_hal_op_mode_t zl38063_mode, media_hal_adc_input_t zl38063_adc_input, media_hal_dac_output_t zl38063_dac_output, int port_num);

/**
 * @brief De-initialize zl38063 audio codec
 *
 * @param port_num i2c port number
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_deinit(int port_num);

/**
 * @brief Configure zl38063 data format either I2S, PCM or left/right justified
 *
 * @param mode select mode for zl38063 either ADC, DAC, both ADC and DAC or LINE
 * @param fmt  select audio data format
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt);

/**
 * @brief Set I2S clock for zl38063. In slave mode the SCLK and LRCLK frequencies are auto-detected
 *
 * @param mode select mode for zl38063 either ADC, DAC, both ADC and DAC or LINE
 * @param rate set frequency
 * @param media_hal_bit_length set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length);

/**
 * @brief Set bit per sample of audio data
 *
 * @param mode select mode for zl38063 either ADc, DAC, both ADC and DAC or LINE
 * @param bits_per_sample set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_set_bits_per_sample(media_hal_codec_mode_t mode, media_hal_bit_length_t bits_per_sample);

/**
 * @brief Start/stop selected mode of zl38063
 *
 * @param mode select mode for zl38063 either ADc, DAC, both ADC and DAC or LINE
 * @param media_hal_state select start stop state for specific mode
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);

/**
 * @brief Set voice volume for audio output
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_control_volume(uint8_t volume);

/**
 * @brief get voice volume
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent returned(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_get_volume(uint8_t *volume);

/**
 * @brief Set speaker mute
 *
 * @param mute enable or disable
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_set_mute(bool bmute);

/**
 * @brief Set microphone gain
 *
 * @param gain select gain value
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t zl38063_set_mic_gain(zl38063_mic_gain_t gain);


//int zl38063_config_adc_input(zl38063_adc_input_t input);
/**
 * @exemple zl38063_config_tzl38063_dac_output_t(ZL38063_DAC_OUTPUT_LOUT1 | ZL38063_DAC_OUTPUT_LOUT2 | ZL38063_DAC_OUTPUT_ROUT1 | ZL38063_DAC_OUTPUT_ROUT2);
 */
//int zl38063_config_dac_output(zl38063_dac_output_t output);
esp_err_t zl38063_write_register(uint8_t regAdd, uint8_t data);
void zl38063_read_all_reg();

esp_err_t zl38063_powerup();
esp_err_t zl38063_powerdown();

#endif //__ZL38063_INTERFACE_H__
