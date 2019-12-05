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

#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "zl38063.h"
#include <audio_board.h>
#include "tw_spi_access.h"
#define ZL_TAG "CODEC_ZL38063"

#define ZL38063_DISABLE_MUTE 0x00   //disable mute
#define ZL38063_ENABLE_MUTE  0x01   //enable  mute

#define ZL38063_DEFAULT_VOL 45

#define ZL38063_I2C_MASTER_SPEED 100000  //set master clk speed to 100k

#define ZL_ERR_CHK(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(ZL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

uint8_t volume_curr = 0;

/**
 * @brief Initialization function for i2c
 */
static esp_err_t audio_codec_i2c_init(int i2c_master_port)
{
    int res;
    i2c_config_t pf_i2c_pin = {0};

    res = audio_board_i2c_pin_config(i2c_master_port, &pf_i2c_pin);

    pf_i2c_pin.mode = I2C_MODE_MASTER;
    pf_i2c_pin.master.clk_speed = ZL38063_I2C_MASTER_SPEED;

    res |= i2c_param_config(i2c_master_port, &pf_i2c_pin);
    res |= i2c_driver_install(i2c_master_port, pf_i2c_pin.mode, 0, 0, 0);
    return res;
}

/**
 * @brief Write ZL38063 register
 *
 * @param slave_add : slave address
 * @param reg_add    : register address
 * @param data      : data to write
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static esp_err_t zl38063_write_reg(uint8_t slave_add, uint8_t reg_add, uint8_t data)
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
    ZL_ERR_CHK(res, "zl38063_write_reg error", -1);
    return res;
}

/**
 * @brief Read ZL38063 register
 *
 * @param reg_add    : register address
 *
 * @return
 *     - (-1)     Error
 *     - (0)      Success
 */
static esp_err_t zl38063_read_reg(uint8_t reg_add, uint8_t *p_data)
{
    uint8_t data;
    esp_err_t res;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res  = i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ZL38063_ADDR, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ZL38063_ADDR | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ZL_ERR_CHK(res, "zl38063_read_reg error", -1);
    *p_data = data;
    return res;
}

/**
 * @brief Configure ZL38063 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           -96 ~ 0              for example zl38063_set_adc_dac_volume(ZL38063_MODULE_ADC, 30, 6); means set ADC volume -30.5db
 * @param dot:              whether include 0.5. for example zl38063_set_adc_dac_volume(ZL38063_MODULE_ADC, 30, 4); means set ADC volume -30db
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static esp_err_t zl38063_set_adc_dac_volume(media_hal_codec_mode_t mode, float volume)
{
    esp_err_t res = 0;
    return res;
}

esp_err_t zl38063_set_state(media_hal_codec_mode_t mode, media_hal_sel_state_t media_hal_state)
{
    esp_err_t res = 0;
    return res;
}

esp_err_t zl38063_deinit(int port_num)
{
    esp_err_t ret = 0;
    return ret;
}


esp_err_t zl38063_init(media_hal_op_mode_t zl38063_mode, media_hal_adc_input_t zl38063_adc_input, media_hal_dac_output_t zl38063_dac_output, int port_num)
{
    esp_err_t res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_SEL_PA_EN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);

    audio_codec_i2c_init(port_num);
    return res;
}

esp_err_t zl38063_config_format(media_hal_codec_mode_t mode, media_hal_format_t fmt)
{
    esp_err_t res = 0;
    return res;
}

//Here we take 2's compliment for all negative values
esp_err_t zl38063_control_volume(uint8_t volume)
{
    esp_err_t res = 0;
    uint8_t shift;
    int8_t offset;
    unsigned short vol;
    volume_curr = volume;
    offset = (43 - volume/2);
    if(offset > 0) {
        shift = (0xFF - offset) + 0x01; //2's compliment standard formula
        vol = ((unsigned short )shift << 8) + shift; // Gain B[15:8] + Gain A[7:0]
        res = zl38063_init_vol(0x238, 1, &vol);
        res = zl38063_init_vol(0x23A, 1, &vol);
        return res;
    } else {
        shift = (-1)*offset;
        vol = ((unsigned short )shift << 8) + shift; // Gain B[15:8] + Gain A[7:0]
        res = zl38063_init_vol(0x238, 1, &vol);
        res = zl38063_init_vol(0x23A, 1, &vol);
        return res;
    }
}

esp_err_t zl38063_get_volume(uint8_t *volume)
{
    esp_err_t res = 0;
    *volume = volume_curr;
    return res;
}

esp_err_t zl38063_set_mute(bool bmute)
{
    esp_err_t res = 0;
    unsigned short vol = 0xA6A6;
    if (bmute) {
        res = zl38063_init_vol(0x238, 1, &vol);  //2's compliment for -90db
        res = zl38063_init_vol(0x23A, 1, &vol);  //2's compliment for -90db
    } else {
        res = zl38063_control_volume(volume_curr);
    }
    return res;
}

esp_err_t zl38063_powerup()
{
    //TODO
    return ESP_OK;
}

esp_err_t zl38063_powerdown()
{
    //TODO
    return ESP_OK;
}

esp_err_t zl38063_set_bits_per_sample(media_hal_codec_mode_t mode, media_hal_bit_length_t bits_per_sample)
{
    esp_err_t res = 0;
    return res;
}


esp_err_t zl38063_set_mic_gain(zl38063_mic_gain_t gain)
{
    esp_err_t ret = 0;
    return ret;
}

void zl38063_read_all_registers()
{

}

esp_err_t zl38063_write_register(uint8_t reg_add, uint8_t data)
{
    esp_err_t ret = 0;
    return ret;
    //return zl38063_write_reg(ZL38063_ADDR, reg_add, data);
}

esp_err_t zl38063_set_i2s_clk(media_hal_codec_mode_t media_hal_codec_mode, media_hal_bit_length_t media_hal_bit_length)
{

    esp_err_t ret = 0;
    return ret;
}
