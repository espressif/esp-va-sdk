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

#define VA_BUTTON_TASK_BUFFER_SZ   (8 * 1024)

enum {
    VA_BUTTON_TAP_TO_TALK = 0,
    VA_BUTTON_MIC_MUTE,
    VA_BUTTON_VOLUME_UP,
    VA_BUTTON_VOLUME_DOWN,
    VA_BUTTON_FACTORY_RST,
    VA_BUTTON_CUSTOM_1,
    VA_BUTTON_CUSTOM_2,
    VA_BUTTON_VAL_IDLE,
    VA_BUTTON_MAX,
};

typedef struct {
    bool is_adc;
    bool is_gpio;
    uint16_t va_button_adc_ch_num;
    union {
        int va_button_gpio_num[VA_BUTTON_MAX - 1];      // Since gpio is interrupt based, Idle is not required. Hence (MAX -1)
        int16_t va_button_adc_val[VA_BUTTON_MAX];
    };
    uint16_t tolerance;
} button_cfg_t;

typedef void (*va_button_wifi_reset_cb_t)(void *arg);
/**
 * @brief  initialize button service
 */
esp_err_t va_button_init(const button_cfg_t *button_cfg, int (*button_event_cb)(int));
void va_button_register_wifi_reset_cb(va_button_wifi_reset_cb_t wifi_reset_cb);

/**
 *
 */
void va_button_notify_mute(bool va_btn_mute);

#ifdef __cplusplus
}
#endif

#endif /* _VA_BUTTON_H_ */
