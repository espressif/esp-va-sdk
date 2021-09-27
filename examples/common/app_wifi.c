// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>

#include <prompt.h>
#include <va_nvs_utils.h>
#include <va_button.h>
#include <va_ui.h>
#include "app_prov.h"
#include "app_wifi.h"
#include <driver/adc.h>

#define WIFI_RESET_TIMER_TIMEOUT  (3 * 65 * 1000 * 1000) // 3 minutes in usec, with some margin for local communication delay, if any

static const char *TAG = "[app_wifi]";
static EventGroupHandle_t event_group;
static const int CONNECTION_BIT = BIT0;
static const char *app_wifi_nvs_key_wifi_reset_bit = "wifi_reset_bit";
static bool wifi_reset_timer_ongoing;
static esp_timer_handle_t wifi_reset_timer_handle;

static void app_wifi_timer_wifi_reset_task()
{
    printf("%s: WiFi reset timed out. Restarting.", TAG);
    prompt_play(PROMPT_SETUP_MODE_OFF);
    va_ui_set_state(VA_UI_OFF);
    /* Wait for prompt to complete playing */
    vTaskDelay(5000/portTICK_PERIOD_MS);
    esp_restart();
}

static void app_wifi_timer_wifi_reset_cb()
{
    TaskHandle_t wifi_reset_task_handle;
    xTaskCreate(app_wifi_timer_wifi_reset_task, "app_wifi_reset", 2048, NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &wifi_reset_task_handle);
}

static void app_wifi_start_wifi_reset_timer()
{
    esp_timer_init();
    esp_timer_create_args_t timer_arg = {
        .callback = app_wifi_timer_wifi_reset_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "WiFi reset timeout timer",
    };
    if (esp_timer_create(&timer_arg, &wifi_reset_timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer");
        abort();
    }
    esp_timer_start_once(wifi_reset_timer_handle, WIFI_RESET_TIMER_TIMEOUT);
    wifi_reset_timer_ongoing = true;
}

static void app_wifi_stop_wifi_reset_timer()
{
    if (wifi_reset_timer_ongoing) {
        esp_timer_stop(wifi_reset_timer_handle);
        esp_timer_delete(wifi_reset_timer_handle);
        wifi_reset_timer_ongoing = false;
    }
}

static void app_wifi_set_wifi_reset_bit(void *arg)
{
    if (va_nvs_set_i8(app_wifi_nvs_key_wifi_reset_bit, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Error setting wifi_reset_bit");
    }
    esp_restart();
}

static int8_t app_wifi_get_wifi_reset_bit()
{
    int8_t wifi_reset_bit;
    if (va_nvs_get_i8(app_wifi_nvs_key_wifi_reset_bit, &wifi_reset_bit) != ESP_OK) {
        return -1;
    }
    return wifi_reset_bit;
}

static void app_wifi_unset_wifi_reset_bit()
{
    if (va_nvs_set_i8(app_wifi_nvs_key_wifi_reset_bit, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Error unsetting wifi_reset_bit");
    }
}

int app_wifi_check_wifi_reset()
{
    if (app_wifi_get_wifi_reset_bit() > 0) {
        app_wifi_start_wifi_reset_timer();
        app_prov_set_provisioning_status(false);
        app_wifi_unset_wifi_reset_bit();
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        return 1;
    }
    return 0;
}

int app_wifi_init_wifi_reset()
{
    va_button_register_wifi_reset_cb(app_wifi_set_wifi_reset_bit);
    return 0;
}

void app_wifi_wait_for_connection(uint32_t wait)
{
    xEventGroupWaitBits(event_group, CONNECTION_BIT, false, true, wait);
}

static void app_wifi_connection_done()
{
    xEventGroupSetBits(event_group, CONNECTION_BIT);
}

static void app_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    static int disconnect_count = 0;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
            esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED || event_id == WIFI_EVENT_STA_AUTHMODE_CHANGE || event_id == WIFI_EVENT_STA_WPS_ER_FAILED || event_id == WIFI_EVENT_STA_WPS_ER_TIMEOUT) {
#define MAX_RECONNECT_ATTEMPTS  6 /* Callback gets called twice for a single disconnect event, with different reason codes. Essentially no. of retries are 3 */
            app_wifi_stop_wifi_reset_timer();
            if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
                wifi_event_sta_disconnected_t* disconnect_data = (wifi_event_sta_disconnected_t*) event_data;
                /* You may want to look in esp_wifi_types.h for reason strings */
                printf("%s: Disconnect event: %d, reason: %d\n", TAG, event_id, disconnect_data->reason);
            } else {
                printf("%s: Disconnected. Event: %d\n", TAG, event_id);
            }
            disconnect_count++;
            if (!app_prov_get_provisioning_status()) {
                if (disconnect_count > MAX_RECONNECT_ATTEMPTS) {
                    ESP_LOGE(TAG, "Not able to connect. Incorrect password? Please re-provision the device");
                    prompt_play(PROMPT_WIFI_PASSWORD_INCORRECT);
                    disconnect_count = 0;
                } else {
                    esp_wifi_connect();
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(3 * 1000));
                esp_wifi_connect();
            }
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            app_wifi_stop_wifi_reset_timer();
            printf("%s: Connected with IP Address: %s\n", TAG, ip4addr_ntoa(&event->ip_info.ip));
            app_wifi_connection_done();
        } else if (event_id == IP_EVENT_STA_LOST_IP) {
            app_wifi_stop_wifi_reset_timer();
            printf("%s: Disconnected. Connecting to the AP again\n", TAG);
            esp_wifi_connect();
        }
    }
}

void app_wifi_start_station()
{
    /* Enable all-channel scan instead of default fast scan */
    wifi_config_t wifi_config = {0};
    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
}

void app_wifi_init()
{
    adc_power_acquire();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &app_wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &app_wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    event_group = xEventGroupCreate();
}
