// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>
#include <driver/ledc.h>

#include <led_driver.h>

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL    LEDC_CHANNEL_2

#define TAG "led_driver_esp_ledc"

static bool is_init_done = false;
static ledc_channel_config_t ledc_channel[3];      /* 3 for RGB */
static int ch_gpio[3] = {-1, -1, -1};

esp_err_t esp_ledc_init()
{
    esp_err_t ret;

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
        .freq_hz = 1000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER            // timer index
    };

    // Set configuration of timer0 for high speed channels
    ret = ledc_timer_config(&ledc_timer);

    if (ch_gpio[0] >= 0) {
        ledc_channel[0].channel    = LEDC_HS_CH0_CHANNEL;
        ledc_channel[0].duty       = 0;
        ledc_channel[0].gpio_num   = ch_gpio[0];
        ledc_channel[0].speed_mode = LEDC_HS_MODE;
        ledc_channel[0].timer_sel  = LEDC_HS_TIMER;
        ledc_channel_config(&ledc_channel[0]);
    }
    if (ch_gpio[1] >= 0) {
        ledc_channel[1].channel    = LEDC_HS_CH1_CHANNEL;
        ledc_channel[1].duty       = 0;
        ledc_channel[1].gpio_num   = ch_gpio[1];
        ledc_channel[1].speed_mode = LEDC_HS_MODE;
        ledc_channel[1].timer_sel  = LEDC_HS_TIMER;
        ledc_channel_config(&ledc_channel[1]);
    }
    if (ch_gpio[2] >= 0) {
        ledc_channel[2].channel    = LEDC_HS_CH2_CHANNEL;
        ledc_channel[2].duty       = 0;
        ledc_channel[2].gpio_num   = ch_gpio[2];
        ledc_channel[2].speed_mode = LEDC_HS_MODE;
        ledc_channel[2].timer_sel  = LEDC_HS_TIMER;
        ledc_channel_config(&ledc_channel[2]);
    }

    return ret;
}

void led_driver_set_value(const uint32_t *led_value)
{
    uint8_t ch0_val, ch1_val, ch2_val;
    ch2_val = 0xFF & (led_value[0]);
    ch1_val = 0xFF & (led_value[0] >> 8);
    ch0_val = 0xFF & (led_value[0] >> 16);
    if (ch_gpio[0] >= 0) {
        ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, ch0_val);
        ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
    }
    if (ch_gpio[1] >= 0) {
        ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, ch1_val);
        ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
    }
    if (ch_gpio[2] >= 0) {
        ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, ch2_val);
        ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);
    }
}

bool led_driver_is_init_done()
{
    return is_init_done;
}

esp_err_t led_driver_init(led_driver_config_t *led_driver_config)
{
    ESP_LOGI(TAG, "Initialising led driver");
    if (led_driver_config && led_driver_config->red_gpio >= 0) {
        ch_gpio[led_color_offset[led_driver_config->led_order].red - 1] = led_driver_config->red_gpio;
    }
    if (led_driver_config && led_driver_config->green_gpio >= 0) {
        ch_gpio[led_color_offset[led_driver_config->led_order].green - 1] = led_driver_config->green_gpio;
    }
    if (led_driver_config && led_driver_config->blue_gpio >= 0) {
        ch_gpio[led_color_offset[led_driver_config->led_order].blue - 1] = led_driver_config->blue_gpio;
    }

    esp_err_t err = esp_ledc_init();

    is_init_done = true;
    return err;
}
