// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_err.h>

typedef enum network_path {
    NETWORK_PATH_UPLOAD,
    NETWORK_PATH_DOWNLOAD,
    NETWORK_PATH_MAX,
} network_path_t;

/** Initialize Network Diagnostics
 *
 * This will create a task and print the network statistics at the specified intervals
 * Statistics includes RSSI of the connected network, upload speed, download speed, ping.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t network_diagnostics_init();

/** Network Speed: Add Data
 *
 * This API should be called to add the number of bytes to calculate the speed for the upload or download path.
 *
 * @param[in] path upload or download
 * @param[in] data number of bytes
 */
void network_diagnostics_speed_add(network_path_t path, int data);

/** Network Speed: Enable
 *
 * This API should be called to start or stop calculating the upload or download speed.
 *
 * @param[in] path upload or download
 * @param[in] status true to start, false to stop
 */
void network_diagnostics_speed_enable(network_path_t path, bool status);
