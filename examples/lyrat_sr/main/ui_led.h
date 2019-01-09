// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _UI_LED_H_
#define _UI_LED_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif


#define LED_GPIO_PIN 22
#define LED_BOOT_PIN 19

enum {
    LED_RESET = 100,
    LED_OFF,
};

esp_err_t ui_led_set(int va_state);

esp_err_t ui_led_init();

#ifdef __cplusplus
}
#endif

#endif /* _UI_LED_H_ */
