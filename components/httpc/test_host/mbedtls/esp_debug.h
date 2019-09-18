#pragma once

#include <stdio.h>
#include <errno.h>

#define esp_err_t int

#define ESP_ERR_INVALID_ARG -1
#define ESP_ERR_NO_MEM      -2
#define ESP_FAIL            -1
#define ESP_OK              0

#define ESP_LOGD(TAG, ...) //printf(__VA_ARGS__);
#define ESP_LOGE(TAG, ...) printf(__VA_ARGS__);
#define ESP_LOGI(TAG, ...) printf(__VA_ARGS__);
