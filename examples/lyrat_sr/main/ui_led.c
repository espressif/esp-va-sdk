// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <va_mem_utils.h>
#include <ui_led.h>
#include <app_dsp.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <voice_assistant_app_cb.h>

static const char *UI_LED_TAG = "UI_LED";

esp_err_t ui_led_set(int va_state)
{
    if (va_state == VA_LISTENING) {
        gpio_set_level(LED_GPIO_PIN, 1);
    } else if (va_state == LED_RESET){
        gpio_set_level(LED_BOOT_PIN, 1);
    } else if (va_state == LED_OFF) {
        gpio_set_level(LED_GPIO_PIN, 0);
        gpio_set_level(LED_BOOT_PIN, 0);
    } else {
        gpio_set_level(LED_GPIO_PIN, 0);
    }
    return ESP_OK;
}

esp_err_t ui_led_init()
{
    esp_err_t ret;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BIT(LED_GPIO_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    ret = gpio_config(&io_conf);
    gpio_set_level(LED_GPIO_PIN, 0);

    io_conf.pin_bit_mask = BIT(LED_BOOT_PIN);
    ret |= gpio_config(&io_conf);
    gpio_set_level(LED_BOOT_PIN, 0);
    ESP_LOGI(UI_LED_TAG, "LED initialized");
    return ret;
}
