// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>

#include <button_driver.h>

#define TAG "button_driver_no_button"

void button_driver_enable_debug(bool enable)
{
    /* Do nothing */
    return;
}

button_event_t button_driver_get_event()
{
    /* Do nothing */
    return BUTTON_EVENT_IDLE;
}

bool button_driver_is_init_done()
{
    return true;
}

esp_err_t button_driver_init(button_driver_config_t *button_driver_config)
{
    /* Do nothing */
    ESP_LOGI(TAG, "Initialising button driver");
    return ESP_OK;
}
