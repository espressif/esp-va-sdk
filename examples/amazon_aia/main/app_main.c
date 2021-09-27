// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include <voice_assistant.h>
#include <aia.h>
#include <alexa_local_config.h>

#include <va_mem_utils.h>
#include <va_ui.h>
#include <va_button.h>
#include <va_led.h>
#include <scli.h>
#include <va_diag_cli.h>
#include <wifi_cli.h>
#include <tone.h>
#include <prompt.h>
#include <speaker.h>
#include <avs_config.h>
#include <auth_delegate.h>
#include <speech_recognizer.h>
#include "va_board.h"
#include <app_wifi.h>
#include <app_prov.h>
#include "app_auth.h"

#ifdef CONFIG_ALEXA_ENABLE_CLOUD
#include <app_cloud.h>
#else /* !CONFIG_ALEXA_ENABLE_CLOUD */
#include <app_smart_home.h>
#endif /* CONFIG_ALEXA_ENABLE_CLOUD */

#define SERVICENAME_SSID_PREFIX  "ESP-Alexa-"
#define MAX_CLIENT_ID_LEN (12 + 1) /* 12 bytes of mac address + 1 byte of \0 */

static const char *TAG = "[app_main]";

#define FACTORY_PARTITION_NAME    "fctry_aia"
#define DEVICE_NAMESPACE         "device"

void app_setup_mode_prompt_cb(void *arg)
{
    prompt_play(PROMPT_SETUP_MODE);
}

char *app_nvs_alloc_and_get_str(const char *key)
{
    nvs_handle handle;
    esp_err_t err;
    if ((err = nvs_open_from_partition(FACTORY_PARTITION_NAME, DEVICE_NAMESPACE,
                                NVS_READONLY, &handle)) != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return NULL;
    }
    size_t required_size = 0;
    if ((err = nvs_get_blob(handle, key, NULL, &required_size)) != ESP_OK) {
        ESP_LOGI(TAG, "Error reading %s from nvs of size %d. Error code: %d", key, required_size, err);
        return NULL;
    }
    char *value = va_mem_alloc(required_size + 1, VA_MEM_EXTERNAL); /* + 1 for NULL termination */
    if (value) {
        nvs_get_blob(handle, key, value, &required_size);
        value[required_size] = '\0';                                /* value goes from 0 to ((required_size + 1) - 1) */

        /* Remove extra newlines from the end */
        while (value[--required_size] == '\n') {
            ESP_LOGI(TAG, "Removing newline for %s", key);
            value[required_size] = '\0';                            /* -1 already subtracted */
        }

    }
    nvs_close(handle);
    return value;
}

void app_get_device_config(aia_config_t *va_cfg)
{
    if (nvs_flash_init_partition(FACTORY_PARTITION_NAME) != ESP_OK) {
        ESP_LOGE(TAG, "NVS Flash factory partition init failed.");
        return;
    }

    /* Server cert is taken from global CA store. If it needs to be overwritten, add it in global CA store or set it from here. */
    // va_cfg->device_config.aws_root_ca_pem_cert = app_nvs_alloc_and_get_str("server_cert");

    va_cfg->device_config.certificate_pem_crt_cert = app_nvs_alloc_and_get_str("client_cert");
    va_cfg->device_config.private_pem_crt_cert = app_nvs_alloc_and_get_str("client_key");
    va_cfg->device_config.aws_endpoint = app_nvs_alloc_and_get_str("mqtt_host");        /* If this is not present in the 'fctry' partition, then the one mentioned the menuconfig is used. */
    if (!va_cfg->device_config.aws_endpoint) {
        va_cfg->device_config.aws_endpoint = CONFIG_ALEXA_AWS_ENDPOINT;
        printf("%s: Using the non-NVS aws_endpoint: %s\n", TAG, va_cfg->device_config.aws_endpoint);
    }
    va_cfg->device_config.aws_account_id = app_nvs_alloc_and_get_str("account_id");     /* If this is not present in the 'fctry' partition, then the one mentioned in the menuconfig is used. */
    if (!va_cfg->device_config.aws_account_id) {
        va_cfg->device_config.aws_account_id = CONFIG_ALEXA_AWS_ACCOUNT_ID;
        printf("%s: Using the non-NVS aws_account_id: %s\n", TAG, va_cfg->device_config.aws_account_id);
    }
    va_cfg->device_config.client_id = app_nvs_alloc_and_get_str("device_id");           /* If this is not present in the 'fctry' partition, then it is set as the MAC address of the device. */
    if (!va_cfg->device_config.client_id) {
        va_cfg->device_config.client_id = (char *)va_mem_alloc(MAX_CLIENT_ID_LEN, VA_MEM_EXTERNAL);
        if (va_cfg->device_config.client_id == NULL) {
            ESP_LOGE(TAG, "Client id not allocated");
        }
        /* Deriving client_id from the MAC address. */
        uint8_t mac_int[6] = {0};
        esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_int);
        snprintf(va_cfg->device_config.client_id, MAX_CLIENT_ID_LEN, "%02x%02x%02x%02x%02x%02x", mac_int[0], mac_int[1], mac_int[2], mac_int[3], mac_int[4], mac_int[5]);
        printf("%s: Using the non-NVS client_id: %s\n", TAG, va_cfg->device_config.client_id);
    }

    if (!va_cfg->device_config.certificate_pem_crt_cert) {
        ESP_LOGE(TAG, "AIA device certificates not found. Please flash the certificates at the correct location and reboot the device to proceed.");
        while(1) {
            vTaskDelay(5000/portTICK_PERIOD_MS);
        }
    }
}

void app_main()
{
    ESP_LOGI(TAG, "==== Voice Assistant SDK version: %s ====", va_get_sdk_version());

    amazon_config_t *amazon_cfg = va_mem_alloc(sizeof(amazon_config_t), VA_MEM_EXTERNAL);
    if (!amazon_cfg) {
        ESP_LOGE(TAG, "Failed to alloc voice assistant config");
        abort();
    }
    amazon_cfg->product_id = CONFIG_ALEXA_PRODUCT_ID;

    /* This will never be freed */
    aia_config_t *va_cfg = va_mem_alloc(sizeof(aia_config_t), VA_MEM_EXTERNAL);
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

    app_wifi_init();

    /* app_cloud_init() and alexa_provisioning_init() should be called after wifi_init(), but before prov_init() */
#ifdef CONFIG_ALEXA_ENABLE_CLOUD
    app_cloud_init();
#else /* !CONFIG_ALEXA_ENABLE_CLOUD */
    /* To enable smart home, uncomment this. For more information check the Readme. */
    // app_smart_home_init();
#endif /* CONFIG_ALEXA_ENABLE_CLOUD */
    alexa_provisioning_init(amazon_cfg);

    app_prov_init();
    app_wifi_init_wifi_reset();
    app_get_device_config(va_cfg);

    va_board_init(); /* Initialize media_hal, media_hal_playback, board buttons and led patters */
    va_button_init();
    va_led_init();

    scli_init(); /* Initialize CLI */
    va_diag_register_cli(); /* Add diagnostic functions to CLI */
    wifi_register_cli();
    app_auth_register_cli();
    speaker_diag_register_cli(); /* Add CLI cmds for vol +/- */

    printf("\r");       // To remove a garbage print ">>"
    auth_delegate_init(alexa_signin_handler, alexa_signout_handler);

    aia_early_init();

    char service_name[20];
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(service_name, sizeof(service_name), "%s%02X%02X", SERVICENAME_SSID_PREFIX, mac[4], mac[5]);

    int wifi_reset_status = app_wifi_check_wifi_reset();

    if (!app_prov_get_provisioning_status()) {
        va_button_register_setup_mode_cb(app_setup_mode_prompt_cb);
        app_prov_start_provisionig(service_name, amazon_cfg);
        va_ui_set_state(VA_UI_RESET);
        if (wifi_reset_status) {
            /* Boot for additional setup */
            prompt_play(PROMPT_SETUP_MODE_ON);
        } else {
            /* Boot after factory reset */
            prompt_play(PROMPT_HELLO);
            prompt_play(PROMPT_SETUP_MODE);
        }
        app_prov_wait_for_provisioning();
        va_button_register_setup_mode_cb(NULL);
        va_ui_set_state(VA_UI_CAN_START);
    } else {
        va_ui_set_state(VA_UI_CAN_START);
        ESP_LOGI(TAG, "Already provisioned, starting station");
        app_prov_stop_provisioning();
        app_wifi_start_station();
    }

    app_wifi_wait_for_connection(30000 / portTICK_PERIOD_MS);

    ret = alexa_local_config_start(amazon_cfg, service_name);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start local SSDP instance. Some features might not work.");
    }

    ret = aia_init(va_cfg);

    if (ret != ESP_OK) {
        while(1) vTaskDelay(2);
    }

    /* This is a blocking call */
    va_dsp_init(speech_recognizer_recognize, speech_recognizer_record, va_button_notify_mute);
    va_boot_dsp_signal();

#ifdef CONFIG_HALF_DUPLEX_I2S_MODE
    /* Disable 'Start of Response' and 'End of Response' tones for half duplex boards */
    tone_set_sor_state(0);
    tone_set_eor_state(0);
#endif

#ifdef CONFIG_PM_ENABLE
    int xtal_freq = (int) rtc_clk_xtal_freq_get();
    esp_pm_config_esp32_t pm_config = {
            .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
            .min_freq_mhz = xtal_freq,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = true
#endif
    };

    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    gpio_wakeup_enable(GPIO_NUM_36, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
#ifdef CONFIG_ULP_COPROC_ENABLED
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
#endif
    //esp_pm_dump_locks(stdout);
#endif
    return;
}
