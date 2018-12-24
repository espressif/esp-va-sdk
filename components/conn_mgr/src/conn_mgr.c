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

#include <string.h>
#include <conn_mgr.h>
#include "conn_mgr_internal.h"
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <freertos/event_groups.h>

static const char *TAG = "conn_mgr";
struct conn_mgr g_conn_mgr;

static esp_err_t conn_mgr_event_handler(void *ctx, system_event_t *event)
{
    struct conn_mgr_msg msg = {
        .event = CMI_EVENT_FROM_WIFI,
        .data.event_from_wifi = *event,
    };

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(TAG, "got ip:%s",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED, reason_code %d", event->event_info.disconnected.reason);
            break;
        default:
            break;
    }

    conn_mgr_send_msg_in(&msg);

    return ESP_OK;
}

/* This thread is basically an execution context to run the various
 * state machines in. Its only job is to provide that context, and
 * deliver all the messages it receives to all the state machines.
 */
void conn_mgr_thread(void *arg)
{
    while (1) {
        struct conn_mgr_msg msg;
        if (xQueueReceive(g_conn_mgr.cm_queue, &msg, portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Queue receive failed");
            vTaskDelete(NULL);
        }
        ESP_LOGI(TAG, "Received event: %d", msg.event);
        /* Send the events to the station state machine */
        conn_mgr_drive_station(&msg);
        conn_mgr_drive_softap(&msg);
    }
}

esp_err_t conn_mgr_init(conn_mgr_cb_t cb)
{
    memset(&g_conn_mgr, 0, sizeof(g_conn_mgr));
    g_conn_mgr.cm_cb = cb;
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed");
        return ret;
    }
    tcpip_adapter_init();
    ret = esp_event_loop_init(conn_mgr_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Event Loop init failed");
        return ret;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed");
        return ret;
    }
    g_conn_mgr.cm_queue = xQueueCreate(CONN_MGR_QUEUE_LEN, sizeof(struct conn_mgr_msg));
    if (g_conn_mgr.cm_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create cm_queue");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(conn_mgr_thread, "conn_mgr", 4096,
                    NULL, 5, &g_conn_mgr.cm_task) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create cm_task");
        ret = -1;
        goto queue_close;
    }
    ESP_LOGI(TAG, "Initialization success");
    return ESP_OK;

queue_close:
    vQueueDelete(g_conn_mgr.cm_queue);
    g_conn_mgr.cm_queue = NULL;
    return ret;
}

esp_err_t conn_mgr_add_event_handler(void *ev_handler)
{
    ESP_LOGI("CONN_MGR", "Adding event handler");
    if (g_conn_mgr.event_handler_len + 1 < CONN_MGR_MAX_EVENT_HANDLERS) {
        g_conn_mgr.event_handlers[g_conn_mgr.event_handler_len++] = ev_handler;
        ESP_LOGI("CONN_MGR", "Added event handler");
        return ESP_OK;
    }
    ESP_LOGE("CONN_MGR", "Adding event handler failed");
    return ESP_FAIL;

}
