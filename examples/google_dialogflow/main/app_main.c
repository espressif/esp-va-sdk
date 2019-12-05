// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <esp_pm.h>
#include <nvs_flash.h>

#include <voice_assistant.h>
#include <dialogflow.h>

#include <va_mem_utils.h>
#include <scli.h>
#include <va_diag_cli.h>
#include <wifi_cli.h>
#include <media_hal.h>
#include <tone.h>
#include <auth_delegate.h>
#include <speech_recognizer.h>
#include <va_board.h>

static const char *TAG = "[app_main]";
bool provisioning_state = false;

static EventGroupHandle_t cm_event_group;
const int CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
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

esp_err_t wifi_get_provisioning_state()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init wifi");
        return ESP_FAIL;
    }

    /* Get WiFi Station configuration */
    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) != ESP_OK) {
        provisioning_state = false;
        return ESP_FAIL;
    }

    if (strlen((const char*) wifi_cfg.sta.ssid)) {
        provisioning_state = true;
        ESP_LOGI(TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
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

void app_main()
{
    ESP_LOGI(TAG, "==== Voice Assistant SDK version: %s ====", va_get_sdk_version());

    /* This will never be freed */
    dialogflow_config_t *va_cfg = va_mem_alloc(sizeof(dialogflow_config_t), VA_MEM_EXTERNAL);

    if (!va_cfg) {
        ESP_LOGE(TAG, "Failed to alloc voice assistant config");
        abort();
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    va_board_init();
    static media_hal_config_t media_hal_conf = MEDIA_HAL_DEFAULT();
    media_hal_init(&media_hal_conf);

    va_board_button_init();
    va_board_led_init();

    scli_init();
    va_diag_register_cli();
    wifi_register_cli();
    cm_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    auth_delegate_init(NULL, NULL);
    if (wifi_get_provisioning_state() != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        abort();
    }
    if (!provisioning_state) {
        va_led_set(LED_RESET);
        ESP_LOGI(TAG, "***************************");
        ESP_LOGI(TAG, "** Starting provisioning **");
        ESP_LOGI(TAG, "***************************");
        ESP_LOGI(TAG, "Refer the README-Dialogflow.md and enter the CLI commands. Make sure to enter the nvs-set commands first and then the wifi-set command.");
    } else {
        va_led_set(VA_CAN_START);
        ESP_LOGI(TAG, "Already provisioned, starting station");
        wifi_init_sta();
    }

    xEventGroupWaitBits(cm_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    if (!provisioning_state) {
        va_led_set(VA_CAN_START);
    }

    va_cfg->device_config.project_name = "project-name-default";    // Enter your dialogflow project name here.
    ret = dialogflow_init(va_cfg);

    if (ret != ESP_OK) {
        while(1) vTaskDelay(2);
    }
    /* This is a blocking call */
    va_dsp_init(speech_recognizer_recognize, speech_recognizer_record);
    return;
}
