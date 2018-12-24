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

#include "conn_mgr_prov.h"
#include "conn_mgr_prov_priv.h"

static const char *TAG = "conn_mgr_prov";

static void conn_mgr_prov_extra_mem_release();

/* Handlers for wifi_config provisioning endpoint */
extern wifi_prov_config_handlers_t wifi_prov_handlers;

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
};

/* Pointer to provisioning application data */
static struct wifi_prov_data *g_prov;
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
    protocomm_set_version(g_prov->pc, "proto-ver", "V0.1");

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

    if (g_prov->prov.event_cb) {
        g_prov->prov.event_cb(g_prov->prov.cb_user_data, CM_ENDPOINT_ADD);
    }

    ESP_LOGI(TAG, "Provisioning started with : \n\tservice name = %s \n\tservice key = %s", service_name, service_key);
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
    /* Release memory used by BT stack */
    esp_err_t err = esp_bt_mem_release(ESP_BT_MODE_BTDM);
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
#if CONFIG_BT_ENABLED
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

    /* Free provisioning process data */
    free((void *)g_prov->pop.data);
    free(g_prov);
    g_prov = NULL;
    ESP_LOGI(TAG, "Provisioning stopped");

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

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "STA Disconnected");
        /* Station couldn't connect to configured host SSID */
        g_prov->wifi_state = WIFI_PROV_STA_DISCONNECTED;
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);

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

esp_err_t wifi_prov_get_wifi_state(wifi_prov_sta_state_t* state)
{
    if (g_prov == NULL || state == NULL) {
        return ESP_FAIL;
    }

    *state = g_prov->wifi_state;
    return ESP_OK;
}

esp_err_t wifi_prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason)
{
    if (g_prov == NULL || reason == NULL) {
        return ESP_FAIL;
    }

    if (g_prov->wifi_state != WIFI_PROV_STA_DISCONNECTED) {
        return ESP_FAIL;
    }

    *reason = g_prov->wifi_disconnect_reason;
    return ESP_OK;
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
        ESP_LOGI(TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
    }
    return ESP_OK;
}

esp_err_t wifi_prov_configure_sta(wifi_config_t *wifi_cfg)
{
    if (!g_prov) {
        ESP_LOGE(TAG, "Invalid state of Provisioning app");
        return ESP_FAIL;
    }

    /* Initialize WiFi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WiFi");
        return ESP_FAIL;
    }

    /* Configure WiFi as both AP and/or Station */
    if (esp_wifi_set_mode(g_prov->prov.wifi_mode) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode");
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

    /* Allocate memory for provisioning app data */
    g_prov = (struct wifi_prov_data *) calloc(1, sizeof(struct wifi_prov_data));
    if (!g_prov) {
        ESP_LOGI(TAG, "Unable to allocate prov data");
        return ESP_ERR_NO_MEM;
    }

    /* Initialise app data */
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
