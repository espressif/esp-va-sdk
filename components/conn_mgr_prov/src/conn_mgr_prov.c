/* Unified Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_bt.h>

#include <protocomm.h>
#include <protocomm_security0.h>
#include <protocomm_security1.h>
#include <protocomm_ble.h>
#include <protocomm_httpd.h>

#include <wifi_provisioning/wifi_config.h>
#include <wifi_provisioning/wifi_scan.h>

#include "conn_mgr_prov.h"
#include "conn_mgr_prov_priv.h"

#define MAX_SCAN_RESULTS 10

static const char *TAG = "[conn_mgr_prov]";

static void conn_mgr_prov_extra_mem_release();

/* Handlers for wifi_config provisioning endpoint */
extern wifi_prov_config_handlers_t wifi_prov_handlers;

/* Handlers for wifi_scan provisioning endpoint */
extern wifi_prov_scan_handlers_t wifi_scan_handlers;

/**
 * @brief   Data relevant to provisioning application
 */
struct wifi_prov_data {
    conn_mgr_prov_t  prov;         /*!< Provisioning handle */
    void        *prov_mode_config; /*!< Wifi Provisioning Mode Config */
    int          security;         /*!< Type of security to use with protocomm */
    protocomm_t *pc;               /*!< Protocomm handle */
    protocomm_security_pop_t pop;  /*!< Pointer to proof of possesion */
    esp_timer_handle_t timer;      /*!< Handle to timer */

    /* State of WiFi Station */
    wifi_prov_sta_state_t wifi_state;

    /* Code for WiFi station disconnection (if disconnected) */
    wifi_prov_sta_fail_reason_t wifi_disconnect_reason;

    /* WiFi scan parameters and state variables */
    bool scanning;
    uint8_t channels_per_group;
    uint16_t curr_channel;
    uint16_t ap_list_len[14];
    wifi_ap_record_t *ap_list[14];
    wifi_ap_record_t *ap_list_sorted[MAX_SCAN_RESULTS];
    wifi_scan_config_t scan_cfg;
};

/* Pointer to provisioning application data */
static struct wifi_prov_data *g_prov = NULL;
static SemaphoreHandle_t g_prov_lock = NULL;
static int endpoint_uuid_used = 0;

static esp_err_t conn_mgr_prov_start_service(const char *service_name, const char *service_key)
{
    /* Create new protocomm instance */
    g_prov->pc = protocomm_new();
    if (g_prov->pc == NULL) {
        ESP_LOGE(TAG, "Failed to create new protocomm instance");
        return ESP_FAIL;
    }

    g_prov->prov_mode_config = g_prov->prov.new_config();
    if (g_prov->prov_mode_config == NULL) {
        ESP_LOGE(TAG, "Failed to allocate provisioning mode config");
        return ESP_ERR_NO_MEM;
    }

    g_prov->prov.set_config_service(g_prov->prov_mode_config, service_name, service_key);

    g_prov->prov.set_config_endpoint(g_prov->prov_mode_config, "prov-scan",    0xFF50);
    g_prov->prov.set_config_endpoint(g_prov->prov_mode_config, "prov-session", 0xFF51);
    g_prov->prov.set_config_endpoint(g_prov->prov_mode_config, "prov-config",  0xFF52);
    g_prov->prov.set_config_endpoint(g_prov->prov_mode_config, "proto-ver",    0xFF53);
    endpoint_uuid_used = 0xFF53;

    if (g_prov->prov.event_cb) {
        g_prov->prov.event_cb(g_prov->prov.cb_user_data, CM_ENDPOINT_CONFIG);
        g_prov->prov.event_cb(g_prov->prov.cb_user_data, CM_PROV_START);
    }

    /* For releasing BT memory, as we need only BLE */
    conn_mgr_prov_extra_mem_release();

    if (g_prov->prov.prov_start(g_prov->pc, g_prov->prov_mode_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start provisioning");
        return ESP_FAIL;
    }

    /* Set protocomm version verification endpoint for protocol */
    protocomm_set_version(g_prov->pc, "proto-ver", "V0.2");

    /* Set protocomm security type for endpoint */
    if (g_prov->security == 0) {
        protocomm_set_security(g_prov->pc, "prov-session", &protocomm_security0, NULL);
    } else if (g_prov->security == 1) {
        protocomm_set_security(g_prov->pc, "prov-session", &protocomm_security1, &g_prov->pop);
    }

    /* Add endpoint for provisioning to set wifi station config */
    if (protocomm_add_endpoint(g_prov->pc, "prov-config",
                               wifi_prov_config_data_handler,
                               (void *) &wifi_prov_handlers) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set provisioning endpoint");
        g_prov->prov.prov_stop(g_prov->pc);
        return ESP_FAIL;
    }

    /* Add endpoint for performing and sending wifi scan results */
    if (protocomm_add_endpoint(g_prov->pc, "prov-scan",
                               wifi_prov_scan_handler,
                               (void *) &wifi_scan_handlers) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi scan endpoint");
        g_prov->prov.prov_stop(g_prov->pc);
        return ESP_FAIL;
    }

    if (g_prov->prov.event_cb) {
        g_prov->prov.event_cb(g_prov->prov.cb_user_data, CM_ENDPOINT_ADD);
    }

    printf("%s: Provisioning started with: \n\tservice name: %s \n\tservice key: %s\n", TAG, service_name, service_key);
    return ESP_OK;
}

void conn_mgr_prov_endpoint_configure(const char *ep_name)
{
    endpoint_uuid_used++;
    g_prov->prov.set_config_endpoint(g_prov->prov_mode_config, ep_name, endpoint_uuid_used);
}

void conn_mgr_prov_endpoint_add(const char *ep_name, int (*handler)(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data), void *user_ctx)
{
    if (protocomm_add_endpoint(g_prov->pc, ep_name,
                               handler,
                               user_ctx) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set provisioning endpoint");
        g_prov->prov.prov_stop(g_prov->pc);
    }
}

void conn_mgr_prov_endpoint_remove(const char *ep_name)
{
    protocomm_remove_endpoint(g_prov->pc, ep_name);
}

void conn_mgr_prov_mem_release()
{
#if CONFIG_BT_ENABLED
    esp_err_t err;
/* This is used in esp-alexa */
#ifndef ALEXA_BT
    /* Release memory used by BT stack */
    err = esp_bt_mem_release(ESP_BT_MODE_BTDM);
#else
    err = esp_bt_mem_release(ESP_BT_MODE_BLE);
#endif /* ALEXA_BT */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bt_mem_release failed %d", err);
        return;
    }
    ESP_LOGI(TAG, "BT stack memory released");
#endif

}

/* Release BT memory, as we need only BLE */
static void conn_mgr_prov_extra_mem_release()
{
#if (CONFIG_BT_ENABLED && CONFIG_BTDM_CONTROLLER_MODE_BLE_ONLY)
    /* Release BT memory, as we need only BLE */
    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bt_controller_mem_release failed %d", err);
        return;
    }
    ESP_LOGI(TAG, "BT controller memory released");
#endif
}

static void conn_mgr_prov_stop_service(void)
{
    /* Remove provisioning endpoint */
    protocomm_remove_endpoint(g_prov->pc, "prov-config");

    /* All the extra application added endpoints are also removed automatically when prov_stop is called. Hence, no need to give a callback with CM_ENDPOINT_REMOVE. */

    /* Unset provisioning security */
    protocomm_unset_security(g_prov->pc, "prov-session");
    /* Unset provisioning version endpoint */
    protocomm_unset_version(g_prov->pc, "proto-ver");
    /* Stop protocomm service */
    g_prov->prov.prov_stop(g_prov->pc);
    /* Free config data */
    g_prov->prov.delete_config(g_prov->prov_mode_config);
    /* Delete protocomm instance */
    protocomm_delete(g_prov->pc);

    if (g_prov->prov.event_cb) {
        g_prov->prov.event_cb(g_prov->prov.cb_user_data, CM_PROV_END);
    }
}

/* Task spawned by timer callback or by wifi_prov_done() */
static void stop_prov_task(void * arg)
{
    /* This delay is so that the phone app is noified first and then the provisioning is stopped. Generally 100ms is enough. */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Stopping provisioning");
    conn_mgr_prov_stop_service();
    esp_wifi_set_mode(WIFI_MODE_STA);

    /* Timer not needed anymore */
    esp_timer_handle_t timer = g_prov->timer;
    esp_timer_delete(timer);
    g_prov->timer = NULL;

    /* Delete all scan results */
    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);
    for (uint16_t channel = 0; channel < 14; channel++) {
        free(g_prov->ap_list[channel]);
    }
    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    /* Free provisioning process data */
    free((void *)g_prov->pop.data);
    free(g_prov);
    g_prov = NULL;
    printf("%s: Provisioning stopped\n", TAG);

    vTaskDelete(NULL);
}

/* Callback to be invoked by timer */
static void _stop_prov_cb(void * arg)
{
    xTaskCreate(&stop_prov_task, "stop_prov", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

/* If the provisioning was done before the timeout occured */
esp_err_t wifi_prov_done()
{
    esp_timer_stop(g_prov->timer);
    xTaskCreate(&stop_prov_task, "stop_prov", 2048, NULL, tskIDLE_PRIORITY, NULL);
    return ESP_OK;
}

static esp_err_t update_wifi_scan_results(void)
{
    ESP_LOGI(TAG, "Scan finished");

    esp_err_t ret = ESP_FAIL;
    uint16_t count = 0;
    uint16_t curr_channel = g_prov->curr_channel;

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);
    if (g_prov->ap_list[curr_channel]) {
        free(g_prov->ap_list[curr_channel]);
        g_prov->ap_list[curr_channel] = NULL;
        g_prov->ap_list_len[curr_channel] = 0;
    }
    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    if (esp_wifi_scan_get_ap_num(&count) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get count of scanned APs");
        goto exit;
    }

    if (!count) {
        ESP_LOGW(TAG, "Scan result empty");
        ret = ESP_OK;
        goto exit;
    }

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);
    g_prov->ap_list[curr_channel] = (wifi_ap_record_t *) calloc(count, sizeof(wifi_ap_record_t));
    if (!g_prov->ap_list[curr_channel]) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP list");
        goto exit;
    }
    if (esp_wifi_scan_get_ap_records(&count, g_prov->ap_list[curr_channel]) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scanned AP records");
        goto exit;
    }
    g_prov->ap_list_len[curr_channel] = count;

    printf("%s: Scan results", TAG);
    if (g_prov->channels_per_group) {
        printf(" for channel %d", curr_channel);
    }
    printf(":\n    S.N. %-32s %s\n", "SSID", "RSSI");
    for (uint8_t i = 0; i < g_prov->ap_list_len[curr_channel]; i++) {
        printf("    [%2d] %-32s %4d\n", i,
               g_prov->ap_list[curr_channel][i].ssid,
               g_prov->ap_list[curr_channel][i].rssi);
    }
    ret = ESP_OK;

    /* Store results in sorted list */
    {
        int8_t rc = MAX_SCAN_RESULTS > count ? count : MAX_SCAN_RESULTS;
        int8_t is = MAX_SCAN_RESULTS - rc - 1;
        while (rc > 0 && is >= 0) {
            if (g_prov->ap_list_sorted[is]) {
                if (g_prov->ap_list_sorted[is]->rssi > g_prov->ap_list[curr_channel][rc - 1].rssi) {
                    g_prov->ap_list_sorted[is + rc] = &g_prov->ap_list[curr_channel][rc - 1];
                    rc--;
                    continue;
                }
                g_prov->ap_list_sorted[is + rc] = g_prov->ap_list_sorted[is];
            }
            is--;
        }
        while (rc > 0) {
            g_prov->ap_list_sorted[rc - 1] = &g_prov->ap_list[curr_channel][rc - 1];
            rc--;
        }
    }

    exit:

    if (!g_prov->channels_per_group) {
        /* All channel scan was performed
         * so nothing more to do */
        g_prov->scanning = false;
        goto final;
    }

    curr_channel = g_prov->curr_channel = (g_prov->curr_channel + 1) % 14;
    if (ret != ESP_OK || curr_channel == 0) {
        g_prov->scanning = false;
        goto final;
    }

    if ((curr_channel % g_prov->channels_per_group) == 0) {
        vTaskDelay(120 / portTICK_PERIOD_MS);
    }

    printf("%s: Scan starting on channel %u\n", TAG, curr_channel);
    g_prov->scan_cfg.channel = curr_channel;
    ret = esp_wifi_scan_start(&g_prov->scan_cfg, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scan");
        g_prov->scanning = false;
        goto final;
    }
    ESP_LOGI(TAG, "Scan started");

    final:
    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    return ret;
}

/* Event handler for starting/stopping provisioning.
 * To be called from within the context of the main
 * event handler.
 */
esp_err_t conn_mgr_prov_event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    /* If pointer to provisioning application data is NULL
     * then provisioning is not running, therefore return without
     * error */
    if (!g_prov) {
        return ESP_OK;
    }

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "STA Start");
        /* Once configuration is received through protocomm,
         * device is started as station. Once station starts,
         * wait for connection to establish with configured
         * host SSID and password */
        g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "STA Got IP");
        /* Station got IP. That means configuraion is successful.
         * Schedule timer to stop provisioning app after 30 seconds. */
        g_prov->wifi_state = WIFI_PROV_STA_CONNECTED;
        if (g_prov && g_prov->timer) {
            esp_timer_start_once(g_prov->timer, 30000*1000U);
        }
        break;

    case SYSTEM_EVENT_SCAN_DONE:
        update_wifi_scan_results();
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "STA Disconnected");
        /* Station couldn't connect to configured host SSID */
        g_prov->wifi_state = WIFI_PROV_STA_DISCONNECTED;
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        esp_wifi_disconnect();

        /* Set code corresponding to the reason for disconnection */
        switch (info->disconnected.reason) {
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGI(TAG, "STA Auth Error");
            g_prov->wifi_disconnect_reason = WIFI_PROV_STA_AUTH_ERROR;
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGI(TAG, "STA AP Not found");
            g_prov->wifi_disconnect_reason = WIFI_PROV_STA_AP_NOT_FOUND;
            break;
        default:
            /* If none of the expected reasons,
             * retry connecting to host SSID */
            g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
            esp_wifi_connect();
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

esp_err_t wifi_prov_wifi_scan_start(bool blocking, bool passive,
                                   uint8_t group_channels, uint32_t period_ms)
{
    if (!g_prov) {
        return ESP_FAIL;
    }

    bool already_scanning = false;
    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    already_scanning = g_prov->scanning;

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    if (already_scanning) {
        ESP_LOGI(TAG, "Scan already running");
        return ESP_OK;
    }

    /* Clear sorted list for new entries */
    for (uint8_t i = 0; i < MAX_SCAN_RESULTS; i++) {
        g_prov->ap_list_sorted[i] = NULL;
    }

    if (passive) {
        g_prov->scan_cfg.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        g_prov->scan_cfg.scan_time.passive = period_ms;
    } else {
        g_prov->scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        g_prov->scan_cfg.scan_time.active.min = period_ms;
        g_prov->scan_cfg.scan_time.active.max = period_ms;
    }
    g_prov->channels_per_group = group_channels;

    if (g_prov->channels_per_group) {
        printf("%s: Scan starting on channel 1\n", TAG);
        g_prov->scan_cfg.channel = 1;
    } else {
        ESP_LOGI(TAG, "Scan starting");
        g_prov->scan_cfg.channel = 0;
    }

    if (esp_wifi_scan_start(&g_prov->scan_cfg, false) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scan");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Scan started");

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    g_prov->scanning = true;
    g_prov->curr_channel = g_prov->scan_cfg.channel;

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    if (!blocking) {
        return ESP_OK;
    }

    already_scanning = true;
    while (already_scanning) {
        ESP_LOGD(TAG, "Taking in %s", __func__);
        xSemaphoreTake(g_prov_lock, portMAX_DELAY);

        already_scanning = g_prov->scanning;

        xSemaphoreGive(g_prov_lock);
        ESP_LOGD(TAG, "Releasing in %s", __func__);
        vTaskDelay(120 / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

bool wifi_prov_wifi_scan_finished(void)
{
    if (!g_prov) {
        return ESP_FAIL;
    }

    bool rval = false;

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    rval = !g_prov->scanning;

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    return rval;
}

uint16_t wifi_prov_wifi_scan_result_count(void)
{
    if (!g_prov) {
        return ESP_FAIL;
    }

    uint16_t rval = 0;
    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    while (rval < MAX_SCAN_RESULTS) {
        if (!g_prov->ap_list_sorted[rval]) {
            break;
        }
        rval++;
    }

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    return rval;
}

const wifi_ap_record_t *wifi_prov_wifi_scan_result(uint16_t index)
{
    if (!g_prov) {
        return NULL;
    }

    const wifi_ap_record_t *rval = NULL;

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    rval = g_prov->ap_list_sorted[index];

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);

    return rval;
}

esp_err_t wifi_prov_get_wifi_state(wifi_prov_sta_state_t* state)
{
    if (g_prov == NULL || state == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    *state = g_prov->wifi_state;

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);
    return ESP_OK;
}

esp_err_t wifi_prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason)
{
    if (g_prov == NULL || reason == NULL) {
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_FAIL;
    ESP_LOGD(TAG, "Taking in %s", __func__);
    xSemaphoreTake(g_prov_lock, portMAX_DELAY);

    if (g_prov->wifi_state != WIFI_PROV_STA_DISCONNECTED) {
        goto exit;
    }

    *reason = g_prov->wifi_disconnect_reason;
    ret = ESP_OK;

    exit:

    xSemaphoreGive(g_prov_lock);
    ESP_LOGD(TAG, "Releasing in %s", __func__);
    return ret;
}

esp_err_t conn_mgr_prov_is_provisioned(bool *provisioned)
{
    if (nvs_flash_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init wifi");
        return ESP_FAIL;
    }

    /* Get WiFi Station configuration */
    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) != ESP_OK) {
        *provisioned = false;
        return ESP_FAIL;
    }

    if (strlen((const char*) wifi_cfg.sta.ssid)) {
        *provisioned = true;
        printf("%s: Found ssid: %s\n", TAG, (const char*) wifi_cfg.sta.ssid);
        printf("%s: Found password: %s\n", TAG, (const char*) wifi_cfg.sta.password);
    }
    return ESP_OK;
}

esp_err_t wifi_prov_configure_sta(wifi_config_t *wifi_cfg)
{
    if (!g_prov) {
        ESP_LOGE(TAG, "Invalid state of Provisioning app");
        return ESP_FAIL;
    }

    /* Configure WiFi station with host credentials
     * provided during provisioning */
    if (esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    /* (Re)Start WiFi */
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    /* Connect to AP */
    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi");
        return ESP_FAIL;
    }

    /* Reset wifi station state for provisioning app */
    g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
    return ESP_OK;
}

esp_err_t conn_mgr_prov_start_provisioning(conn_mgr_prov_t prov, int security, const char *pop,
                                       const char *service_name, const char *service_key)
{
    /* If provisioning app data present,
     * means provisioning app is already running */
    if (g_prov) {
        ESP_LOGI(TAG, "Invalid provisioning state");
        return ESP_ERR_INVALID_STATE;
    }

    /* Create semaphore */
    if (!g_prov_lock) {
        g_prov_lock = xSemaphoreCreateMutex();
    }

    /* Allocate memory for provisioning app data */
    g_prov = (struct wifi_prov_data *) calloc(1, sizeof(struct wifi_prov_data));
    if (!g_prov) {
        ESP_LOGI(TAG, "Unable to allocate prov data");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize app data */
    g_prov->pop.len = strlen(pop);
    g_prov->pop.data = malloc(g_prov->pop.len);
    if (!g_prov->pop.data) {
        ESP_LOGI(TAG, "Unable to allocate PoP data");
        free(g_prov);
        return ESP_ERR_NO_MEM;
    }
    memcpy((void *)g_prov->pop.data, pop, g_prov->pop.len);
    g_prov->security = security;
    g_prov->prov = prov;

    /* Create timer object as a member of app data */
    esp_timer_create_args_t timer_conf = {
        .callback = _stop_prov_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "stop_softap_tm"
    };
    esp_err_t err = esp_timer_create(&timer_conf, &g_prov->timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer");
        free((void *)g_prov->pop.data);
        free(g_prov);
        return err;
    }

    /* Initialize WiFi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WiFi");
        return ESP_FAIL;
    }

#if 0
    /* Reset WiFi settings */
    if (esp_wifi_restore() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset WiFi settings");
        return ESP_FAIL;
    }
#endif

    wifi_config_t wifi_config = {0};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    /* Configure WiFi as Station */
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode");
        return ESP_FAIL;
    }

    /* Start WiFi */
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }

    /* Start provisioning service */
    err = conn_mgr_prov_start_service(service_name, service_key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Provisioning failed to start");
        esp_timer_delete(g_prov->timer);
        free((void *)g_prov->pop.data);
        free(g_prov);
        return err;
    }
    return ESP_OK;
}
