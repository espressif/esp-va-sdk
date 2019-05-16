// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "va_led.h"
#include "ledc_lyrat.h"

#define TAG "LEDC-JUNLAM"

ledc_channel_config_t ledc_channel[LEDC_MAX_CH_NUM];

esp_err_t ledc_lyrat_init()
{
    esp_err_t ret;
    int ch;
    
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
        .freq_hz = 1000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER            // timer index
    };
    
    // Set configuration of timer0 for high speed channels
    ret = ledc_timer_config(&ledc_timer);

    ledc_channel[0].channel    = LEDC_HS_CH0_CHANNEL;
    ledc_channel[0].duty       = 0;
    ledc_channel[0].gpio_num   = LEDC_HS_CH0_GPIO;
    ledc_channel[0].speed_mode = LEDC_HS_MODE;
    ledc_channel[0].timer_sel  = LEDC_HS_TIMER;
    
    ledc_channel[1].channel    = LEDC_HS_CH1_CHANNEL;
    ledc_channel[1].duty       = 0;
    ledc_channel[1].gpio_num   = LEDC_HS_CH1_GPIO;
    ledc_channel[1].speed_mode = LEDC_HS_MODE;
    ledc_channel[1].timer_sel  = LEDC_HS_TIMER;

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_MAX_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    return ret;
}

void va_led_set_pwm(const uint32_t * ui_led_v)
{

    uint8_t red_led_val, green_led_val;
    green_led_val  = 0xFF & (ui_led_v[0]);
    red_led_val = 0xFF & (ui_led_v[0] >> 8);
    ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, green_led_val);
    ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
    ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, red_led_val);
    ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
}