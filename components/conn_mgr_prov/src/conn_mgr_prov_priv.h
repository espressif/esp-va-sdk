/* Unified Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_event_loop.h>

#include <protocomm.h>
#include <protocomm_security.h>
#include <wifi_provisioning/wifi_config.h>

/**
 * @brief   Get state of WiFi Station during provisioning
 *
 * @note    WiFi is initially configured as AP, when
 *          provisioning starts. After provisioning data
 *          is provided by user, the WiFi is reconfigured
 *          to run as both AP and Station.
 *
 * @param[out] state    Pointer to wifi_prov_sta_state_t variable to be filled
 *
 * @return
 *  - ESP_OK    : Successfully retrieved wifi state
 *  - ESP_FAIL  : Provisioning app not running
 */
esp_err_t wifi_prov_get_wifi_state(wifi_prov_sta_state_t* state);

/**
 * @brief   Get reason code in case of WiFi station
 *          disconnection during provisioning
 *
* @param[out] reason    Pointer to wifi_prov_sta_fail_reason_t variable to be filled
 *
 * @return
 *  - ESP_OK    : Successfully retrieved wifi disconnect reason
 *  - ESP_FAIL  : Provisioning app not running
 */
esp_err_t wifi_prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason);

/**
 * @brief   Runs WiFi as Station
 *
 * Configures the WiFi station mode to connect to the
 * SSID and password specified in config structure,
 * and starts WiFi to run as station
 *
 * @param[in] wifi_cfg  Pointer to WiFi cofiguration structure
 *
 * @return
 *  - ESP_OK      : WiFi configured and started successfully
 *  - ESP_FAIL    : Failed to set configuration
 */
esp_err_t wifi_prov_configure_sta(wifi_config_t *wifi_cfg);


/**
 * @brief   Notify conn_manager that provisioning is done
 *
 * Stops the provisioning. This is called by the get_status_handler()
 * when the status is connected. The handler is returned and then the
 * provisioning is stopped.
 *
 * @return
 *  - ESP_OK      : Provisioning will be stopped
 *  - ESP_FAIL    : Failed to stop provisioning
 */
esp_err_t wifi_prov_done();
