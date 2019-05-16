/* Unified Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>

#include <protocomm.h>
#include <protocomm_httpd.h>

#include "conn_mgr_prov.h"
#include "conn_mgr_prov_mode_softap.h"

static const char *TAG = "conn_mgr_prov_mode_softap";

extern conn_mgr_prov_t conn_mgr_prov_mode_softap;

static esp_err_t start_wifi_ap(const char *ssid, const char *pass)
{
    /* Initialise WiFi with default configuration */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WiFi : %d", err);
        return err;
    }

    /* Build WiFi configuration for AP mode */
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 5,
        },
    };

    strncpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ssid);

    if (strlen(pass) == 0) {
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strncpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    /* Start WiFi in AP (+ STA for scanning) mode with configuration built above */
    err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode : %d", err);
        return err;
    }
    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config : %d", err);
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi : %d", err);
        return err;
    }

    return ESP_OK;
}

static esp_err_t prov_start(protocomm_t *pc, void *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    conn_mgr_prov_mode_softap_config_t *softap_config = (conn_mgr_prov_mode_softap_config_t *) config;

    protocomm_httpd_config_t *httpd_config = &softap_config->httpd_config;

    /* Start protocomm server on top of HTTP */
    esp_err_t err = protocomm_httpd_start(pc, httpd_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start protocomm HTTP server");
        return ESP_FAIL;
    }

    /* Start WiFi softAP with specified ssid and password */
    err = start_wifi_ap(softap_config->ssid, softap_config->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi AP");
        protocomm_httpd_stop(pc);
        return err;
    }

    return ESP_OK;
}

static void *new_config(void)
{
    conn_mgr_prov_mode_softap_config_t *softap_config = calloc(1, sizeof(conn_mgr_prov_mode_softap_config_t));
    if (softap_config == NULL) {
        return NULL;
    }
    protocomm_httpd_config_t default_config = PROTOCOMM_HTTPD_DEFAULT_CONFIG();
    softap_config->httpd_config = default_config;
    return softap_config;
}

static void delete_config(void *config)
{
    conn_mgr_prov_mode_softap_config_t *softap_config = (conn_mgr_prov_mode_softap_config_t *) config;
    free(softap_config);
}

static esp_err_t set_config_service(void *config, const char *service_name, const char *service_key)
{
    conn_mgr_prov_mode_softap_config_t *softap_config = (conn_mgr_prov_mode_softap_config_t *) config;
    strlcpy(softap_config->ssid,     service_name, sizeof(softap_config->ssid));
    strlcpy(softap_config->password, service_key,  sizeof(softap_config->password));
    return ESP_OK;
}

static esp_err_t set_config_endpoint(void *config, const char *endpoint_name, uint16_t uuid)
{
    return ESP_OK;
}

conn_mgr_prov_t conn_mgr_prov_mode_softap = {
    .prov_start          = prov_start,
    .prov_stop           = protocomm_httpd_stop,
    .new_config          = new_config,
    .delete_config       = delete_config,
    .set_config_service  = set_config_service,
    .set_config_endpoint = set_config_endpoint,
    .wifi_mode           = WIFI_MODE_APSTA,
    .event_cb            = NULL,
    .cb_user_data        = NULL,
};
