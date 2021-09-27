#ifndef __ES8323_H__
#define __ES8323_H__

#include "esp_types.h"
#include "driver/i2c.h"
#include "media_hal.h"

/*Enable pin for es8323*/
#define GPIO_CODEC_EN           GPIO_NUM_2
#define GPIO_SEL_CODEC_EN       GPIO_SEL_2

/* ES8323 address */
#define ES8323_ADDR 0x20  // 0x22:CE=1;0x20:CE=0

/* ES8323 register */
#define ES8323_CONTROL1         0x00
#define ES8323_CONTROL2         0x01
/* Power control registers */
#define ES8323_CHIPPOWER        0x02
#define ES8323_ADCPOWER         0x03
#define ES8323_DACPOWER         0x04
#define ES8323_CHIPLOPOW1       0x05
#define ES8323_CHIPLOPOW2       0x06
#define ES8323_ANAVOLMANAG      0x07
#define ES8323_MASTERMODE       0x08
/* ADC */
#define ES8323_ADCCONTROL1      0x09
#define ES8323_ADCCONTROL2      0x0a
#define ES8323_ADCCONTROL3      0x0b
#define ES8323_ADCCONTROL4      0x0c
#define ES8323_ADCCONTROL5      0x0d
#define ES8323_ADCCONTROL6      0x0e
#define ES8323_ADCCONTROL7      0x0f
#define ES8323_ADCCONTROL8      0x10
#define ES8323_ADCCONTROL9      0x11
#define ES8323_ADCCONTROL10     0x12
#define ES8323_ADCCONTROL11     0x13
#define ES8323_ADCCONTROL12     0x14
#define ES8323_ADCCONTROL13     0x15
#define ES8323_ADCCONTROL14     0x16
/* DAC */
#define ES8323_DACCONTROL1      0x17
#define ES8323_DACCONTROL2      0x18
#define ES8323_DACCONTROL3      0x19
#define ES8323_DACCONTROL4      0x1a
#define ES8323_DACCONTROL5      0x1b
#define ES8323_DACCONTROL6      0x1c
#define ES8323_DACCONTROL7      0x1d
#define ES8323_DACCONTROL8      0x1e
#define ES8323_DACCONTROL9      0x1f
#define ES8323_DACCONTROL10     0x20
#define ES8323_DACCONTROL11     0x21
#define ES8323_DACCONTROL12     0x22
#define ES8323_DACCONTROL13     0x23
#define ES8323_DACCONTROL14     0x24
#define ES8323_DACCONTROL15     0x25
#define ES8323_DACCONTROL16     0x26
#define ES8323_DACCONTROL17     0x27
#define ES8323_DACCONTROL18     0x28
#define ES8323_DACCONTROL19     0x29
#define ES8323_DACCONTROL20     0x2a
#define ES8323_DACCONTROL21     0x2b
#define ES8323_DACCONTROL22     0x2c
#define ES8323_DACCONTROL23     0x2d
#define ES8323_DACCONTROL24     0x2e
#define ES8323_DACCONTROL25     0x2f
#define ES8323_DACCONTROL26     0x30
#define ES8323_DACCONTROL27     0x31
#define ES8323_DACCONTROL28     0x32
#define ES8323_DACCONTROL29     0x33
#define ES8323_DACCONTROL30     0x34


typedef enum {
    ES8323_BIT_LENGTH_MIN = -1,
    ES8323_BIT_LENGTH_16BITS = 0x03,
    ES8323_BIT_LENGTH_18BITS = 0x02,
    ES8323_BIT_LENGTH_20BITS = 0x01,
    ES8323_BIT_LENGTH_24BITS = 0x00,
    ES8323_BIT_LENGTH_32BITS = 0x04,
    ES8323_BIT_LENGTH_MAX,
} es8323_bit_length_t;

typedef enum {
    ES8323_SAMPLE_RATE_MIN = -1,
    ES8323_SAMPLE_RATE_16K,
    ES8323_SAMPLE_RATE_32K,
    ES8323_SAMPLE_RATE_44_1K,
    ES8323_SAMPLE_RATE_MAX,
} es8323_sample_rate_t;

/**
 * @brief Select adc channel for input mic signal
 */
typedef enum {
    ES8323_ADC_INPUT_LINE1 = 0x00,       //mic input to adc channel 1
    ES8323_ADC_INPUT_LINE2 = 0x50,       //mic input to adc channel 2
    ES8323_ADC_INPUT_DIFFERENCE = 0xf0,
} es8323_adc_input_t;

/**
 * @brief Select dac channel for output voice signal
 */
typedef enum {
    ES8323_DAC_OUTPUT_LINE1 = 0x14,  //voice output from dac channel 1
    ES8323_DAC_OUTPUT_LINE2 = 0x28,  //voice output from dac channel 2
    ES8323_DAC_OUTPUT_ALL = 0x3c,
    ES8323_DAC_OUTPUT_MAX,
} es8323_dac_output_t;

/**
 * @brief Select microphone gain for es8323
 */
typedef enum {
    ES8323_MIC_GAIN_0DB = 0,    //microphone gain set to 00DB
    ES8323_MIC_GAIN_3DB = 3,    //microphone gain set to 03DB
    ES8323_MIC_GAIN_6DB = 6,    //microphone gain set to 06DB
    ES8323_MIC_GAIN_9DB = 9,    //microphone gain set to 09DB
    ES8323_MIC_GAIN_12DB = 12,  //microphone gain set to 12DB
    ES8323_MIC_GAIN_15DB = 15,  //microphone gain set to 15DB
    ES8323_MIC_GAIN_18DB = 18,  //microphone gain set to 18DB
    ES8323_MIC_GAIN_21DB = 21,  //microphone gain set to 21DB
    ES8323_MIC_GAIN_24DB = 24,  //microphone gain set to 24DB
    ES8323_MIC_GAIN_MAX,
} es8323_mic_gain_t;

/**
 * @brief Select ES8323 working module
 */
typedef enum {
    ES8323_MODULE_ADC = 0x01,  //select adc mode
    ES8323_MODULE_DAC,         //select dac mode
    ES8323_MODULE_ADC_DAC,     //select both, adc and dac mode
    ES8323_MODULE_LINE,        //select line mode
} es8323_module_t;

/**
 * @brief Select mode for ES8323
 */
typedef enum {
    ES8323_MODE_SLAVE = 0x00,   //set es8323 in slave mode
    ES8323_MODE_MASTER = 0x01,  //set es8323 in master mode
} es8323_mode_t;

/**
 * @brief Select i2s format for ES8323
 */
typedef enum {
    ES8323_I2S_NORMAL = 0,  //i2s format
    ES8323_I2S_LEFT,        //left justified
    ES8323_I2S_RIGHT,       //right justified
    ES8323_I2S_DSP,         //pcm/dsp format
    ES8323_I2S_MAX
} es8323_i2s_format_t;

typedef struct {
    es8323_mode_t esMode;
    es8323_dac_output_t dacOutput;
    es8323_adc_input_t adcInput;
} es8323_config_t;

typedef enum {
    ES8323_SEL_DAC_CH_0,
    ES8323_SEL_DAC_CH_1,
} es8323_sel_dac_ch_t;

/**
 * @brief Initialize es8323 audio codec
 *
 * @param es8323_mode set es8323 in master or slave mode
 * @param es8323_adc_input select adc input channel
 * @param es8323_dac_output select dac output channel
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_init(media_hal_config_t *media_hal_conf);

/**
 * @brief De-initialize es8323 audio codec
 *
 * @param port_num i2c port number
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_deinit(int port_num);

/**
 * @brief Configure es8323 data format either I2S, PCM or left/right justified
 *
 * @param mode select mode for es8323 either ADC, DAC, both ADC and DAC or LINE
 * @param fmt  select audio data format
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt);

/**
 * @brief Set I2S clock for es8323. In slave mode the SCLK and LRCLK frequencies are auto-detected
 *
 * @param mode select mode for es8323 either ADC, DAC, both ADC and DAC or LINE
 * @param rate set frequency
 * @param media_hal_bit_length set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_set_i2s_clk(media_hal_codec_mode_t mode, media_hal_bit_length_t media_hal_bit_length);

/**
 * @brief Set bit per sample of audio data
 *
 * @param mode select mode for es8323 either ADc, DAC, both ADC and DAC or LINE
 * @param bits_per_sample set bit length for each audio sample
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_set_bits_per_sample(media_hal_codec_mode_t mode, media_hal_bit_length_t bits_per_sample);

/**
 * @brief Start/stop selected mode of es8323
 *
 * @param mode select mode for es8323 either ADc, DAC, both ADC and DAC or LINE
 * @param media_hal_state select start stop state for specific mode
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state);

/**
 * @brief Set voice volume for audio output
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_control_volume(uint8_t volume);

/**
 * @brief get voice volume
 *        @note if volume is 0, mute is enabled
 *
 * @param volume value of volume in percent returned(%)
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_get_volume(uint8_t *volume);

/**
 * @brief Set mute
 *
 * @param mute enable or disable
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_set_mute(bool bmute);

/**
 * @brief Set microphone gain
 *
 * @param gain select gain value
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_set_mic_gain(es8323_mic_gain_t gain);


//int es8323_config_adc_input(es8323_adc_input_t input);
/**
 * @exemple es8323_config_tes8323_dac_output_t(ES8323_DAC_OUTPUT_LOUT1 | ES8323_DAC_OUTPUT_LOUT2 | ES8323_DAC_OUTPUT_ROUT1 | ES8323_DAC_OUTPUT_ROUT2);
 */
//int es8323_config_dac_output(es8323_dac_output_t output);
esp_err_t es8323_write_register(uint8_t regAdd, uint8_t data);

/**
 * @brief Power up es8388 audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_powerup();

/**
 * @brief Power down es8388 audio codec
 *
 * @param none
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t es8323_powerdown();

void es8323_read_all_reg();


#endif //__ES8323_INTERFACE_H__