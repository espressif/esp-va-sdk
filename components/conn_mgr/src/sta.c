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
#include <freertos/event_groups.h>

static const char *TAG = "[sta]";

static int do_sta_start()
{
    wifi_mode_t mode;
    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_cfg));

    if (conn_mgr_sta_get_config(&wifi_cfg.sta) != 0) {
        ESP_LOGE(TAG, "Couldn't read station configuration\n");
        return ESP_ERR_CONN_MGR_STA_CFG;
    }

    printf("%s: Connecting to ssid: %s\n", TAG, wifi_cfg.sta.ssid);

    if (g_conn_mgr.softap_enabled == true) {
        mode = WIFI_MODE_APSTA;
    } else {
        mode = WIFI_MODE_STA;
    }
    if (esp_wifi_set_mode(mode) != 0) {
        ESP_LOGE(TAG, "Couldn't set station mode\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }
    if (esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg) != 0) {
        ESP_LOGE(TAG, "Couldn't set Wi-Fi configuration\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }

    if (esp_wifi_start() != 0) {
        ESP_LOGE(TAG, "Couldn't start Wi-Fi\n");
        return ESP_ERR_CONN_MGR_WIFI;
    }

    return 0;
}

esp_err_t conn_mgr_sta_get_state(conn_mgr_sta_state_t *state)
{
    if (state == NULL) {
        ESP_LOGE(TAG, "invalid arguement to conn_mgr_sta_get_state()");
        return ESP_ERR_INVALID_ARG;
    } 
    switch (g_conn_mgr.cm_sta.state) {
    case CM_STA_STATE_INITED:
        state->wifi_state = STA_STATE_INITED;
        state->err_reason = 0;
        break;
    case CM_STA_STATE_STARTED:
        state->wifi_state = STA_STATE_CONNECTING;
        state->err_reason = 0;
        break;
    case CM_STA_STATE_CONNECTED:
        state->wifi_state = STA_STATE_CONNECTED;
        state->err_reason = 0;
        break;
    case CM_STA_STATE_DISCONNECTED:
        state->wifi_state = STA_STATE_DISCONNECTED;
        state->err_reason = g_conn_mgr.cm_sta.discon_reason.reason;
        break;
    default:
        state->wifi_state = STA_STATE_INVALID;
        state->err_reason = 0;
    }
    return ESP_OK;
}

void conn_mgr_drive_station(struct conn_mgr_msg *msg)
{
    int ret;
    enum cm_sta_state next = g_conn_mgr.cm_sta.state;

    // STA state machine not yet started
    if (g_conn_mgr.sta_enabled == false) {
        return;
    }

    switch (g_conn_mgr.cm_sta.state) {
    case CM_STA_STATE_PRE_INIT:
        if (msg->event == CMI_CMD_STA_START) {
            ESP_LOGI(TAG, "In station start");
            ret = do_sta_start();
            if (ret == 0) {
                next = CM_STA_STATE_INITED;
            } else {
                ESP_LOGE(TAG, "Error in station start\n");
                /* XXX Notify the application */
            }
        }
        break;
    case CM_STA_STATE_INITED:
        if (msg->event == CMI_EVENT_FROM_WIFI &&
                msg->data.event_from_wifi.event_id == SYSTEM_EVENT_STA_START) {
            esp_wifi_connect();
            next = CM_STA_STATE_STARTED;
        }
        break;
    case CM_STA_STATE_STARTED:
        if (msg->event == CMI_EVENT_FROM_WIFI) {
            if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_STA_CONNECTED) {
                conn_mgr_send_event_out(CM_EVT_STA_CONNECTED, &msg->data.event_from_wifi);
                next = CM_STA_STATE_CONNECTED;
            } else if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
                conn_mgr_send_event_out(CM_EVT_STA_DISCONNECTED, &msg->data.event_from_wifi);
                g_conn_mgr.cm_sta.discon_reason = msg->data.event_from_wifi.event_info.disconnected;
                next = CM_STA_STATE_DISCONNECTED;
            }
        }
        break;
    case CM_STA_STATE_CONNECTED:
        if (msg->event == CMI_EVENT_FROM_WIFI) {
            if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_STA_GOT_IP) {
                conn_mgr_send_event_out(CM_EVT_STA_GOT_IPV4, &msg->data.event_from_wifi.event_info.got_ip.ip_info);
            } else if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
                conn_mgr_send_event_out(CM_EVT_STA_DISCONNECTED, &msg->data.event_from_wifi);
                g_conn_mgr.cm_sta.discon_reason = msg->data.event_from_wifi.event_info.disconnected;
                next = CM_STA_STATE_DISCONNECTED;
            } else if (msg->data.event_from_wifi.event_id == SYSTEM_EVENT_GOT_IP6) {
                //      conn_mgr_send_event_out(CM_EVT_STA_GOT_IPV6, &msg->data.event_from_wifi.event_info.got_ip6.ip6_info);
                /* XXX Check if the interface is station, and only then send the event */
            }
        }
        break;
    case CM_STA_STATE_DISCONNECTED:
        if (msg->event == CMI_CMD_STA_START) {
            do_sta_start();
            esp_wifi_connect();
            next = CM_STA_STATE_STARTED;
        }
        break;
    default:
        ESP_LOGE(TAG, "Invalid state");
        return;
    }
    if (next != g_conn_mgr.cm_sta.state) {
        g_conn_mgr.cm_sta.state = next;
    }
}

int conn_mgr_sta_start()
{
    struct conn_mgr_msg msg = { .event = CMI_CMD_STA_START };
    g_conn_mgr.sta_enabled = true;

    return conn_mgr_send_msg_in(&msg);
}
