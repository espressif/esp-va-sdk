// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>
#include <led_driver.h>

#define DEFAULT_NUM_OF_LEDS 5

static const char *TAG = "led_driver_hollow_led";

static bool is_init_done = false;
static int num_of_leds = 0;

void led_driver_set_led(uint8_t led_num, uint8_t red, uint8_t green, uint8_t blue)
{
    /* Set the LED which is at led_num with the correct RGB values here. */
}

void led_driver_set_value(const uint32_t *led_value)
{
    int i;
    uint8_t red_led_val, blue_led_val, green_led_val;
    for (i = 0; i < num_of_leds; i++) {
        /* For each LED in led_value, extract the R, G and B values and then set the LED */
        /* This assumes that the led_order is LED_ORDER_RED_GREEN_BLUE. For a different led_order just change the below order. */
        blue_led_val  = 0xFF & (led_value[i]);
        green_led_val = 0xFF & (led_value[i] >> 8);
        red_led_val   = 0xFF & (led_value[i] >> 16);
        led_driver_set_led(i, red_led_val, green_led_val, blue_led_val);
    }
}

bool led_driver_is_init_done()
{
    return is_init_done;
}

esp_err_t led_driver_init(led_driver_config_t *led_driver_config)
{
    ESP_LOGI(TAG, "Initialising led driver");
    /* Initialize your LED driver here */

    if (led_driver_config && led_driver_config->num_of_leds > 0) {
        num_of_leds = led_driver_config->num_of_leds;
    } else {
        num_of_leds = DEFAULT_NUM_OF_LEDS;
    }

    is_init_done = true;
    return ESP_OK;
}
