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
#include <common_dsp.h>
#include <esp_dsp.h>

esp_err_t va_dsp_hal_configure(void *config)
{
    esp_dsp_config_t *esp_dsp_config = (esp_dsp_config_t *)config;
    esp_dsp_configure(esp_dsp_config);
    return ESP_OK;
}

esp_err_t va_dsp_hal_init(QueueHandle_t queue)
{
    esp_dsp_init();
    common_dsp_init(queue);
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
    common_dsp_mic_mute();
    return ESP_OK;
}

esp_err_t va_dsp_hal_mic_unmute()
{
    common_dsp_mic_unmute();
    return ESP_OK;
}

esp_err_t va_dsp_hal_tap_to_talk()
{
    return ESP_OK;
}

esp_err_t va_dsp_hal_start_capture()
{
    common_dsp_start_capture();
    return ESP_OK;
}

esp_err_t va_dsp_hal_stop_capture()
{
    common_dsp_stop_capture();
    return ESP_OK;
}

size_t va_dsp_hal_get_ww_len()
{
    return common_dsp_get_ww_len();
}

esp_err_t va_dsp_hal_get_preroll(void *data)
{
    return ESP_OK;
}

int va_dsp_hal_stream_audio(uint8_t *buffer, int size, int wait)
{
    return common_dsp_stream_audio(buffer, size, wait);;
}

esp_err_t va_dsp_hal_stream_pause()
{
    return esp_dsp_audio_stream_pause();
}

esp_err_t va_dsp_hal_stream_resume()
{
    return esp_dsp_audio_stream_resume();
}
