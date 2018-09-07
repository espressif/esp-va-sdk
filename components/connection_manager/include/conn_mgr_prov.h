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

#pragma once

#include <protocomm_security.h>
#include <wifi_provisioning/wifi_config.h>

typedef enum {
    SECURITY_0,
    SECURITY_1,
} prov_security_type_t;

/**
 * This structure is populated with the details received
 * from the provisioning application - Smart phone, python script, etc.
 */
struct conn_mgr_softap_prov_cfg {
    /** SoftAP network SSID */
    const char *ssid;
    /** SoftAP password if secured */
    const char *password;
    /** Provisioning Proof-of-possesion */
    protocomm_security_pop_t pop;
    /** Provisioning security type */
    prov_security_type_t sec_type;
};

/** Start Connection Manager SoftAP based Provisioning Scheme
 *
 * Start SoftAP mode of WiFi as per provided configuration. Followed up with start of HTTP server
 * and required handlers for interfacing with provisioning core protocol
 *
 * @param[in] cfg Pointer to SoftAP provisioning configuration
 * @return ESP_OK for success and appropriate error code for failure
 */
int conn_mgr_softap_prov_start(struct conn_mgr_softap_prov_cfg *cfg);

/** Stop Connection Manager SoftAP based Provisioning Scheme
 *
 * Stop provisioning, HTTP server and WiFi SoftAP mode
 *
 * @return ESP_OK for success and appropriate error code for failure
 */
int conn_mgr_softap_prov_stop();

/** Indicate network credential recieved event while provisioning.
 *
 *  Send a message to the connection manager thread upon recieving network
 *  credential.
 *
 *  @return ESP_OK for success and ESP_FAIL on failure
 */
int conn_mgr_softap_prov_done();

/** Common connection manager provisioning handler demultiplexing table */
extern wifi_prov_config_handlers_t conn_mgr_prov_handlers;
