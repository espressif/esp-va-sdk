// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

/* Alexa + AWS IoT Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <esp_pm.h>
#include <nvs_flash.h>

#include <conn_mgr_prov.h>
#include <conn_mgr_prov_mode_softap.h>

#include <alexa.h>
#include <va_mem_utils.h>
#include <scli.h>
#include <diag_cli.h>
#include <wifi_cli.h>
#include <avs_config.h>

#include "app_dsp.h"
#include "ui_led.h"

#ifdef CONFIG_AWS_IOT_SDK
extern void aws_iot_init();
#endif

#define SOFTAP_SSID_PREFIX  "ESP-Alexa-"

static const char *TAG = "alexa";

static EventGroupHandle_t cm_event_group;
const int CONNECTED_BIT = BIT0;

static alexa_config_t *alexa_cfg;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    conn_mgr_prov_event_handler(ctx, event);

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Connected with IP Address:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(cm_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected, connecting to the AP again...\n");
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void wifi_init_sta()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_start() );
}

extern int i2s_playback_init();

int app_main()
{
    ESP_LOGI(TAG, "==== Alexa SDK version: %s ====", alexa_get_sdk_version());

    alexa_cfg = va_mem_alloc(sizeof(alexa_config_t), VA_MEM_EXTERNAL);
    if (!alexa_cfg) {
        ESP_LOGE(TAG, "Failed to alloc alexa config");
        abort();
    }

    alexa_cfg->auth_delegate.type = auth_type_subsequent;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    scli_init();
    diag_register_cli();
    wifi_register_cli();
    cm_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    bool provisioned = false;
    if (conn_mgr_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        abort();
    }
    if (! provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
        char service_name[20];
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(service_name, sizeof(service_name), "%s%02X%02X", SOFTAP_SSID_PREFIX, mac[4], mac[5]);

        conn_mgr_prov_t prov_type = conn_mgr_prov_mode_softap;
        prov_type.event_cb = alexa_conn_mgr_prov_cb;
        prov_type.cb_user_data = (void *)alexa_cfg;
        int security = 1;
        const char *pop = "abcd1234";
        const char *service_key = "";
        conn_mgr_prov_start_provisioning(prov_type, security, pop, service_name, service_key);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting station");
        conn_mgr_prov_mem_release();        // This is useful in case of BLE provisioning
        wifi_init_sta();
    }

    xEventGroupWaitBits(cm_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    i2s_playback_init();
    app_dsp_init();
    if (alexa_init(alexa_cfg) != ESP_OK) {
        while(1) vTaskDelay(2);
    }
#ifdef CONFIG_AWS_IOT_SDK
    aws_iot_init();
#endif
    va_mem_free(alexa_cfg);
    return ESP_OK;
}
