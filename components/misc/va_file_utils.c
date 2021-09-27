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
 */

#include <fcntl.h>
#include <esp_vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>

#include <va_file_utils.h>

static const char *TAG = "[va_file_utils]";

struct va_file_params {
    int fd;
    void *data;
    int len;
    char *file_name;
    int mode;
    enum {
        OPEN,
        CLOSE,
        READ,
    } op;
    TaskHandle_t calling_task_handle;
};

static void va_file_timer_cb(void *arg, uint32_t arg_len)
{
    struct va_file_params *params = (struct va_file_params *)arg;
    uint32_t ret = 0;
    if (params->op == OPEN) {
        ret = open(params->file_name, params->mode);
    } else if (params->op == CLOSE) {
        close(params->fd);
    } else if (params->op == READ) {
        ret = read(params->fd, params->data, params->len);
    } else {
        ESP_LOGE(TAG, "Invalid operation");
    }
    xTaskNotify(params->calling_task_handle, ret, eSetValueWithOverwrite);
}

static uint32_t va_file_process_common(struct va_file_params *params)
{
    uint32_t ret = 0;
    xTimerPendFunctionCall(va_file_timer_cb, params, 0, portMAX_DELAY);
    xTaskNotifyWait(0, 0, &ret, portMAX_DELAY);
    return ret;
}

uint32_t va_file_read(int fd, void *data, int len)
{
    struct va_file_params params = {
        .op = READ,
        .fd = fd,
        .data = data,
        .len = len,
        .calling_task_handle = xTaskGetCurrentTaskHandle(),
    };
    return va_file_process_common(&params);
}

uint32_t va_file_open(char *file_name, int mode)
{
    struct va_file_params params = {
        .op = OPEN,
        .file_name = file_name,
        .mode = mode,
        .calling_task_handle = xTaskGetCurrentTaskHandle(),
    };
    return va_file_process_common(&params);
}

uint32_t va_file_close(int fd)
{
    struct va_file_params params = {
        .op = CLOSE,
        .fd = fd,
        .calling_task_handle = xTaskGetCurrentTaskHandle(),
    };
    return va_file_process_common(&params);
}
