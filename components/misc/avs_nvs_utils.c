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

#include <esp_log.h>
#include <nvs_flash.h>

#include "avs_nvs_utils.h"

#define NVS_TASK_STACK_SIZE 2048

static const char *TAG = "avs-nvs-util";

/* TODO: As of now, this utility will create a task for every NVS get/set operation. This is required,
 * only if a task having stack in SPIRAM needs NVS functionality. In future, we can add a check
 * if task is in extram or intram and act accordingly.
 */

/* This structure can take partition and other datatype as parameters in future, if required. */
struct nvs_ops_params {
    const char *ns;
    const char *key;
    uint8_t *val_buf;
    size_t *buf_size;
    enum {
        GET,
        SET,
        ERASE,
    } op;
    enum {
        STR,
        BLOB,
        INVALID,
    } type;
    TaskHandle_t calling_task_handle;
};

static void nvs_task(void *arg)
{
    struct nvs_ops_params *params = (struct nvs_ops_params *)arg;
    nvs_handle handle;
    uint32_t ret = ESP_OK;

    nvs_open_mode mode = params->op == SET ? NVS_READWRITE : NVS_READONLY;

    if (params->op == ERASE) {
        esp_err_t ret = nvs_flash_erase();
        if (ret == ESP_OK) {
            goto done;
        }
    }
    esp_err_t err = nvs_open(params->ns, mode, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening %s namespace!", params->ns);
        ret = ESP_FAIL;
        goto done;
    }

    if (params->op == SET) {
        if (params->type == BLOB) {
            err = nvs_set_blob(handle, params->key, params->val_buf, *(params->buf_size));
        } else if (params->type == STR) {
            err = nvs_set_str(handle, params->key, (char *)params->val_buf);
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting value: %s", params->key);
            ret = ESP_FAIL;
        }
    } else if (params->op == GET) {
        if (params->type == BLOB) {
            err = nvs_get_blob(handle, params->key, params->val_buf, params->buf_size);
        } else if (params->type == STR) {
            err = nvs_get_str(handle, params->key, (char *)params->val_buf, params->buf_size);
        }
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "No value set for: %s", params->key);
            ret = ESP_FAIL;
        }
    }
    nvs_close(handle);

done:
    /* Notify calling task */
    xTaskNotify(params->calling_task_handle, ret, eSetValueWithOverwrite);
    vTaskDelete(NULL);
}

esp_err_t avs_nvs_set_blob(const char *ns, const char *key, uint8_t *val_buf, size_t buf_size)
{
    struct nvs_ops_params tp = {ns, key, val_buf, &buf_size, SET, BLOB, xTaskGetCurrentTaskHandle()};
    /* This will create a thread, do NVS operation and complete the thread */
    TaskHandle_t nvs_task_handle;
    xTaskCreate(nvs_task, "nvs_task", NVS_TASK_STACK_SIZE, &tp, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &nvs_task_handle);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t avs_nvs_get_blob(const char *ns, const char *key, uint8_t *val_buf, size_t *buf_size)
{
    struct nvs_ops_params tp = {ns, key, val_buf, buf_size, GET, BLOB, xTaskGetCurrentTaskHandle()};
    /* This will create a thread, do NVS operation and complete the thread */
    TaskHandle_t nvs_task_handle;
    xTaskCreate(nvs_task, "nvs_task", NVS_TASK_STACK_SIZE, &tp, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &nvs_task_handle);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t avs_nvs_set_str(const char *ns, const char *key, char *val_buf)
{
    struct nvs_ops_params tp = {ns, key, (uint8_t *)val_buf, NULL, SET, STR, xTaskGetCurrentTaskHandle()};
    /* This will create a thread, do NVS operation and complete the thread */
    TaskHandle_t nvs_task_handle;
    xTaskCreate(nvs_task, "nvs_task", NVS_TASK_STACK_SIZE, &tp, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &nvs_task_handle);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t avs_nvs_get_str(const char *ns, const char *key, char *val_buf, size_t *buf_size)
{
    struct nvs_ops_params tp = {ns, key, (uint8_t *)val_buf, buf_size, GET, STR, xTaskGetCurrentTaskHandle()};
    /* This will create a thread, do NVS operation and complete the thread */
    TaskHandle_t nvs_task_handle;
    xTaskCreate(nvs_task, "nvs_task", NVS_TASK_STACK_SIZE, &tp, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &nvs_task_handle);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t avs_nvs_flash_erase()
{
    struct nvs_ops_params tp = {NULL, NULL, NULL, 0, ERASE, INVALID, xTaskGetCurrentTaskHandle()};
    /* This will create a thread, do NVS operation and complete the thread */
    TaskHandle_t nvs_task_handle;
    xTaskCreate(nvs_task, "nvs_task", NVS_TASK_STACK_SIZE, &tp, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &nvs_task_handle);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}
