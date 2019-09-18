// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "sdkconfig.h"

#ifdef CONFIG_ALEXA_ENABLE_OTA

#include <cloud_agent.h>

#include <esp_log.h>
#include <esp_timer.h>
#include <voice_assistant.h>
#include <va_led.h>
#include <alexa_local_config.h>
#include "app_ota.h"

#define OTA_CHECK_TIME (10 * 1000)      // 10 seconds

static const char *TAG = "[app_ota]";

static bool ota_started;

static void app_ota_timer_cb(void *arg)
{
    if (ota_started == false) {
        ESP_LOGI(TAG, "No OTA update. Stopping check for OTA.");
        cloud_agent_ota_stop();
    }
}

static void app_ota_start_timer()
{
    ESP_LOGI(TAG, "Starting OTA timer for 10 seconds.");
    esp_timer_handle_t app_ota_timer_handle;
    esp_timer_create_args_t timer_arg = {
        .callback = app_ota_timer_cb,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "app_ota_timer",
    };
    if (esp_timer_create(&timer_arg, &app_ota_timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer for OTA");
        return;
    }
    esp_timer_start_once(app_ota_timer_handle, OTA_CHECK_TIME * 1000);
}

static void app_ota_cloud_agent_cb(cloud_agent_event_t event)
{
    switch (event) {
        case CLOUD_AGENT_INIT_DONE:
            app_ota_start_timer();
            break;

        case CLOUD_AGENT_OTA_START:
            ota_started = true;
            va_led_set(LED_OFF);
            alexa_local_config_stop();
            va_reset();
            va_led_set(LED_OTA);
            break;

        case CLOUD_AGENT_OTA_END:
            va_led_set(LED_OFF);
            break;
    }
}

void app_ota_init()
{
    cloud_agent_device_cfg_t cloud_agent_device_cfg = {
        .name = "ESP Alexa",
        .type = "Speaker",
        .model = "ESP-Alexa",
        .fw_version = "1.0",
    };

    cloud_agent_ota_start(&cloud_agent_device_cfg, app_ota_cloud_agent_cb);
    ESP_LOGI(TAG, "OTA Init done.");
}

#endif /* CONFIG_ALEXA_ENABLE_OTA */
