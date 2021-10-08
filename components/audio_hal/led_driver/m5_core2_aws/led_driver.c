// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>
#include <core2forAWS.h>

#include <led_driver.h>

#define DEFAULT_NUM_OF_LEDS 10
#define DEFAULT_LEDS_IN_SET 5

static const char *TAG = "led_driver_m5_core2_aws";

static bool is_init_done = false;
static int num_of_leds = 0;
static int leds_in_set = 0;

void led_driver_set_value(const uint32_t *led_value)
{
    /* 2 rows of 5 LEDs is default. Both displaying the same led_pattern for linear_5 */
    for (int i = 0; i < num_of_leds; i++) {
        Core2ForAWS_Sk6812_SetColor(i, led_value[i % leds_in_set]);
        Core2ForAWS_Sk6812_Show();
    }
}

bool led_driver_is_init_done()
{
    return is_init_done;
}

esp_err_t led_driver_init(led_driver_config_t *led_driver_config)
{
    ESP_LOGI(TAG, "Initialising led driver");

    /* M5_Core2_AWS specific LED driver is already initalized in Core2ForAWS_Init() */

    if (led_driver_config && led_driver_config->num_of_leds > 0) {
        num_of_leds = led_driver_config->num_of_leds;
    } else {
        num_of_leds = DEFAULT_NUM_OF_LEDS;
    }

    if (led_driver_config && led_driver_config->leds_in_set > 0) {
        leds_in_set = led_driver_config->leds_in_set;
    } else {
        leds_in_set = DEFAULT_LEDS_IN_SET;
    }

    is_init_done = true;
    return ESP_OK;
}
