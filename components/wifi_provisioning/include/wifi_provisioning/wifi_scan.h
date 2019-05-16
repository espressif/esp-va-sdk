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

#ifndef _PROV_WIFI_SCAN_H_
#define _PROV_WIFI_SCAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Type of context data passed to each get/set/apply handler
 *           function set in `wifi_prov_scan_handlers` structure.
 *
 * This is passed as an opaque pointer, thereby allowing it be defined
 * later in application code as per requirements.
 */
typedef struct wifi_prov_scan_ctx wifi_prov_scan_ctx_t;

typedef struct {
    char    ssid[33];
    uint8_t channel;
    int     rssi;
} wifi_prov_scan_result_t;

typedef struct wifi_prov_scan_handlers {
    esp_err_t (*scan_start)(bool blocking, bool passive,
                            uint8_t group_channels, uint32_t period_ms,
                            wifi_prov_scan_ctx_t **ctx);

    esp_err_t (*scan_status)(bool *scan_finished,
                             uint16_t *result_count,
                             wifi_prov_scan_ctx_t **ctx);

    esp_err_t (*scan_result)(uint16_t result_index,
                             wifi_prov_scan_result_t *result,
                             wifi_prov_scan_ctx_t **ctx);

    wifi_prov_scan_ctx_t *ctx;
} wifi_prov_scan_handlers_t;

/**
 * @brief   Handler for sending on demand WiFi scan results
 *
 * This is to be registered as the `prov-scan` endpoint handler
 * (protocomm `protocomm_req_handler_t`) using `protocomm_add_endpoint()`
 */
esp_err_t wifi_prov_scan_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                 uint8_t **outbuf, ssize_t *outlen, void *priv_data);

#ifdef __cplusplus
}
#endif

#endif
