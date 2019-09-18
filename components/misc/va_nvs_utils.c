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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <nvs_flash.h>

#include "va_nvs_utils.h"

#define NVS_TASK_STACK_SIZE 2048
#define VA_NVS_NAMESPACE "avs"

static const char *TAG = "[va_nvs_utils]";

/* TODO: As of now, this utility will create a task for every NVS get/set operation. This is required,
 * only if a task having stack in SPIRAM needs NVS functionality. In future, we can add a check
 * if task is in extram or intram and act accordingly.
 */

/* This structure can take partition and other datatype as parameters in future, if required. */
struct nvs_ops_params {
    const char *ns;
    const char *key;
    void *val_buf;
    size_t *buf_size;
    enum {
        GET,
        SET,
        ERASE,
    } op;
    enum {
        I8,
        U16,
        STR,
        BLOB,
        KEY,
        INVALID,
    } type;
    TaskHandle_t calling_task_handle;
};

static void nvs_task(void *arg)
{
    struct nvs_ops_params *params = (struct nvs_ops_params *)arg;
    nvs_handle handle;
    uint32_t ret = ESP_OK;
    esp_err_t err = ESP_OK;

    nvs_open_mode mode = params->op == GET ? NVS_READONLY : NVS_READWRITE;

    if (params->type != INVALID) {
        err = nvs_open(params->ns, mode, &handle);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Cannot find namespace %s in NVS", params->ns);
            ret = ESP_FAIL;
            goto done;
        }
    }

    if (params->op == SET) {
        if (params->type == BLOB) {
            err = nvs_set_blob(handle, params->key, params->val_buf, *(params->buf_size));
        } else if (params->type == STR) {
            err = nvs_set_str(handle, params->key, (char *)params->val_buf);
        } else if (params->type == I8) {
            err = nvs_set_i8(handle, params->key, (int8_t)params->val_buf);
        } else if (params->type == U16) {
            err = nvs_set_u16(handle, params->key, (uint16_t)params->val_buf);
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
        } else if (params->type == I8) {
            err = nvs_get_i8(handle, params->key, (int8_t *)params->val_buf);
        } else if (params->type == U16) {
            err = nvs_get_u16(handle, params->key, (uint16_t *)params->val_buf);
        }
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "No value set for: %s", params->key);
            ret = ESP_FAIL;
        }
    } else if (params->op == ERASE) {
        if (params->type == KEY) {
            err = nvs_erase_key(handle, params->key);
        } else if (params->type == INVALID) {
            err = nvs_flash_erase();
        }
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error erasing nvs type: %d, %s, %d", params->type, params->key, err);
            ret = ESP_FAIL;
        }
    }
    if (params->type != INVALID) {
        nvs_close(handle);
    }

done:
    /* Notify calling task */
    xTaskNotify(params->calling_task_handle, ret, eSetValueWithOverwrite);
}

esp_err_t va_nvs_set_blob(const char *key, uint8_t *val_buf, size_t buf_size)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, &buf_size, SET, BLOB, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_set_blob",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_get_blob(const char *key, uint8_t *val_buf, size_t *buf_size)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, buf_size, GET, BLOB, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_get_blob",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_set_str(const char *key, char *val_buf)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, NULL, SET, STR, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_set_str",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_get_str(const char *key, char *val_buf, size_t *buf_size)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, buf_size, GET, STR, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_get_str",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_set_i8(const char *key, int8_t val_buf)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, NULL, SET, I8, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_set_i8",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_get_i8(const char *key, int8_t *val_buf)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, NULL, GET, I8, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_get_i8",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_set_u16(const char *key, uint16_t val_buf)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, NULL, SET, U16, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_set_u16",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_get_u16(const char *key, uint16_t *val_buf)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, val_buf, NULL, GET, U16, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_get_u16",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to set %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_flash_erase()
{
    struct nvs_ops_params tp = {NULL, NULL, NULL, 0, ERASE, INVALID, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_flash_erase",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to erase flash in NVS");
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t va_nvs_erase_key(const char *key)
{
    struct nvs_ops_params tp = {VA_NVS_NAMESPACE, key, NULL, NULL, ERASE, KEY, xTaskGetCurrentTaskHandle()};
    esp_timer_handle_t nvs_task_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = nvs_task,
        .arg = &tp,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_nvs_erase_key",
    };
    if (esp_timer_create(&timer_arg, &nvs_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer to erase %s in NVS", key);
        return ESP_FAIL;
    }
    esp_timer_start_once(nvs_task_handle, 0);
    uint32_t result;
    /* Wait for operation to complete */
    xTaskNotifyWait(0, 0, &result, portMAX_DELAY);
    esp_timer_delete(nvs_task_handle);
    if ((int)result != ESP_OK) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}
