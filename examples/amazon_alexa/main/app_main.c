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
#include <alexa_local_config.h>

#include <va_mem_utils.h>
#include <scli.h>
#include <va_diag_cli.h>
#include <wifi_cli.h>
#include <media_hal.h>
#include <tone.h>
#include <avs_config.h>
#include <speech_recognizer.h>
#include "va_board.h"
#include "app_auth.h"
#include "app_wifi.h"

#ifdef CONFIG_ALEXA_ENABLE_EQUALIZER
#include "alexa_equalizer.h"
#endif

#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif

#ifdef CONFIG_ALEXA_ENABLE_OTA
#include "app_ota.h"
#endif

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
        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        // We already have print in SYSTEM_EVENT_STA_GOT_IP
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        app_wifi_stop_timeout_timer();
        printf("%s: Connected with IP Address: %s\n", TAG, ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(cm_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    case SYSTEM_EVENT_STA_LOST_IP:
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        app_wifi_stop_timeout_timer();
        printf("%s: Disconnected. Event: %d. Connecting to the AP again\n", TAG, event->event_id);
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
#ifdef CONFIG_PM_ENABLE
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
#else
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
#endif
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

    /* This will never be freed */
    alexa_config_t *va_cfg = va_mem_alloc(sizeof(alexa_config_t), VA_MEM_EXTERNAL);

    if (!va_cfg) {
        ESP_LOGE(TAG, "Failed to alloc voice assistant config");
        abort();
    }
    va_cfg->product_id = CONFIG_ALEXA_PRODUCT_ID;

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
    app_wifi_reset_to_prov_init();
    app_auth_register_cli();
    cm_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    printf("\r");       // To remove a garbage print ">>"
    alexa_auth_delegate_init(NULL, NULL);
    bool provisioned = false;
    if (conn_mgr_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        abort();
    }
    if (app_wifi_get_reset_to_prov() > 0) {
        app_wifi_start_timeout_timer();
        provisioned = false;
        app_wifi_unset_reset_to_prov();
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
    }

    char service_name[20];
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(service_name, sizeof(service_name), "%s%02X%02X", SOFTAP_SSID_PREFIX, mac[4], mac[5]);

    if (!provisioned) {
        va_led_set(LED_RESET);
        printf("%s: Starting provisioning\n", TAG);
        conn_mgr_prov_t prov_type = conn_mgr_prov_mode_ble;
        prov_type.event_cb = alexa_conn_mgr_prov_cb;
        prov_type.cb_user_data = (void *)va_cfg;
        int security = 1;
        const char *pop = "abcd1234";
        const char *service_key = "";
        conn_mgr_prov_start_provisioning(prov_type, security, pop, service_name, service_key);
        printf("\tproof of possession (pop): %s\n", pop);
    } else {
        va_led_set(VA_CAN_START);
        ESP_LOGI(TAG, "Already provisioned, starting station");
        conn_mgr_prov_mem_release();        // This is useful in case of BLE provisioning
        app_prov_done_cb();
        wifi_init_sta();
    }

    xEventGroupWaitBits(cm_event_group, CONNECTED_BIT | PROV_DONE_BIT, false, true, portMAX_DELAY);

    if (!provisioned) {
        va_led_set(VA_CAN_START);
    }

#ifdef CONFIG_ALEXA_ENABLE_EQUALIZER
    alexa_equalizer_init();
#endif

    ret = alexa_local_config_start(va_cfg, service_name);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start local SSDP instance. Some features might not work.");
    }

#ifdef ALEXA_BT
    alexa_bt_a2dp_sink_init();
#endif
    ret = alexa_init(va_cfg);

    if (ret != ESP_OK) {
        while(1) vTaskDelay(2);
    }

    /* This is a blocking call */
    va_dsp_init(speech_recognizer_recognize, speech_recognizer_record);

#ifdef CONFIG_ALEXA_ENABLE_OTA
    /* Doing OTA init after full alexa boot-up. */
    app_ota_init();
#endif
    /* This is only supported with minimum flash size of 8MB. */
    alexa_tone_enable_larger_tones();

#ifdef CONFIG_PM_ENABLE
    rtc_cpu_freq_t max_freq;
    rtc_clk_cpu_freq_from_mhz(CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ, &max_freq);
    esp_pm_config_esp32_t pm_config = {
            .max_cpu_freq = max_freq,
            .min_cpu_freq = RTC_CPU_FREQ_XTAL,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = true
#endif
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config));
    gpio_wakeup_enable(GPIO_NUM_36, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_pm_dump_locks(stdout);
#endif
    return;
}
