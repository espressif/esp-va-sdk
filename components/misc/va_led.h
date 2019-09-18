// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _VA_LED_H_
#define _VA_LED_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include <voice_assistant.h>
#include <voice_assistant_app_cb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VA_LED_TASK_STACK_SZ   (1024 * 12)

typedef enum va_led_set_colour {
    VA_LED_RED = 0,
    VA_LED_GREEN,
    VA_LED_BLUE,
    VA_LED_WHITE,
} va_led_set_colour_t;

enum {
    LED_RESET = 100,
    LED_OFF,
    LED_OTA,
};

typedef struct {
    int va_led_delay;
    uint32_t va_led_val[12];
    bool loop_en;
} va_led_specs_t;

typedef enum {
    VA_LED_RADIAL_12 = 0,
    VA_LED_LINEAR_11,
    VA_LED_LINEAR_7,
    VA_LED_LINEAR_5,
    VA_LED_SINGLE,
} va_led_animation_type_t;

typedef struct {
    uint8_t va_led_state_sz;
    va_led_specs_t *va_led_state_st;
} va_led_config_t;

enum {
    VA_LED_BOOTUP_1 = 0,
    VA_LED_BOOTUP_2,
    VA_LED_WW_ACTIVE,
    VA_LED_WW_ONGOING,
    VA_LED_WW_DEACTIVATE,
    VA_LED_SPEAKER_VOL,
    VA_LED_SPEAKER_MUTE,
    VA_LED_SPEAKING,
    VA_LED_MIC_OFF_END,
    VA_LED_MIC_OFF_ON,
    VA_LED_MIC_OFF_START,
    VA_LED_ERROR,
    VA_LED_BTCONNECT,
    VA_LED_BTDISCONNECT,
    VA_LED_NTF_QUEUED,
    VA_LED_NTF_INCOMING,
    VA_LED_THINKING,
    VA_LED_ALERT_SHORT,
    VA_LED_ALERT,
    VA_LED_FACTORY_RESET,
    VA_LED_OFF,
    VA_LED_DND,
    VA_LED_OTA,
    VA_LED_PATTERN_MAX,
};

extern int number_of_active_alerts;
extern uint8_t volume_to_set;

esp_err_t va_led_set(int va_state);

esp_err_t va_led_init(va_led_config_t va_led_conf[VA_LED_PATTERN_MAX]);

void va_led_set_pwm(const uint32_t * va_led_v);

void va_led_set_alert(alexa_alert_types_t alert_type, alexa_alert_state_t alert_state);

void va_led_set_dnd(bool dnd_state);

    /* XXX This needn't be here, can be a starting event for va_led.c itself, which is always delivered at the end of va_led_init */
typedef enum va_led_called_states {
    VA_CAN_START = (VA_SET_VOLUME_DONE + 2),
} va_led_called_states_t;


#ifdef __cplusplus
}
#endif

#endif /* _VA_LED_H_ */
