// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

/* Alexa Example

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

#include <conn_mgr.h>
#include <conn_mgr_prov.h>
#include <http_server.h>
#include <protocomm.h>
#include <protocomm_httpd.h>
#include <protocomm_security1.h>

#include <mem_utils.h>
#include <scli.h>
#include <diag_cli.h>
#include <alexa.h>
#include "app_dsp.h"
#include "ui_led.h"

#define SOFTAP_SSID_PREFIX  "ESP-Alexa-"

static const char *TAG = "alexa";

int avs_config_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data);

static alexa_config_t *alexa_cfg;

static EventGroupHandle_t cm_event_group;
const int CONNECTED_BIT = BIT0;
const int PROV_DONE_BIT = BIT1;

static esp_timer_handle_t timer;

static void _stop_softap_cb(void *arg)
{
    conn_mgr_softap_stop();
    esp_timer_delete(timer);
}

#define PROTOCOMM_HTTPD_PORT    80
static int app_cm_cb(conn_mgr_event_t event, void *data)
{
    static protocomm_security_pop_t pop;
    static protocomm_t *pc;
    protocomm_httpd_config_t cfg = { .port = PROTOCOMM_HTTPD_PORT };
#ifdef CONFIG_USE_POP
    pop.data = (uint8_t *)CONFIG_POP,
    pop.len = strlen(CONFIG_POP);
#else
    pop.data = NULL;
    pop.len = 0;
#endif
    switch (event) {
    case CM_EVT_STA_CONNECTED:
        ESP_LOGI(TAG, "got station connected event");
        break;
    case CM_EVT_STA_GOT_IPV4: {
        tcpip_adapter_ip_info_t *ipv4_details = data;
        ESP_LOGI(TAG, "got IPv4:%s",
                 ip4addr_ntoa(&ipv4_details->ip));
        if (timer) {
            esp_timer_start_once(timer, 15000 * 1000U);
        }
        xEventGroupSetBits(cm_event_group, CONNECTED_BIT);
        break;
    }
    case CM_EVT_STA_GOT_IPV6:
        ESP_LOGI(TAG, "got IPV6 address\n");
        break;
    case CM_EVT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "got station disconnected event\n");
        break;
    case CM_EVT_SOFTAP_NW_CRED_RCVD:
        ESP_LOGI(TAG, "got station network credential recieved event");
        break;
    case CM_EVT_SOFTAP_STARTED:
        ESP_LOGI(TAG, "SOFTAP_STARTED event");
        pc = protocomm_new();
        protocomm_httpd_start(pc, &cfg);
        protocomm_set_security(pc, "prov-session", &security1, &pop);
        protocomm_add_endpoint(pc, "avsconfig", avs_config_data_handler, (void *) alexa_cfg);
        protocomm_add_endpoint(pc, "prov-config", wifi_prov_config_data_handler, (void *) &conn_mgr_prov_handlers);
        break;
    case CM_EVT_SOFTAP_STOPPED:
        protocomm_remove_endpoint(pc, "prov-config");
        protocomm_remove_endpoint(pc, "avsconfig");
        protocomm_unset_security(pc, "prov-session");
        protocomm_httpd_stop(pc);
        protocomm_delete(pc);
        pc = NULL;
        xEventGroupSetBits(cm_event_group, PROV_DONE_BIT);
        break;
    default:
        break;
    }
    return 0;
}

extern int i2s_playback_init();

int app_main()
{
    ESP_LOGI(TAG, "==== Alexa SDK version: %s ====", alexa_get_sdk_version());

    alexa_cfg = mem_alloc(sizeof(alexa_config_t), EXTERNAL);
    if (!alexa_cfg) {
        ESP_LOGE(TAG, "Failed to alloc alexa config");
        abort();
    }
    alexa_cfg->auth_delegate.type = auth_type_undecided;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    scli_init();
    diag_register_cli();

    cm_event_group = xEventGroupCreate();
    ret = conn_mgr_init(app_cm_cb);
    ESP_ERROR_CHECK( ret );
    if (conn_mgr_sta_is_configured()) {
        xEventGroupSetBits(cm_event_group, PROV_DONE_BIT);
        conn_mgr_sta_start();
    } else {
        struct conn_mgr_softap_cfg pcfg;
        memset(&pcfg, 0, sizeof(struct conn_mgr_softap_cfg));
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_AP, mac);

        snprintf((char *) pcfg.wifi_cfg.ssid, sizeof(pcfg.wifi_cfg.ssid), "%s%02x%02x", SOFTAP_SSID_PREFIX, mac[4] & 0xff, mac[5] & 0xff);
        pcfg.wifi_cfg.authmode = WIFI_AUTH_OPEN;
        pcfg.wifi_cfg.max_connection = 4;

        conn_mgr_softap_start(&pcfg);
        esp_timer_create_args_t timer_conf = {
            .callback = _stop_softap_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "stop_softap_tm"
        };
        esp_err_t err = esp_timer_create(&timer_conf, &timer);
        ESP_ERROR_CHECK( err );
    }
    xEventGroupWaitBits(cm_event_group, CONNECTED_BIT | PROV_DONE_BIT, false, true, portMAX_DELAY);

    i2s_playback_init();
    app_dsp_init();
    alexa_init(alexa_cfg);

    mem_free(alexa_cfg);
    return ESP_OK;
}
