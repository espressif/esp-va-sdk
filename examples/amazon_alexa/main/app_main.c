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

#include <voice_assistant.h>
#include <alexa.h>
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
#include <speech_recognizer.h>
#include "va_board.h"
#include <app_wifi.h>
#include <app_prov.h>
#include "app_auth.h"
#include <avs_config.h>

#ifdef CONFIG_ALEXA_ENABLE_EQUALIZER
#include "alexa_equalizer.h"
#endif

#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif

#ifdef CONFIG_ALEXA_ENABLE_CLOUD
#include <app_cloud.h>
#endif /* CONFIG_ALEXA_ENABLE_CLOUD */

#define SERVICENAME_SSID_PREFIX  "ESP-Alexa-"

static const char *TAG = "[app_main]";

void app_setup_mode_prompt_cb(void *arg)
{
    prompt_play(PROMPT_SETUP_MODE);
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
    alexa_auth_delegate_init(NULL, NULL);

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
#endif /* CONFIG_ALEXA_ENABLE_CLOUD */
    alexa_provisioning_init(va_cfg);

    app_prov_init();
    app_wifi_init_wifi_reset();

    va_board_init(); /* Initialize media_hal, media_hal_playback, board buttons and led patters */
    va_button_init();
    va_led_init();

    scli_init(); /* Initialize CLI */
    va_diag_register_cli(); /* Add diagnostic functions to CLI */
    wifi_register_cli();
    app_auth_register_cli();
    speaker_diag_register_cli(); /* Add CLI cmds for vol +/- */

#ifdef ALEXA_BT
    alexa_bt_init();
    if (alexa_bt_only_mode_init() == ESP_OK) {
        /* If this is BT only mode, we do not go for Alexa functionality. */
        return;
    }
#endif

    printf("\r");       // To remove a garbage print ">>"

    alexa_early_init();

    /* XXX: Has to be done after tcpip_adapter_init */
    alexa_init_config(va_cfg);
    char service_name[20];
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(service_name, sizeof(service_name), "%s%02X%02X", SERVICENAME_SSID_PREFIX, mac[4], mac[5]);

    int wifi_reset_status = app_wifi_check_wifi_reset();

    if (!app_prov_get_provisioning_status()) {
        va_button_register_setup_mode_cb(app_setup_mode_prompt_cb);
        app_prov_start_provisionig(service_name, va_cfg);
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
        ESP_LOGI(TAG, "Already provisioned, starting station");
        va_ui_set_state(VA_UI_CAN_START);
        app_prov_stop_provisioning();
        app_wifi_start_station();
    }

    app_wifi_wait_for_connection(30000 / portTICK_PERIOD_MS);

#ifdef CONFIG_ALEXA_ENABLE_EQUALIZER
    alexa_equalizer_init();
#endif

    ret = alexa_local_config_start((amazon_config_t *) va_cfg, service_name);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start local SSDP instance. Some features might not work.");
    }

    ret = alexa_init(va_cfg);

    if (ret != ESP_OK) {
        while(1) vTaskDelay(2);
    }

    /* This is a blocking call */
    va_dsp_init(speech_recognizer_recognize, speech_recognizer_record, va_button_notify_mute);
    va_boot_dsp_signal();

#ifndef ENABLE_4MB_FLASH_PARTITION
    /* This is only supported with minimum flash size of 8MB. */
    alexa_tone_enable_larger_tones();
#endif
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
