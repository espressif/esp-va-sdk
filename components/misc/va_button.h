// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _VA_BUTTON_H_
#define _VA_BUTTON_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_adc_cal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*va_button_factory_reset_cb_t)(void *arg);
typedef void (*va_button_wifi_reset_cb_t)(void *arg);
typedef void (*va_button_setup_mode_cb_t)(void *arg);
/**
 * @brief  initialize button service
 */
esp_err_t va_button_init();
void va_button_register_factory_reset_cb(va_button_factory_reset_cb_t factory_reset_cb);
void va_button_register_wifi_reset_cb(va_button_wifi_reset_cb_t wifi_reset_cb);
void va_button_register_setup_mode_cb(va_button_setup_mode_cb_t setup_mode_cb);

/**
 *
 */
void va_button_notify_mute(bool va_btn_mute);

#ifdef __cplusplus
}
#endif

#endif /* _VA_BUTTON_H_ */
