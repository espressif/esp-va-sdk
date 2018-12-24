// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_HTTPD_OTA_
#define _ESP_HTTPD_OTA_

#include <http_server.h>

typedef enum {
    /* Before the OTA update has started */
    ESP_HTTPD_OTA_PRE_UPDATE,
    /* OTA update is complete and the device is about to restart */
    ESP_HTTPD_OTA_POST_UPDATE,
} esp_httpd_ota_cb_event_t;

/** Start the server for OTA updates
 * This API should be called by the application if it wants to enable OTA updates.
 * This will start a http server.
 * A POST request with the binary file of application can be sent at the device's
 * ip address at "/update" for OTA update.
 *
 * \param[in] event_cb                  Pointer to the callback function.
 * \param[in] server_handle             Handle of the server if the application has already created a server and does not want to create a new one.
 *                                      Pass NULL for creating a new server.
 */
void esp_httpd_ota_update_init(void (*event_cb)(esp_httpd_ota_cb_event_t event), httpd_handle_t server_handle);

#endif /* _ESP_HTTPD_OTA_ */
