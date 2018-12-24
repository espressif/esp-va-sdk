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
#include <nvs_flash.h>
#include <esp_log.h>

static const char *TAG = "conn_mgr";

/* The NVS station configuration is written to NVS as follows:
 * namespace: CONN_MGR_NVS_NAMESPACE
 * variables:
 *   - ssid (str): for storing the network's SSID
 *   - pass (str): for storing the network's passphrase
 *   - config (i8): set to 1 if the configuration is present, else 0 or not available
 *
 */
#define CONN_MGR_NVS_KEY_SSID     "ssid"
#define CONN_MGR_NVS_KEY_PASS     "pass"
#define CONN_MGR_NVS_KEY_CONFIG   "config"
bool __conn_mgr_sta_is_configured(nvs_handle nvs)
{
    int8_t config = 0;
    esp_err_t ret;
    ret = nvs_get_i8(nvs, CONN_MGR_NVS_KEY_CONFIG, &config);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "CONFIG variable not found in NVS");
        return false;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read variable CONFIG");
        return false;
    }
    return config ? true : false;
}

bool conn_mgr_sta_is_configured()
{
    nvs_handle nvs;
    esp_err_t ret = nvs_open(CONN_MGR_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* The flash was probably erased, so there's no namespace in there */
        return false;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return false;
    }
    bool config = __conn_mgr_sta_is_configured(nvs);
    nvs_close(nvs);
    return config;
}

esp_err_t conn_mgr_sta_set_config(const wifi_sta_config_t *sta)
{
    nvs_handle nvs;
    esp_err_t err = ESP_FAIL;

    if (nvs_open(CONN_MGR_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return err;
    }
    if (nvs_set_str(nvs, CONN_MGR_NVS_KEY_SSID, (char *)sta->ssid) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write variable SSID");
        goto err_close;
    }
    if (nvs_set_str(nvs, CONN_MGR_NVS_KEY_PASS, (char *)sta->password) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write variable PASS");
        goto err_close;
    }
    if (nvs_set_i8(nvs, CONN_MGR_NVS_KEY_CONFIG, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write variable CONFIG");
        goto err_close;
    }
    if (nvs_commit(nvs) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to do nvs commit");
        goto err_close;
    }
    err = ESP_OK;
err_close:
    nvs_close(nvs);
    return err;
}

int conn_mgr_sta_get_config(wifi_sta_config_t *sta)
{
    nvs_handle nvs;
    esp_err_t ret = ESP_FAIL;

    if (nvs_open(CONN_MGR_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS %d", ret);
        return ret;
    }

    bool config = __conn_mgr_sta_is_configured(nvs);
    if (!config) {
        ESP_LOGI(TAG, "CONFIG variable not set in NVS");
        goto err_close;
    }
    size_t limit = sizeof(sta->ssid);
    if (nvs_get_str(nvs, CONN_MGR_NVS_KEY_SSID,
                    (char *)sta->ssid, &limit) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write variable SSID");
        goto err_close;
    }
    limit = sizeof(sta->password);
    if (nvs_get_str(nvs, CONN_MGR_NVS_KEY_PASS,
                    (char *)sta->password, &limit) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write variable PASS");
        goto err_close;
    }
    /* We ignore the length for SSID and passphrase, since the
     * underlying subsystem doesn't accept it in any manner.
     */
    ret = ESP_OK;
err_close:
    nvs_close(nvs);
    return ret;
}

