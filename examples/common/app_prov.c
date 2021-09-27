// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "sdkconfig.h"

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <wifi_provisioning/manager.h>
#ifdef CONFIG_BT_ENABLED
#include <wifi_provisioning/scheme_ble.h>
#else /* !CONFIG_BT_ENABLED */
#include <wifi_provisioning/scheme_softap.h>
#endif /* CONFIG_BT_ENABLED */
#include <esp_log.h>

#include "app_prov.h"

#ifdef CONFIG_ALEXA_ENABLE_CLOUD
#include <qrcode.h>
#define ENABLE_PROV_QR 1
#endif /* CONFIG_ALEXA_ENABLE_CLOUD */

#ifdef CONFIG_BT_ENABLED
static const char *prov_transport = "ble";
#else /* !CONFIG_BT_ENABLED */
static const char *prov_transport = "softap";
#endif

static const char *TAG = "[app_prov]";
static EventGroupHandle_t event_group;
static const int PROVISION_BIT = BIT1;
static bool is_provisioned = false;

static void app_wifi_print_qr(const char *name, const char *pop, const char *transport)
{
#ifdef ENABLE_PROV_QR
    if (!name || !pop || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150];
    snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    "v1", name, pop, transport);
    ESP_LOGI(TAG, "Scan this QR code from the phone app for Provisioning.");
    qrcode_display(payload);
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", "https://rainmaker.espressif.com/qrcode.html", payload);
#endif /* ENABLE_PROV_QR */
}

bool app_prov_get_provisioning_status()
{
    return is_provisioned;
}

void app_prov_set_provisioning_status(bool provisioning_status)
{
    is_provisioned = provisioning_status;
}

void app_prov_wait_for_provisioning()
{
    xEventGroupWaitBits(event_group, PROVISION_BIT, false, true, portMAX_DELAY);
}

void app_prov_stop_provisioning()
{
    wifi_prov_mgr_deinit();
    xEventGroupSetBits(event_group, PROVISION_BIT);
}

/* Event handler for catching provisioning manager events */
static void app_prov_event_handler(void *user_data, wifi_prov_cb_event_t event, void *event_data)
{
    wifi_sta_config_t *wifi_sta_cfg = NULL;
    wifi_prov_sta_fail_reason_t *reason = NULL;
    size_t ssid_len;
    switch (event) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;

        case WIFI_PROV_CRED_RECV:
            wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ssid_len = strnlen((const char *)wifi_sta_cfg->ssid, sizeof(wifi_sta_cfg->ssid));
            ESP_LOGI(TAG, "Received Wi-Fi credentials:\n\tSSID: %.*s\n\tPassword: %s", ssid_len, (const char *)wifi_sta_cfg->ssid, (const char *)wifi_sta_cfg->password);
            break;

        case WIFI_PROV_CRED_FAIL:
            reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason: %s\n\tPlease reset to factory and retry provisioning", (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi AP password incorrect" : "Wi-Fi AP not found");
            break;

        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            app_prov_set_provisioning_status(true);
            break;

        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning stopped");
            wifi_prov_mgr_deinit();
            app_prov_stop_provisioning();
            break;

        default:
            break;
    }
}

void app_prov_start_provisionig(const char *service_name, void *data)
{
    printf("%s: Starting provisioning\n", TAG);

#ifdef CONFIG_BT_ENABLED
    uint8_t custom_service_uuid[16] = {
        /* This is a random uuid. This can be modified if you want to change the BLE uuid. */
        /* 12th and 13th bit will be replaced by internal bits. */
        0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0xfe,
        0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12,
    };
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
#endif

    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    const char *pop = CONFIG_VOICE_ASSISTANT_POP;
    const char *service_key = NULL;

    if (wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start provisioning");
    }
    app_wifi_print_qr(service_name, pop, prov_transport);
    printf("%s: Provisioning started with: \n\tservice name: %s \n\tservice key: %s\n\tproof of possession (pop): %s\n", TAG, service_name, service_key ? service_key : "", pop);
}

void app_prov_init()
{
    event_group = xEventGroupCreate();

    wifi_prov_mgr_config_t config = {
#ifdef CONFIG_BT_ENABLED
        .scheme = wifi_prov_scheme_ble,
#ifdef ALEXA_BT
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
#else /* ALEXA_BT == 0 */
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
#endif /* ALEXA_BT */
#else /* !CONFIG_BT_ENABLED */
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
#endif
        .app_event_handler = {
            .event_cb = app_prov_event_handler,
            .user_data = NULL
        }
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioning_status = false;
    if (wifi_prov_mgr_is_provisioned(&provisioning_status) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        abort();
    }
    app_prov_set_provisioning_status(provisioning_status);
}
