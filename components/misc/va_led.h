// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _VA_LED_H_
#define _VA_LED_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include <led_pattern.h>
#include <voice_assistant.h>
#include <voice_assistant_app_cb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VA_LED_TASK_STACK_SZ   (1024 * 12)

extern int number_of_active_alerts;

esp_err_t va_led_set(int va_state);

esp_err_t va_led_init();

void va_led_set_alert(alexa_alert_types_t alert_type, alexa_alert_state_t alert_state);

void va_led_set_dnd(bool dnd_state);

#ifdef __cplusplus
}
#endif

#endif /* _VA_LED_H_ */
