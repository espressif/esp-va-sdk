// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <esp_pm.h>
#include <nvs_flash.h>

#include <conn_mgr_prov.h>
#include <conn_mgr_prov_mode_ble.h>

#include <voice_assistant.h>
#include <alexa.h>

#include <va_mem_utils.h>
#include <scli.h>
#include <va_diag_cli.h>
#include <wifi_cli.h>
#include "app_auth.h"
#include <media_hal.h>
#include <tone.h>
#include <avs_config.h>
#include "va_dsp.h"
#include "va_board.h"

#define SOFTAP_SSID_PREFIX  "ESP-Alexa-"

static const char *TAG = "[app_main]";

static EventGroupHandle_t cm_event_group;
const int CONNECTED_BIT = BIT0;
const int PROV_DONE_BIT = BIT1;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    conn_mgr_prov_event_handler(ctx, event);

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        printf("%s: Connected with IP Address: %s\n", TAG, ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(cm_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        printf("%s: Disconnected. Connecting to the AP again\n", TAG);
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
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

#define MEDIA_HAL_DEFAULT()     \
    {   \
        .op_mode    = MEDIA_HAL_MODE_SLAVE,              \
        .adc_input  = MEDIA_HAL_ADC_INPUT_LINE1,         \
        .dac_output = MEDIA_HAL_DAC_OUTPUT_ALL,          \
        .codec_mode = MEDIA_HAL_CODEC_MODE_BOTH,         \
        .bit_length = MEDIA_HAL_BIT_LENGTH_16BITS,       \
        .format     = MEDIA_HAL_I2S_NORMAL,              \
        .port_num = 0,                          \
    };

void app_prov_done_cb()
{
    xEventGroupSetBits(cm_event_group, PROV_DONE_BIT);
}

void app_main()
{
    ESP_LOGI(TAG, "==== Voice Assistant SDK version: %s ====", va_get_sdk_version());

    alexa_config_t *va_cfg = va_mem_alloc(sizeof(alexa_config_t), VA_MEM_EXTERNAL);

    if (!va_cfg) {
        ESP_LOGE(TAG, "Failed to alloc voice assistant config");
        abort();
    }
    va_cfg->auth_delegate.type = auth_type_subsequent;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    static media_hal_config_t media_hal_conf = MEDIA_HAL_DEFAULT();
    media_hal_init(&media_hal_conf);

    va_board_button_init();
    va_board_led_init();

    scli_init();
    va_diag_register_cli();
    wifi_register_cli();
    app_auth_register_cli();
    cm_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    printf("\r");       // To remove a garbage print ">>"
    bool provisioned = false;
    if (conn_mgr_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        abort();
    }
    if (!provisioned) {
        printf("%s: Starting provisioning\n", TAG);
        char service_name[20];
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(service_name, sizeof(service_name), "%s%02X%02X", SOFTAP_SSID_PREFIX, mac[4], mac[5]);

        conn_mgr_prov_t prov_type = conn_mgr_prov_mode_ble;
        prov_type.event_cb = alexa_conn_mgr_prov_cb;
        prov_type.cb_user_data = (void *)va_cfg;
        int security = 1;
        const char *pop = "abcd1234";
        const char *service_key = "";
        conn_mgr_prov_start_provisioning(prov_type, security, pop, service_name, service_key);
        printf("\tproof of possession (pop): %s\n", pop);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting station");
        conn_mgr_prov_mem_release();        // This is useful in case of BLE provisioning
        app_prov_done_cb();
        wifi_init_sta();
    }

    xEventGroupWaitBits(cm_event_group, CONNECTED_BIT | PROV_DONE_BIT, false, true, portMAX_DELAY);
    va_led_set(VA_CAN_START);
    va_board_init();
    ret = alexa_init(va_cfg);
#ifdef ALEXA_BT
    alexa_bluetooth_init();
#endif

    if (ret != ESP_OK) {
        while(1) vTaskDelay(2);
    }
    /* This is a blocking call */
    va_dsp_init();
    /* This is only supported with minimum flash size of 8MB. */
    alexa_tone_enable_larger_tones();
    va_mem_free(va_cfg);
    return;

}
