/* Unified Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_event_loop.h>
#include <protocomm.h>


typedef enum {
    CM_ENDPOINT_CONFIG,
    CM_ENDPOINT_ADD,
    CM_ENDPOINT_REMOVE,
    CM_PROV_START,
    CM_PROV_END,
} conn_mgr_cb_event_t;

typedef struct {
    esp_err_t (*prov_start) (protocomm_t *pc, void *config);
    esp_err_t (*prov_stop)  (protocomm_t *pc);
    void *    (*new_config) (void);
    void      (*delete_config) (void *config);
    esp_err_t (*set_config_service) (void *config, const char *service_name, const char *service_key);
    esp_err_t (*set_config_endpoint) (void *config, const char *endpoint_name, uint16_t uuid);
    int       wifi_mode;
    int       (*event_cb)(void *user_data, conn_mgr_cb_event_t event);
    void      *cb_user_data;
} conn_mgr_prov_t;

/**
 * @brief   Event handler for provisioning app
 *
 * This is called from the main event handler and controls the
 * provisioning application, depeding on WiFi events
 *
 * @param[in] ctx   Event context data
 * @param[in] event Event info
 *
 * @return
 *  - ESP_OK      : Event handled successfully
 *  - ESP_FAIL    : Failed to start server on event AP start
 */
esp_err_t conn_mgr_prov_event_handler(void *ctx, system_event_t *event);

/**
 * @brief   Checks if device is provisioned
 * *
 * @param[out] provisioned  True if provisioned, else false
 *
 * @return
 *  - ESP_OK      : Retrieved provision state successfully
 *  - ESP_FAIL    : Failed to retrieve provision state
 */
esp_err_t conn_mgr_prov_is_provisioned(bool *provisioned);

/**
 * @brief   Start provisioning
 *
 * @param[in] mode      Provisioning mode to use (BLE/SoftAP)
 * @param[in] security  Security mode
 * @param[in] pop       Pointer to proof of possesion (NULL if not present)
 *
 * @return
 *  - ESP_OK      : Provisioning started successfully
 *  - ESP_FAIL    : Failed to start
 */
esp_err_t conn_mgr_prov_start_provisioning(conn_mgr_prov_t mode, int security, const char *pop,
                                       const char *service_name, const char *service_key);

/**
 * @brief   Configure an extra endpoint
 * 
 * This API can be called by the application if it wants to configure an extra endpoint. The
 * API must be called when the application gets the callback with event CM_ENDPOINT_CONFIG.
 *
 * @param[in] ep_name  Name of the endpoint
 */
void conn_mgr_prov_endpoint_configure(const char *ep_name);

/**
 * @brief   Add a configured endpoint
 * 
 * This API can be called by the application if it wants to add i.e. enable the endpoint. The
 * API must be called when the application gets the callback with event CM_ENDPOINT_ADD.
 *
 * @param[in] ep_name   Name of the endpoint
 * @param[in] handler   Pointer to the endpoint handler
 * @param[in] user_ctx  User data
 */
void conn_mgr_prov_endpoint_add(const char *ep_name, int (*handler)(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data), void *user_ctx);

/**
 * @brief   Remove an endpoint
 * 
 * This API can be called if the application wants to selectively remove an endpoint while the
 * provisioning is still in progress.
 * All the endpoints are removed automatically when the provisioning stops.
 *
 * @param[in] ep_name  Name of the endpoint
 */
void conn_mgr_prov_endpoint_remove(const char *ep_name);

/**
 * @brief   Release the memory used during provisioning
 *
 * This API must be called by the application when it gets the callback
 * with event CM_PROV_END. It must be called incase of provisioning is done by BLE.
 * If the device is already provisioned, this API must be called if BLE is being used.
 */
void conn_mgr_prov_mem_release();