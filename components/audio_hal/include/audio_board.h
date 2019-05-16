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
#ifndef __AUDIO_BOARD_H__
#define __AUDIO_BOARD_H__

#include <esp_types.h>

#include <driver/i2c.h>
#include <driver/i2s.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief returns i2s gpio config struct
 *
 * @param port_num i2s port number
 * @param pf_i2s_pin i2s gpio init struct
 *
 */
esp_err_t audio_board_i2s_pin_config(int port_num, i2s_pin_config_t *pf_i2s_pin);

/*
 * @brief returns i2c config struct with gpio pins intialization
 *
 *@param port_num i2c port number
 *@param pf_i2c_pin i2c gpio init struct
 *
 */
esp_err_t audio_board_i2c_pin_config(int port_num, i2c_config_t *pf_i2c_pin);

/*
 * @brief returns i2s default parameters init
 *
 *@param i2s_cfg_dft i2s param config structure
 *
 */
esp_err_t audio_board_i2s_init_default(i2s_config_t *i2s_cfg_dft);

#ifdef __cplusplus
}
#endif

#endif
