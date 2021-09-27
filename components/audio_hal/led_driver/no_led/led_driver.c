// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>

#include <led_driver.h>

#define TAG "led_driver_no_led"

void led_driver_set_value(const uint32_t *led_value)
{
    /* Do nothing */
}

bool led_driver_is_init_done()
{
    return true;
}

esp_err_t led_driver_init(led_driver_config_t *led_driver_config)
{
    /* Do nothing */
    ESP_LOGI(TAG, "Initialising led driver");
    return ESP_OK;
}
