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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <nvs.h>
#include <esp_audio_nvs.h>

struct esp_audio_nvs_params {
    const char *namespace;
    const char *key;
    void *value;
    enum {
        GET,
        SET,
    } op;
    enum {
        I8,
        U16,
    } type;
    TaskHandle_t calling_task_handle;
};

static void esp_audio_nvs_timer_cb(void *arg, uint32_t arg_len)
{
    struct esp_audio_nvs_params *params = (struct esp_audio_nvs_params *)arg;
    esp_err_t err = 0;
    uint32_t ret = 0;
    nvs_handle_t handle;

    nvs_open_mode mode = params->op == GET ? NVS_READONLY : NVS_READWRITE;
    err = nvs_open(params->namespace, mode, &handle);
    if (err != ESP_OK) {
        goto done;
    }

    if (params->op == SET) {
        if (params->type == I8) {
            err = nvs_set_i8(handle, params->key, (int8_t)params->value);
        }
        nvs_commit(handle);
    } else if (params->op == GET) {
        if (params->type == I8) {
            err = nvs_get_i8(handle, params->key, (int8_t *)params->value);
        } else if (params->type == U16) {
            err = nvs_get_u16(handle, params->key, (uint16_t *)params->value);
        }
    }

    nvs_close(handle);

done:
    ret = err == ESP_OK ? 0 : 1;
    xTaskNotify(params->calling_task_handle, ret, eSetValueWithOverwrite);
}

static esp_err_t esp_audio_nvs_process_common(struct esp_audio_nvs_params *params)
{
    uint32_t ret = 0;
    params->calling_task_handle = xTaskGetCurrentTaskHandle();
    xTimerPendFunctionCall(esp_audio_nvs_timer_cb, params, 0, portMAX_DELAY);
    xTaskNotifyWait(0, 0, &ret, portMAX_DELAY);
    esp_err_t err = ret == 0 ? ESP_OK : ESP_FAIL;
    return err;
}

esp_err_t esp_audio_nvs_set_i8(char *namespace, char *key, int8_t value)
{
    struct esp_audio_nvs_params params = {
        .namespace = namespace,
        .key = key,
        .value = (void *)value,
        .op = SET,
        .type = I8,
    };
    return esp_audio_nvs_process_common(&params);
}

esp_err_t esp_audio_nvs_get_i8(char *namespace, char *key, int8_t *value)
{
    struct esp_audio_nvs_params params = {
        .namespace = namespace,
        .key = key,
        .value = (void *)value,
        .op = GET,
        .type = I8,
    };
    return esp_audio_nvs_process_common(&params);

}

esp_err_t esp_audio_nvs_get_u16(char *namespace, char *key, uint16_t *value)
{
    struct esp_audio_nvs_params params = {
        .namespace = namespace,
        .key = key,
        .value = (void *)value,
        .op = GET,
        .type = U16,
    };
    return esp_audio_nvs_process_common(&params);
}
