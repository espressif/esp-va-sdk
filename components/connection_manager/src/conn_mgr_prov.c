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

#include <conn_mgr.h>
#include <conn_mgr_prov.h>
#include <esp_log.h>
#include "conn_mgr_internal.h"
#include <string.h>

#include <protocomm.h>
#include <protocomm_httpd.h>
#include <protocomm_security0.h>
#include <protocomm_security1.h>

static const char *TAG = "conn_mgr_prov";

static protocomm_t *pc_prov;

typedef struct {
    prov_security_type_t sec;
    protocomm_security_pop_t pop;
} prov_security_t;
#define PROTOCOMM_HTTPD_PORT    80
static int prov_softap_started(void *arg)
{
    prov_security_t *p_sec = (prov_security_t *) arg;
    if (!p_sec) {
        ESP_LOGE(TAG, "Prov start request withou security config");
        return ESP_FAIL;
    }

    pc_prov = protocomm_new();
    if (!pc_prov) {
        ESP_LOGE(TAG, "Error creating protocomm");
        return ESP_FAIL;
    }

    /* Config for protocomm_httpd_start() */
    protocomm_httpd_config_t pc_config = {
        .port = PROTOCOMM_HTTPD_PORT
    };

    if (protocomm_httpd_start(pc_prov, &pc_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start httpd protocomm");
        return ESP_FAIL;
    }

    if (p_sec->sec == SECURITY_0) {
        protocomm_set_security(pc_prov, "prov-session", &security0, NULL);
    } else if (p_sec->sec == SECURITY_1) {
        protocomm_set_security(pc_prov, "prov-session", &security1, &p_sec->pop);
    } else {
        ESP_LOGE(TAG, "Invalid security");
    }

    return protocomm_add_endpoint(pc_prov, "prov-config", wifi_prov_config_data_handler, (void *) &conn_mgr_prov_handlers);
}

static int prov_softap_stopped(void *arg)
{
    prov_security_t *p_sec = (prov_security_t *) arg;
    if (!p_sec) {
        ESP_LOGE(TAG, "Prov start request withou security config");
        return ESP_FAIL;
    }

    protocomm_remove_endpoint(pc_prov, "prov-config");
    protocomm_unset_security(pc_prov, "prov-session");
    protocomm_httpd_stop(pc_prov);
    protocomm_delete(pc_prov);

    free(p_sec);
    pc_prov = NULL;
    return ESP_OK;
}

int conn_mgr_softap_prov_start(struct conn_mgr_softap_prov_cfg *p_cfg)
{
    esp_err_t ret;

    if (p_cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct conn_mgr_softap_cfg cfg = {
        .wifi_cfg = {
            .max_connection = 5,
            .authmode = WIFI_AUTH_OPEN
        },
        .handlers = {
            .softap_started = prov_softap_started,
            .softap_stopped = prov_softap_stopped,
        }
    };

    snprintf((char *)cfg.wifi_cfg.ssid, sizeof(cfg.wifi_cfg.ssid), "%s", p_cfg->ssid);
    if (p_cfg->password) {
        snprintf((char *)cfg.wifi_cfg.password, sizeof(cfg.wifi_cfg.password), "%s", p_cfg->password);
        cfg.wifi_cfg.authmode = WIFI_AUTH_WPA2_PSK;
    }

    prov_security_t *p_sec = (prov_security_t *) calloc(1, sizeof(prov_security_t));
    if (p_cfg->pop.data && p_cfg->pop.len > 0) {
        p_sec->pop.data = (uint8_t *) strndup((char *) p_cfg->pop.data, p_cfg->pop.len);
        p_sec->pop.len = p_cfg->pop.len;
    }
    p_sec->sec = p_cfg->sec_type;

    cfg.arg = (void *) p_sec;

    ret = conn_mgr_softap_start(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "softap init failed: %d", ret);
        return ret;
    }

    return ESP_OK;
}

int conn_mgr_softap_prov_stop()
{
    return conn_mgr_softap_stop();
}

int conn_mgr_softap_prov_done()
{
    struct conn_mgr_msg msg = { .event = CMI_CMD_SOFTAP_CUSTOM };
    g_conn_mgr.softap_enabled = true;
    return conn_mgr_send_msg_in(&msg);
}
