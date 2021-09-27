/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include <va_dsp_hal.h>

extern const uint8_t firmware_start[] asm("_binary_firmware_bin_start");
extern const uint8_t firmware_end[]   asm("_binary_firmware_bin_end");

esp_err_t va_dsp_hal_configure(void *config)
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_init(QueueHandle_t queue)
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_reset()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_enter_low_power()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_exit_low_power()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_mic_mute()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_mic_unmute()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_tap_to_talk()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_start_capture()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_stop_capture()
{
    return ESP_OK;
}

size_t va_dsp_hal_get_ww_len()
{
    return 0;
}

esp_err_t va_dsp_hal_get_preroll(void *data)
{
    return ESP_OK;
}

int va_dsp_hal_stream_audio(uint8_t *buffer, int size, int wait)
{
    return 0;
}
