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
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_adc_cal.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BUTTON_SET_VAL         372
#define BUTTON_PLAY_VAL       1016
#define BUTTON_REC_VAL        1657
#define BUTTON_MODE_VAL       2233
#define BUTTON_VOL_LOW_VAL    2815
#define BUTTON_VOL_HIGH_VAL   3570
#define BUTTON_IDLE_VAL       4000

#define BUTTON_TASK_PRIORITY 3
#define BUTTON_STACK_SIZE    2048

typedef enum {
    BUTTON_EVENT_SET = 0x00,
    BUTTON_EVENT_PLAY_PAUSE,
    BUTTON_EVENT_REC,
    BUTTON_EVENT_MODE,
    BUTTON_EVENT_VOL_DOWN,
    BUTTON_EVENT_VOL_UP,
    BUTTON_EVENT_IDLE,
} button_event_t;

typedef struct {
    uint8_t set;
    uint8_t mode;
    uint8_t play_pause;
    uint8_t volume_up;
    uint8_t volume_down;
} button_gpio_t;

#define UI_BUTTON_TASK_BUFFER_SZ   (8 * 1024)

#define V_REF   1100

/* ADC1 GPIO36 for button V+ / V- / X / BT Pair
 * ADC1 GPIO39 for HWID
 */
#define ADC1_BUTTON_CHANNEL (ADC1_CHANNEL_3)

#define ESP_INTR_FLAG_DEFAULT        0

/**
 * @brief  initialize button service
 */
esp_err_t ui_button_init();

#ifdef __cplusplus
}
#endif

#endif /* _UI_BUTTON_H_ */
