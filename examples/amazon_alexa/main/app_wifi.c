// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_log.h>
#include <esp_system.h>

#include <va_nvs_utils.h>
#include <va_button.h>
#include <va_led.h>
#include "app_wifi.h"

static const char *TAG = "[app_wifi]";
static const char *app_wifi_reset_prov_flag_key = "prov_rst_flag";

#define WIFI_RESET_TO_PROV_TIMEOUT  (3 * 65 * 1000 * 1000) // 3 minutes in usec, with some margin for local communication delay, if any

static bool timeout_timer_running;

static void app_wifi_timeout_reset()
{
    printf("%s: WiFi reset timed out. Restarting.", TAG);
    va_led_set(LED_OFF);
    vTaskDelay(500/portTICK_PERIOD_MS);
    esp_restart();
}

static void app_wifi_reset_timeout_cb()
{
    TaskHandle_t wifi_reset_task_handle;
    xTaskCreate(app_wifi_timeout_reset, "wifi-timeout-reset", 2048, NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &wifi_reset_task_handle);
}

static void app_wifi_reset_to_prov(void *arg)
{
    if (va_nvs_set_i8(app_wifi_reset_prov_flag_key, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Error setting reset to provisioning flag");
    }
    esp_restart();
}

int app_wifi_reset_to_prov_init()
{
    va_button_register_wifi_reset_cb(app_wifi_reset_to_prov);
    return 0;
}

int8_t app_wifi_get_reset_to_prov()
{
    int8_t reset_to_prov_flag;
    if (va_nvs_get_i8(app_wifi_reset_prov_flag_key, &reset_to_prov_flag) != ESP_OK) {
        return -1;
    }
    return reset_to_prov_flag;
}

void app_wifi_unset_reset_to_prov()
{
    if (va_nvs_set_i8(app_wifi_reset_prov_flag_key, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Error unsetting reset to provisioning flag");
    }
}

esp_timer_handle_t wifi_reset_to_prov_timeout_timer_handle;
void app_wifi_start_timeout_timer()
{
    esp_timer_init();
    esp_timer_create_args_t timer_arg = {
        .callback = app_wifi_reset_timeout_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "WiFi reset timeout timer",
    };
    if (esp_timer_create(&timer_arg, &wifi_reset_to_prov_timeout_timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer");
        abort();
    }
    esp_timer_start_once(wifi_reset_to_prov_timeout_timer_handle, WIFI_RESET_TO_PROV_TIMEOUT);
    timeout_timer_running = true;
}

void app_wifi_stop_timeout_timer()
{
    if (timeout_timer_running) {
        esp_timer_stop(wifi_reset_to_prov_timeout_timer_handle);
        esp_timer_delete(wifi_reset_to_prov_timeout_timer_handle);
        timeout_timer_running = false;
    }
}

