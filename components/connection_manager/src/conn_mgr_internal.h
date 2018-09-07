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

#ifndef _ESP_CONN_MGR_INTERNAL_H_
#define _ESP_CONN_MGR_INTERNAL_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <conn_mgr.h>
#include <nvs.h>

enum cm_sta_state {
    CM_STA_STATE_PRE_INIT,
    CM_STA_STATE_INITED,
    CM_STA_STATE_STARTED,
    CM_STA_STATE_CONNECTED,
    CM_STA_STATE_DISCONNECTED,
};

enum cm_softap_state {
    CM_SOFTAP_STATE_INIT,
    CM_SOFTAP_STATE_STARTED,
};

enum conn_mgr_internal_event {
    CMI_CMD_STA_START = 0,
    CMI_CMD_SOFTAP_START = 64,
    CMI_CMD_SOFTAP_STOP = 65,
    CMI_CMD_SOFTAP_CUSTOM = 66,
    CMI_EVENT_FROM_WIFI = 128,
};

struct conn_mgr_msg {
    enum conn_mgr_internal_event event;
    union conn_mgr_data {
        /* Data for the event CM_EVENT_FROM_WIFI */
        system_event_t  event_from_wifi;
    } data;
    void *user_data;
};
#define CONN_MGR_QUEUE_LEN 5
#define CONN_MGR_MAX_EVENT_HANDLERS 5
/* The connection manager's data structure */
struct conn_mgr {
    TaskHandle_t cm_task;
    xQueueHandle cm_queue;
    struct cm_sta {
        enum cm_sta_state state;
        system_event_sta_disconnected_t discon_reason;
    } cm_sta;
    struct cm_softap {
        enum cm_softap_state state;
    } cm_softap;
    bool sta_enabled;
    bool softap_enabled;

    /* The application's callback */
    conn_mgr_cb_t cm_cb;
    conn_mgr_cb_t event_handlers[CONN_MGR_MAX_EVENT_HANDLERS];
    uint8_t event_handler_len;
};
extern struct conn_mgr g_conn_mgr;

/* Common */
static inline esp_err_t conn_mgr_send_msg_in(struct conn_mgr_msg *msg)
{
    if (xQueueSend(g_conn_mgr.cm_queue, msg, 0) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static inline int conn_mgr_send_event_out(conn_mgr_event_t event, void *data)
{
    /* Should always be called from the Connection Manager's thread */
    for (int i = 0 ; i < g_conn_mgr.event_handler_len; i++) {
        g_conn_mgr.event_handlers[i](event, data);
    }
    if (g_conn_mgr.cm_cb) {
        return (*g_conn_mgr.cm_cb)(event, data);
    }
    return 0;
}


/* Station */
void conn_mgr_drive_station(struct conn_mgr_msg *msg);
/* SoftAP */
void conn_mgr_drive_softap(struct conn_mgr_msg *msg);

#define CONN_MGR_BIT0 BIT0

#endif /* ! _ESP_CONN_MGR_INTERNAL_H_ */
