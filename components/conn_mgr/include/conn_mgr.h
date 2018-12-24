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

#ifndef _CONN_MGR_H_
#define _CONN_MGR_H_

#include <esp_wifi.h>
#include <stdbool.h>

enum esp_err_conn_mgr {
    ESP_ERR_CONN_MGR_BASE = 0X2100,  /*!< Starting number of error codes */
    ESP_ERR_CONN_MGR_STA_CFG,        /*!< Error reading station configuration */
    ESP_ERR_CONN_MGR_WIFI,           /*!< Error in Wi-Fi operation */
};

#define CONN_MGR_NVS_NAMESPACE   "cm"

enum wifi_sta_state {
    STA_STATE_INITED,
    STA_STATE_CONNECTING,
    STA_STATE_CONNECTED,
    STA_STATE_DISCONNECTED,
    STA_STATE_INVALID = 0x64
};

typedef enum conn_mgr_event {
    CM_EVT_STA_CONNECTED,
    CM_EVT_STA_GOT_IPV4,
    CM_EVT_STA_GOT_IPV6,
    CM_EVT_STA_DISCONNECTED,
    CM_EVT_SOFTAP_NW_CRED_RCVD,
    CM_EVT_SOFTAP_STARTED,
    CM_EVT_SOFTAP_STOPPED,
} conn_mgr_event_t;

typedef struct conn_mgr_sta_state {
    enum wifi_sta_state wifi_state;
    uint8_t err_reason;
} conn_mgr_sta_state_t;

/**
 * This structure consists of HTTP handlers which should point to your
 * HTTP server implementation
 */

struct conn_mgr_softap_handlers {
    /** Callback when softap is started*/
    int (*softap_started)(void *arg);
    /** Callback when softap is about to stop*/
    int (*softap_stopped)(void *arg);
};

/**
 * This structure acts as the configuration structure to be used
 * with SoftAP method of provisioning. The developer can configure the Wi-Fi access
 * point that the end user will see and point handlers to an existing HTTP server
 * implementation if the application already needs one using this structure.
 */
struct conn_mgr_softap_cfg {
    /** Contains information about Wi-Fi AP like SSID, password, and so on */
    wifi_ap_config_t wifi_cfg;
    /** Handlers for softAP callbacks */
    struct conn_mgr_softap_handlers handlers;
    /** Opaque data to be passed to handlers */
    void *arg;
};

/* Although the return value is 'int', currently the return value is ignored */
typedef int (*conn_mgr_cb_t)(conn_mgr_event_t id, void *data);

/** Initialize connection manager
 *
 * Initializes connection manager which is responsible for managing various WiFi state
 * transitions including both Station and SoftAP mode
 *
 * @param[in] cb Callback for various events from connection manager
 * @return ESP_OK for success, appropriate error code on failure
 */
esp_err_t conn_mgr_init(conn_mgr_cb_t cb);

/** Get WiFi STA state
 *
 * Get WiFi STA state, whether device is provisioned or not, as per flash config settings
 *
 * @return true if configured, else false
 */
bool conn_mgr_sta_is_configured();

/** Connection Manager Set Config
 *
 * Set connection manager sta configuration in NVS storage on flash
 *
 * @param[in] sta Handle to WiFi STA configuration
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_sta_set_config(const wifi_sta_config_t *sta);

/** Connection Manager Get Config
 *
 * Get sta configuration from NVS storage on flash
 *
 * @param[out] sta Handle to WiFi STA configuration
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_sta_get_config(wifi_sta_config_t *sta);

/** Connection Manager Start STA
 *
 * Starts WiFi station mode with required state machine for internal state transitions
 *
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_sta_start();

/** Connection Manager Start SoftAP mode
 *
 * Starts WiFi SoftAP mode along with HTTP server related callbacks if registered
 *
 * @param[in] softap_cfg Handle to SoftAP configuration structure
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_softap_start(struct conn_mgr_softap_cfg *softap_cfg);

/** Connection Manager Stop SoftAP
 *
 * Stop WiFi SoftAP mode
 *
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_softap_stop();

/** Register event handler with connection manager
 *
 * @param[in] func Callback to be registered
 * @return ESP_OK for success, appropriate error code for failure
 */
esp_err_t conn_mgr_add_event_handler(void *func);

/** Get WiFi STA state
 *
 * @return WiFi State state
 */
esp_err_t conn_mgr_sta_get_state(conn_mgr_sta_state_t *state);

#endif /* ! _CONN_MGR_H_ */
