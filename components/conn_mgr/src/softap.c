// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <conn_mgr.h>
#include "conn_mgr_internal.h"
#include <esp_wifi.h>
#include <esp_log.h>

static const char *TAG = "softap_sm";

static int do_softap_start(struct conn_mgr_softap_cfg *softap_cfg)
{
    if (softap_cfg->wifi_cfg.authmode != WIFI_AUTH_OPEN) {
        ESP_LOGI(TAG, "SoftAP SSID: %s, Passphrase: %s", softap_cfg->wifi_cfg.ssid, softap_cfg->wifi_cfg.password);
    } else {
        ESP_LOGI(TAG, "SoftAP SSID: %s", softap_cfg->wifi_cfg.ssid);
    }

    if (esp_wifi_set_mode(WIFI_MODE_AP) != 0) {
        ESP_LOGE(TAG, "Couldn't set station mode\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }
    if (esp_wifi_set_config(ESP_IF_WIFI_AP, (wifi_config_t *) &softap_cfg->wifi_cfg) != 0) {
        ESP_LOGE(TAG, "Couldn't set Wi-Fi configuration\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }
    if (esp_wifi_start() != 0) {
        ESP_LOGE(TAG, "Couldn't start Wi-Fi\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }
    free(softap_cfg);
    return 0;
}

void conn_mgr_drive_softap(struct conn_mgr_msg *msg)
{
    int ret;
    enum cm_softap_state next = g_conn_mgr.cm_softap.state;
    static struct conn_mgr_softap_handlers softap_handlers;
    static void *softap_handlers_arg;

    if (g_conn_mgr.softap_enabled == false) {
        return;
    }

    switch (g_conn_mgr.cm_softap.state) {
    case CM_SOFTAP_STATE_INIT:
        if (msg->event == CMI_CMD_SOFTAP_START) {
            softap_handlers = ((struct conn_mgr_softap_cfg *) msg->user_data)->handlers;
            softap_handlers_arg = ((struct conn_mgr_softap_cfg *) msg->user_data)->arg;
            ret = do_softap_start(msg->user_data);
            if (ret == 0) {
                next = CM_SOFTAP_STATE_STARTED;
            } else {
                ESP_LOGI(TAG, "failure in starting soft-ap");
                /* XXX Notify the application */
            }
        }
        break;
    case CM_SOFTAP_STATE_STARTED:
        /* Let's just keep a single state for all of this for now. We
         * may have to split it up to consume the multiple
         * DISCONNECTED events that we receive from the Wi-Fi
         * subsystem soon.
         */
        if (msg->event == CMI_EVENT_FROM_WIFI) {
            if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_AP_START) {
                if (softap_handlers.softap_started) {
                    softap_handlers.softap_started(softap_handlers_arg);
                }
                conn_mgr_send_event_out(CM_EVT_SOFTAP_STARTED, NULL);
            }
        }
        if (msg->event == CMI_CMD_SOFTAP_CUSTOM) {
            conn_mgr_send_event_out(CM_EVT_SOFTAP_NW_CRED_RCVD, NULL);
        }
        if (msg->event == CMI_CMD_SOFTAP_STOP) {
            conn_mgr_send_event_out(CM_EVT_SOFTAP_STOPPED, NULL);
            if (softap_handlers.softap_stopped) {
                softap_handlers.softap_stopped(softap_handlers_arg);
            }
            next = CM_SOFTAP_STATE_INIT;
        }
        break;
    default:
        ESP_LOGE(TAG, "Invalid state");
        return;
    }
    if (next != g_conn_mgr.cm_softap.state) {
        g_conn_mgr.cm_softap.state = next;
    }
}

esp_err_t conn_mgr_softap_start(struct conn_mgr_softap_cfg *softap_cfg)
{
    struct conn_mgr_msg msg = { .event = CMI_CMD_SOFTAP_START };
    struct conn_mgr_softap_cfg *l_softap_cfg;

    if (softap_cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    l_softap_cfg = (struct conn_mgr_softap_cfg *) malloc(sizeof(struct conn_mgr_softap_cfg));
    if (l_softap_cfg == NULL) {
        ESP_LOGE(TAG, "failed to allocate memory");
        return ESP_ERR_NO_MEM;
    }

    memcpy(l_softap_cfg, softap_cfg, sizeof(struct conn_mgr_softap_cfg));
    msg.user_data = l_softap_cfg;
    g_conn_mgr.softap_enabled = true;

    return conn_mgr_send_msg_in(&msg);
}

esp_err_t conn_mgr_softap_stop()
{
    wifi_mode_t mode;
    struct conn_mgr_msg msg = { .event = CMI_CMD_SOFTAP_STOP };

    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_AP) {
        // Only AP mode enabled, hence stop WiFi
        esp_wifi_stop();
    }
    esp_wifi_set_mode(WIFI_MODE_STA);
    return conn_mgr_send_msg_in(&msg);
}
