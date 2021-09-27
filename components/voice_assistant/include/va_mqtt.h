// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _VA_MQTT_H_
#define _VA_MQTT_H_

#include <stdint.h>
#include <esp_err.h>
#include <mqtt_client.h>

#define MAX_MQTT_SUBSCRIPTIONS      5
#define MAX_EVENT_CALLBACKS         3

/** MQTT Subscribe callback prototype
 *
 * @param[in] topic Topic on which the message was received
 * @param[in] payload Data received in the message
 * @param[in] payload_len Length of the data
 * @param[in] priv_data The private data passed during subscription
 */
typedef void (*va_mqtt_subscribe_cb_t)(const char *topic, void *payload, size_t payload_len, size_t total_payload_len, void *priv_data);

/** MQTT Event callback prototype
 *
 * The callback is called for the following events:
 * - Connect
 * - Disconnect
 */
typedef void (*va_mqtt_event_cb_t)(esp_mqtt_event_id_t event);

/** Publish MQTT Message
 *
 * @param[in] topic The MQTT topic on which the message should be published.
 * @param[in] data NULL terminated data string to be published.
 * @param[in] data_len length of data to be published.
 *
 * @return ESP_OK on success.
 * @return error in case of any error.
 */
esp_err_t va_mqtt_publish(const char *topic, const char *data, size_t data_len);

/** Subscribe to MQTT topic
 *
 * @param[in] topic The topic to be subscribed to.
 * @param[in] cb The callback to be invoked when a message is received on the given topic.
 * @param[in] priv_data Optional private data to be passed to the callback
 *
 * @return ESP_OK on success.
 * @return error in case of any error.
 */
esp_err_t va_mqtt_subscribe(const char *topic, va_mqtt_subscribe_cb_t cb, void *priv_data);

/** Unsubscribe from MQTT topic
 *
 * @param[in] topic Topic from which to unsubscribe.
 *
 * @return ESP_OK on success.
 * @return error in case of any error.
 */
esp_err_t va_mqtt_unsubscribe(const char *topic);

/** Get Client
 *
 * It returns the MQTT handle that is used by the voice assistant.
 *
 * @return esp_mqtt_client_handle_t
 */
esp_mqtt_client_handle_t va_mqtt_get_client(void);

/** Add event callback
 *
 * Add an event callback of the type va_mqtt_event_cb_t.
 *
 * @return ESP_OK on success.
 * @return error in case of any error.
 */
esp_err_t va_mqtt_add_event_callback(va_mqtt_event_cb_t event_cb);

/** Remove event callback
 *
 * Remove an event callback of the type va_mqtt_event_cb_t which was previously added using va_mqtt_add_event_callback().
 *
 * @return ESP_OK on success.
 * @return error in case of any error.
 */
esp_err_t va_mqtt_remove_event_callback(va_mqtt_event_cb_t event_cb);

#endif /* _VA_MQTT_H_ */
