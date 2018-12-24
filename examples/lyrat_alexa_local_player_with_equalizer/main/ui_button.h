// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.


#ifndef _UI_BUTTON_H_
#define _UI_BUTTON_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_BUTTON_TASK_BUFFER_SZ (8 * 1024)

#define GPIO_RECORD_BUTTON 36
#define GPIO_MODE_BUTTON   39
#define ESP_INTR_FLAG_DEFAULT        0

/**
 * @brief  initialize button service
 */
esp_err_t ui_button_init();

#ifdef __cplusplus
}
#endif

#endif /* _UI_BUTTON_H_ */
