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
*   |   The file includes functions and variables to configure ES8388.                      |
*   |                                                                                       |
*   ----------------------------------------------------------------------------------------
*/
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "es8388.h"
#include <audio_board.h>

#define ES_TAG "CODEC_ES8388"

#define ES8388_DISABLE_MUTE 0x00   //disable mute
#define ES8388_ENABLE_MUTE  0x01   //enable  mute

#define ES8388_DEFAULT_VOL 45

#define ES8388_I2C_MASTER_SPEED 100000  //set master clk speed to 100k

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(ES_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define LOG_8388(fmt, ...)   ESP_LOGW(ES_TAG, fmt, ##__VA_ARGS__)

uint8_t curr_vol = 0;

/**
 * @brief Initialization function for i2c
 */
static esp_err_t audio_codec_i2c_init(int i2c_master_port)
{
    int res;
    i2c_config_t pf_i2c_pin = {0};

    res = audio_board_i2c_pin_config(i2c_master_port, &pf_i2c_pin);

    pf_i2c_pin.mode = I2C_MODE_MASTER;
    pf_i2c_pin.master.clk_speed = ES8388_I2C_MASTER_SPEED;

    res |= i2c_param_config(i2c_master_port, &pf_i2c_pin);
    res |= i2c_driver_install(i2c_master_port, pf_i2c_pin.mode, 0, 0, 0);
    return res;
}

/**
 * @brief Write ES8388 register
 *
 * @param slave_add : slave address
 * @param reg_add    : register address
 * @param data      : data to write
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static esp_err_t es8388_write_reg(uint8_t slave_add, uint8_t reg_add, uint8_t data)
{
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slave_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ES_ASSERT(res, "es8388_write_reg error", -1);
    return res;
}

/**
 * @brief Read ES8388 register
 *
 * @param reg_add    : register address
 *
 * @return
 *     - (-1)     Error
 *     - (0)      Success
 */
static esp_err_t es8388_read_reg(uint8_t reg_add, uint8_t *p_data)
{
    uint8_t data;
    esp_err_t res;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res  = i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES8388_ADDR, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES8388_ADDR | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ES_ASSERT(res, "es8388_read_reg error", -1);
    *p_data = data;
    return res;
}

/**
 * @brief Configure ES8388 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           -96 ~ 0              for example es8388_set_adc_dac_volume(ES8388_MODULE_ADC, 30, 6); means set ADC volume -30.5db
 * @param dot:              whether include 0.5. for example es8388_set_adc_dac_volume(ES8388_MODULE_ADC, 30, 4); means set ADC volume -30db
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static esp_err_t es8388_set_adc_dac_volume(media_hal_codec_mode_t mode, float volume)
{
    esp_err_t res = 0;
    uint8_t vol;
    if ( volume < -96 || volume > 0 ) {
        LOG_8388("Warning: volume < -96! or > 0!\n");
        if (volume < -96)
            volume = -96;
        else
            volume = 0;
    }
    vol = (uint8_t) ((-1) * (volume * 2));

    if (mode == MEDIA_HAL_CODEC_MODE_ENCODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
        res  = es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL8, vol);
        res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL9, vol);  //ADC Right Volume=0db
    }
    if (mode == MEDIA_HAL_CODEC_MODE_DECODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
        res  = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL5, vol);
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL4, vol);
    }
    return res;
}

esp_err_t es8388_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    esp_err_t res = 0;
    uint8_t reg= 0;
    if(media_hal_state == MEDIA_HAL_START_STATE) {
        uint8_t prev_data = 0, data = 0;
        es8388_read_reg(ES8388_DACCONTROL21, &prev_data);
        if (mode == MEDIA_HAL_CODEC_MODE_LINE_IN) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL16, 0x09); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2 by pass enable
            res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL17, 0x50); // left DAC to left mixer enable  and  LIN signal to left mixer enable 0db  : bupass enable
            res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL20, 0x50); // right DAC to right mixer enable  and  LIN signal to right mixer enable 0db : bupass enable
            res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0xC0); //enable dac
        } else {
            res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80);   //enable dac
        }
        es8388_read_reg(ES8388_DACCONTROL21, &data);
        if (prev_data != data) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0xF0);   //start state machine
            res |= es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00);   //start state machine
        }
        if (mode == MEDIA_HAL_CODEC_MODE_ENCODE || mode == MEDIA_HAL_CODEC_MODE_BOTH || mode == MEDIA_HAL_CODEC_MODE_LINE_IN)
            res  = es8388_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0x00);   //power up adc and line in
        if (mode == MEDIA_HAL_CODEC_MODE_DECODE || mode == MEDIA_HAL_CODEC_MODE_BOTH || mode == MEDIA_HAL_CODEC_MODE_LINE_IN) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x3c);   //power up dac and line out
            /*
            Here we mute the dac when the State for decoding is in STOP mode
            Hence now we read the previous volume which has 33 steps in total
            also es8388_control_volume parameter is in %, so multiply the register value by 3
            */
            res |= es8388_read_reg(ES8388_DACCONTROL24, &reg);
            reg  = reg * 3;
            res |= es8388_control_volume(reg);
        }
        return res;
    }
    if(media_hal_state == MEDIA_HAL_STOP_STATE) {
        if (mode == MEDIA_HAL_CODEC_MODE_LINE_IN) {
        res  = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80); //enable dac
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
        return res;
        }
        if (mode == MEDIA_HAL_CODEC_MODE_DECODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x00);
            res |= es8388_control_volume(0);    //Mute
        }
        if (mode == MEDIA_HAL_CODEC_MODE_ENCODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0xFF);  //power down adc and line in
        }
        if (mode == MEDIA_HAL_CODEC_MODE_BOTH) {
            res  = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x9C);  //disable mclk
        }
        return res;
    }
    return res;
}

esp_err_t es8388_deinit(int port_num)
{
    esp_err_t ret;
    ret = es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0xFF);  //reset and stop es8388
    gpio_set_level(GPIO_PA_EN, 0);
    i2c_driver_delete(port_num);
    return ret;
}

esp_err_t es8388_powerup()
{
    esp_err_t ret;
    ret = es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00);  //Power up codec
    gpio_set_level(GPIO_PA_EN, 1);
    return ret;
}

esp_err_t es8388_powerdown()
{
    esp_err_t ret;
    ret = es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0xFF);  //Power down codec
    gpio_set_level(GPIO_PA_EN, 0);
    return ret;
}

esp_err_t es8388_init(media_hal_config_t *media_hal_conf)
{
    media_hal_op_mode_t es8388_mode = media_hal_conf->op_mode;
    media_hal_adc_input_t es8388_adc_input = media_hal_conf->adc_input;
    media_hal_dac_output_t es8388_dac_output = media_hal_conf->dac_output;
    int port_num = media_hal_conf->port_num;

    esp_err_t res;

    audio_codec_i2c_init(port_num);   //set i2c pin and i2c clock frequency for esp32

#ifndef ES8388_DISABLE_PA_PIN
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_SEL_PA_EN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);
#endif
    res = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, 0x04);  // 0x04 mute/0x00 unmute&ramp;DAC unmute and  disabled digital volume control soft ramp
    /* Chip Control and Power Management */
    res |= es8388_write_reg(ES8388_ADDR, ES8388_CONTROL2, 0x50);
    res |= es8388_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00);         //normal all and power up all
    res |= es8388_write_reg(ES8388_ADDR, ES8388_MASTERMODE, es8388_mode); //CODEC IN I2S SLAVE MODE

    /* dac */
    // res |= es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0xC0);       //disable DAC and disable Lout/Rout/1/2
    res |= es8388_write_reg(ES8388_ADDR, ES8388_CONTROL1, 0x12);       //Enfr=0,Play&Record Mode,(0x17-both of mic&paly)
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL1, 0x18);    //1a 0x18:16bit iis , 0x00:24
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL2, 0x02);    //DACFsMode,SINGLE SPEED; DACFsRatio,256
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL16, 0x00);   // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL17, 0x90);   // only left DAC to left mixer enable 0db
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL20, 0x90);   // only right DAC to right mixer enable 0db
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80);   //set internal ADC and DAC use the same LRCK clock, ADC LRCK as internal LRCK
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL23, 0x00);   //vroi=0
    res |= es8388_set_adc_dac_volume(MEDIA_HAL_CODEC_MODE_DECODE, 0);  // 0db

    /* adc */
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0xFF);    //power down adc
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL1, 0x88); //0x88 MIC PGA =24DB

    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL3, 0x02);
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL4, 0x0c); //0d 0x0c I2S-16BIT, LEFT ADC DATA = LIN1 , RIGHT ADC DATA =RIN1
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL5, 0x02);  //ADCFsMode,singel SPEED,RATIO=256
    //ALC for Microphone
    res |= es8388_set_adc_dac_volume(MEDIA_HAL_CODEC_MODE_ENCODE, 0);      // 0db
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0x09); //Power up ADC, Enable LIN&RIN, Power down MICBIAS, set int1lp to low power mode

    if(es8388_dac_output == MEDIA_HAL_DAC_OUTPUT_LINE2) {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x28);  //Enable Lout/Rout 2
    } else if(es8388_dac_output == MEDIA_HAL_DAC_OUTPUT_ALL) {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x3c);  //Enable Lout/Rout 1 and 2 both
    } else {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x14);  //Default: Enable Lout/Rout 1
    }
    if(es8388_adc_input == MEDIA_HAL_ADC_INPUT_LINE2) {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL2, 0x50);  // Enable LIN2/RIN2 as ADC input
    } else if(es8388_adc_input == MEDIA_HAL_ADC_INPUT_DIFFERENCE) {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL2, 0xf0);  // Enable LIN1/RIN1 as well as LIN2/RIN2 for ADC input
    } else {
        res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL2, 0x00);  //Default: Enable LIN1/RIN1 as ADC input
    }
    //     es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL8, 0xC0);
    //res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL9,0xC0);
    return res;
}

esp_err_t es8388_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    esp_err_t res = 0;
    uint8_t reg = 0;
    if (mode == MEDIA_HAL_CODEC_MODE_ENCODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
        res = es8388_read_reg(ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xfc;
        res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL4, reg | fmt);
    }
    if (mode == MEDIA_HAL_CODEC_MODE_DECODE || mode == MEDIA_HAL_CODEC_MODE_BOTH) {
        res = es8388_read_reg(ES8388_DACCONTROL1, &reg);
        reg = reg & 0xf9;
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL1, reg | (fmt << 1));
    }
    return res;
}

esp_err_t es8388_control_volume(uint8_t volume)
{
    esp_err_t res = 0;
    curr_vol = volume;
    uint8_t reg = 0;

    if (volume > 100) {
        volume = 100;
    }
    res = es8388_read_reg(ES8388_DACCONTROL3, &reg);
    reg = reg & 0xFB;
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, reg | (ES8388_DISABLE_MUTE << 2));
    volume /= 3;
    res  = es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL24, volume);
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL25, volume);
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL26, 0);
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL27, 0);
    return res;
}

esp_err_t es8388_get_volume(uint8_t *volume)
{
    esp_err_t res = 0;
    uint8_t reg = 0;
    res = es8388_read_reg(ES8388_DACCONTROL26, &reg);
    if (res == ESP_FAIL) {
        *volume = 0;
    } else {
        *volume = reg;
        *volume *= 3;
        if (*volume == 99) {
            *volume = 100;
        }
    }
    *volume = curr_vol;
    return res;
}

esp_err_t es8388_set_mute(bool bmute)
{
    esp_err_t res = 0;
    uint8_t reg = 0;

    if (bmute) {
        res = es8388_read_reg(ES8388_DACCONTROL3, &reg);
        reg = reg & 0xFB;
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, reg | (ES8388_ENABLE_MUTE << 2));
    } else {
        res = es8388_read_reg(ES8388_DACCONTROL3, &reg);
        reg = reg & 0xFB;
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, reg | (ES8388_DISABLE_MUTE << 2));
    }
    return res;
}

/**
 * @brief Configure ES8388 data sample bits
 *
 * @param mode:             set ADC or DAC or all
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int es8388_set_bits_per_sample(es_module_t mode, es_bits_length_t bits_length)
{
    int res = 0;
    uint8_t reg = 0;
    int bits = (int) bits_length;

    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res = es8388_read_reg(ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xe3;
        res |=  es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL4, reg | (bits << 2));
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res = es8388_read_reg(ES8388_DACCONTROL1, &reg);
        reg = reg & 0xc7;
        res |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL1, reg | (bits << 3));
    }
    return res;
}

#if 0
/**
 * @param gain: Config DAC Output
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int es8388_config_dac_output(int output)
{
    int res;
    uint8_t reg = 0;
    res = es8388_read_reg(ES8388_DACPOWER, &reg);
    reg = reg & 0xc3;
    res |= es8388_write_reg(ES8388_ADDR, ES8388_DACPOWER, reg | output);
    return res;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int es8388_config_adc_input(es8388_adc_input_t input)
{
    int res;
    uint8_t reg = 0;
    res = es8388_read_reg(ES8388_ADCCONTROL2, &reg);
    reg = reg & 0x0f;
    res |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL2, reg | input);
    return res;
}
#endif

esp_err_t es8388_set_mic_gain(es8388_mic_gain_t gain)
{
    esp_err_t ret;
    int gain_n;
    gain_n = (int)gain / 3;
    ret = es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL1, gain_n); //MIC PGA
    return ret;
}

void es8388_read_all_registers()
{
    for (int i = 0; i < 50; i++) {
        uint8_t reg = 0;
        es8388_read_reg(i, &reg);
        ets_printf("%x: %x\n", i, reg);
    }
}

esp_err_t es8388_write_register(uint8_t reg_add, uint8_t data)
{
    return es8388_write_reg(ES8388_ADDR, reg_add, data);
}

esp_err_t es8388_set_i2s_clk(media_hal_codec_mode_t media_hal_codec_mode, media_hal_bit_length_t media_hal_bit_length)
{
    int clk_div = 2, tmp = 0;
    esp_err_t ret = ESP_OK;

    switch (media_hal_bit_length) {
        case MEDIA_HAL_BIT_LENGTH_16BITS:
            tmp = BIT_LENGTH_16BITS;
            clk_div = 3;
        break;
        case MEDIA_HAL_BIT_LENGTH_18BITS:
            tmp = BIT_LENGTH_18BITS;
        break;
        case MEDIA_HAL_BIT_LENGTH_20BITS:
            tmp = BIT_LENGTH_20BITS;
        break;
        case MEDIA_HAL_BIT_LENGTH_24BITS:
            tmp = BIT_LENGTH_24BITS;
        break;
        default:
            tmp = BIT_LENGTH_32BITS;
            clk_div = 3;
    }

    ret |= es8388_set_bits_per_sample(ES_MODULE_ADC_DAC, tmp);
    ret |= es8388_write_reg(ES8388_ADDR, ES8388_ADCCONTROL5, clk_div);  //ADCFsMode,singel SPEED,RATIO=256
    ret |= es8388_write_reg(ES8388_ADDR, ES8388_DACCONTROL2, clk_div);  //ADCFsMode,singel SPEED,RATIO=256
    return ret;
}
