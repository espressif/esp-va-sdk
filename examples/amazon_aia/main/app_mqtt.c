// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_log.h>
#include <va_mqtt.h>
#include "app_mqtt.h"

static const char *TAG = "[app_mqtt]";

static bool is_connected = false;

static void app_mqtt_subscribe_callback(const char *topic, void *payload, size_t payload_len, size_t total_payload_len, void *priv_data)
{
    printf("%s: Subscription callback for topic: %s. Data received: %.*s\n", TAG, topic, payload_len, (char *)payload);
}

int app_mqtt_subscribe_handler()
{
    if (is_connected == false) {
        return -1;
    }

    char *topic = "test_topic";
    printf("%s: Subscribing to topic: %s\n", TAG, topic);
    esp_err_t err = va_mqtt_subscribe(topic, app_mqtt_subscribe_callback, NULL);
    if (err != ESP_OK) {
        return -1;
    }
    return 0;
}

int app_mqtt_publish_handler()
{
    if (is_connected == false) {
        return -1;
    }

    char *topic = "test_topic";
    char *data = "test_data";
    uint32_t data_len = strlen(data);
    printf("%s: Publishing to topic: %s. Data: %.*s\n", TAG, topic, data_len, data);

    esp_err_t err = va_mqtt_publish(topic, data, data_len);
    if (err != ESP_OK) {
        return -1;
    }
    return 0;
}

static void mqtt_event_cb(esp_mqtt_event_id_t event)
{
    static bool subscribe_done = false;
    if (event == MQTT_EVENT_CONNECTED) {
        is_connected = true;
        if (subscribe_done == false) {
            /* Note: Subscription needs to be done only once */
            if (app_mqtt_subscribe_handler() == 0) {
                subscribe_done = true;
            } else {
                ESP_LOGE(TAG, "Error subscribing");
            }
        }
    } else if (event == MQTT_EVENT_DISCONNECTED) {
        is_connected = false;
    }
}

esp_err_t app_mqtt_init()
{
    return va_mqtt_add_event_callback(mqtt_event_cb);
}
