// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <conn_mgr.h>
#include <conn_mgr_prov.h>
#include <tcpip_adapter.h>

#include <wifi_provisioning/wifi_config.h>

static const char* TAG = "wifi_hdlr";

static esp_err_t get_status_handler(wifi_prov_config_get_data_t *resp_data)
{
    conn_mgr_sta_state_t state;

    memset(resp_data, 0, sizeof(wifi_prov_config_get_data_t));
    
    esp_err_t err = conn_mgr_sta_get_state(&state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Couldn't fetch sta state");
        return ESP_FAIL;
    }

    if (state.wifi_state == STA_STATE_CONNECTING) {
        printf("Connecting state\n");
        resp_data->wifi_state = WIFI_PROV_STA_CONNECTING;
    } else if (state.wifi_state == STA_STATE_CONNECTED) {
        printf("Connected state\n");
        resp_data->wifi_state = WIFI_PROV_STA_CONNECTED;

        tcpip_adapter_ip_info_t ip_info;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
        char *ip_addr = ip4addr_ntoa(&ip_info.ip);
        strcpy(resp_data->conn_info.ip_addr, ip_addr);

        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(resp_data->conn_info.bssid, (char *)ap_info.bssid, sizeof(ap_info.bssid));
        memcpy(resp_data->conn_info.ssid,  (char *)ap_info.ssid,  sizeof(ap_info.ssid));
        resp_data->conn_info.channel   = ap_info.primary;
        resp_data->conn_info.auth_mode = ap_info.authmode;

    } else if (state.wifi_state == STA_STATE_DISCONNECTED) {
        printf("Connect failed state\n");
        resp_data->wifi_state = WIFI_PROV_STA_DISCONNECTED;

        if (state.err_reason == 2) {
            resp_data->fail_reason = WIFI_PROV_STA_AUTH_ERROR;
        } else if (state.err_reason == 201) {
            resp_data->fail_reason = WIFI_PROV_STA_AP_NOT_FOUND;
        }
    }
    return ESP_OK;
}

static esp_err_t set_config_handler(const wifi_prov_config_set_data_t *req_data)
{
    wifi_sta_config_t sta;

    memset(&sta, 0, sizeof(sta));
    memcpy((char *)sta.ssid, req_data->ssid, strlen(req_data->ssid));
    memcpy((char *)sta.password, req_data->password, strlen(req_data->password));

    ESP_LOGI(TAG, "Creds received %s %s", sta.ssid, sta.password);
    int ret = conn_mgr_sta_set_config(&sta);
    if (ret < 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t apply_config_handler()
{
    int ret;

    ret = conn_mgr_softap_prov_done();
    if (ret < 0) {
        return ESP_FAIL;
    }

    ret = conn_mgr_sta_start();
    if (ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

wifi_prov_config_handlers_t conn_mgr_prov_handlers = {
    .get_status_handler = get_status_handler,
    .set_config_handler = set_config_handler,
    .apply_config_handler = apply_config_handler,
};

