// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_a2dp_api.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_log.h>
#include <esp_wifi.h> /* For mac */

#include <UUIDGeneration.h>
#include "bt_app_av.h"
#include "bluetooth.h"
#include "sys/lock.h"

static const char *TAG = "[bluetooth]";

/* Minimum 3 characters. Last 3 XXX will be replaced with last 3 nibbles from physical address. */
#define DEFAULT_BT_DEVICE_NAME      "ESP32-XXX"

static int bt_enable_ref_count;
static bool bt_stack_up;
static SemaphoreHandle_t bt_ct_enable_disable_mutex;
/* Event group to synchronize few things */
static EventGroupHandle_t bt_event_group;
/* BT events */
const int REMOTE_SERVICES_GET = BIT0;
const int DISCONNECT_SUCCESS = BIT1;
const int BT_STACK_UP = BIT2;

size_t alexa_supported_profiles_t[] = {
    [A2DP_SOURCE] = 0x110A, /* AudioSource */
    [A2DP_SINK] = 0x110B, /* AudioSink */
    [AVRC_TARGET] = 0x110C, /* AV_RemoteControlTarget */
    [AVRC] = 0x110E, /* AV_RemoteControl */
    [AVRC_CONTROLLER] = 0x110F, /* AV_RemoteControlController */
};
size_t SUPPORTED_PROFILES_COUNT = sizeof(alexa_supported_profiles_t) / sizeof(size_t);

static struct scanned_device_info scand EXT_RAM_ATTR;

static struct bluetooth {
    bt_event_handler_t event_handler;
    sys_playback_requester_t playback_requester;
    char device_name[ESP_BT_GAP_MAX_BDNAME_LEN];
    enum supported_profiles current_profile;
} bt EXT_RAM_ATTR;

static struct paired_device_info paird EXT_RAM_ATTR;

/* Forward declarations */
static esp_err_t bluetooth_scan_add(char *peer_bdname, uint8_t peer_bdname_len, esp_bd_addr_t peer_bd_addr, enum supported_profiles peer_profile);
static esp_err_t bluetooth_connect(esp_bd_addr_t bda, enum supported_profiles profile_to_disconnect);
static esp_err_t bluetooth_disconnect(esp_bd_addr_t bda);
static esp_err_t bluetooth_sink_start();
static esp_err_t bluetooth_source_start();
static esp_err_t bluetooth_sink_stop();
static esp_err_t bluetooth_source_stop();
static int bluetooth_remove_pairing(int index);

static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir) {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len) {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    uint8_t peer_bdname_len;
    char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];

    ESP_LOGD(TAG, "GAP event: %d", event);
    switch (event) {
    /**
     * Need a connection started event with COD info.
     * Upon which we should be able to reject a connection.
     * This would avoid a case where speaker connects when we are in SINK mode.
     */
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            /* This device name needs to go in bluetooth.c and then to NVS */
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);

            /* Update connecting_bd */
            if (memcmp(paird.connecting_bd.bda, param->auth_cmpl.bda, sizeof(esp_bd_addr_t)) != 0) {
                memcpy(paird.connecting_bd.bda, param->auth_cmpl.bda, sizeof(esp_bd_addr_t));
                strlcpy(paird.connecting_bd.friendly_name, (const char *) &param->auth_cmpl.device_name, MAX_FRIENDLY_NAME_LEN);

                paird.connecting_bd.profiles = 1 << A2DP_SOURCE;
                ESP_LOGI(TAG, "Generating UUID...! line = %d", __LINE__);
                char *tmp_uuid = (char *) generateUUID();
                memcpy(paird.connecting_bd.uuid, tmp_uuid, MAX_UUID_SIZE);
                free(tmp_uuid);
            }
            /* Doesn't really hurt to query this again! */
            esp_bt_gap_get_remote_services((uint8_t *) paird.connecting_bd.bda);
        } else if (param->auth_cmpl.stat == ESP_BT_STATUS_AUTH_FAILURE ||
                param->auth_cmpl.stat == ESP_BT_STATUS_AUTH_REJECTED) {
            ESP_LOGE(TAG, "Pair/Repair failed, status:%d! Pairing info should be removed...", param->auth_cmpl.stat);

            /* ToDo: Ask to stop retries */

            if (bt.event_handler) {
                /**
                 * Send repair failed event
                 * Handler is expected to remove pairing info.
                 */
                bt.event_handler(EVENT_BT_REPAIR_FAILED, (void *) param->auth_cmpl.bda);
            }
        } else {
            ESP_LOGI(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
#if (BT_SSP_INCLUDED == TRUE)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif
    case ESP_BT_GAP_DISC_RES_EVT: {
        uint32_t cod = 0;
        uint8_t *eir = NULL;
        esp_bt_gap_dev_prop_t *p;
        enum supported_profiles profile = 1 << A2DP_SOURCE; //default
        uint8_t *peer_bd_addr = NULL;

        for (int i = 0; i < param->disc_res.num_prop; i++) {
            p = param->disc_res.prop + i;
            switch (p->type) {
                case ESP_BT_GAP_DEV_PROP_EIR:
                    eir = (uint8_t *) (p->val);
                    peer_bd_addr = param->disc_res.bda;
                    break;
                case ESP_BT_GAP_DEV_PROP_COD:
                    cod = *(uint32_t *) (p->val);
                    break;
                case ESP_BT_GAP_DEV_PROP_BDNAME:
                    if (!peer_bd_addr) {
                        peer_bd_addr = param->disc_res.bda;
                    }
                    break;
                case ESP_BT_GAP_DEV_PROP_RSSI:
                default:
                    break;
            }
        }

        /* Search for device with MAJOR service class as "rendering" or "audio" in COD */
        if (esp_bt_gap_is_valid_cod(cod)) {
            int cod_srvc = esp_bt_gap_get_cod_srvc(cod);
            if ((cod_srvc & ESP_BT_COD_SRVC_RENDERING) || (cod_srvc & ESP_BT_COD_SRVC_AUDIO)) {
                /* Profile is a2dp_sink */
                profile = 1 << A2DP_SINK;
            }
            if (eir) {
                get_name_from_eir(eir, peer_bdname, &peer_bdname_len);
            }
            bluetooth_scan_add(peer_bdname, peer_bdname_len, peer_bd_addr, profile);
        }
    }
        break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            scand.discovering = true;
            ESP_LOGD(TAG, "BT device discovery started");
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            /* TODO: Re-start scanning if not done from the cloud */
            ESP_LOGD(TAG, "BT device discovery stopped");
            /* XXX: We receive the event twice, check would enqueue event only once */
            if (scand.discovering) {
                scand.discovering = false;
                if (scand.connect_on_found && (paird.connect_status != BT_CONNECTING)) {
                    if (bt.event_handler) {
                        bt.event_handler(EVENT_NO_SCAN_DEVICES, NULL);
                    }
                } else if (!scand.connect_on_found) {
                    if (bt.event_handler) {
                        bt.event_handler(EVENT_SCAN_DEVICES_UPDATED, NULL);
                    }
                }
            }
        }
        break;
    case ESP_BT_GAP_RMT_SRVCS_EVT: {
        ESP_LOGD(TAG, "Supported profiles for %s: %d", paird.connecting_bd.friendly_name, param->rmt_srvcs.num_uuids);
        for (int i = 0 ; i < param->rmt_srvcs.num_uuids; i++) {
            /* ESP_LOGE(TAG,"%d",param->rmt_srvcs.uuid_list[i].len); */
            if (param->rmt_srvcs.uuid_list[i].len == 2) {
                for (int j = 0; j < SUPPORTED_PROFILES_COUNT; j++) {
                    if (param->rmt_srvcs.uuid_list[i].uuid.uuid16 == alexa_supported_profiles_t[j]) {
                        ESP_LOGD(TAG, "%x", param->rmt_srvcs.uuid_list[i].uuid.uuid16);
                        paird.connecting_bd.profiles |= 1 << j;
                    }
                }
            }
        }
        ESP_LOGD(TAG, "Supported profiles for %s are 0x%x", paird.connecting_bd.friendly_name, paird.connecting_bd.profiles);
        xEventGroupSetBits(bt_event_group, REMOTE_SERVICES_GET);
        break;
    }
    default:
        ESP_LOGD(TAG, "event: %d", event);
        break;
    }
    return;
}

extern int32_t bt_app_a2d_source_data_cb(uint8_t *data, int32_t len);

static inline int bluetooth_stack_up()
{
    /* set up device name */
    strlcpy(bt.device_name, DEFAULT_BT_DEVICE_NAME, sizeof(bt.device_name));
    int len = strlen(bt.device_name);

    /* Get device address and replace last three characters of device name with last 3 nibbles from device address. */
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(bt.device_name + len - 3, 4, "%01X%02X", mac[4] & 0xF, mac[5]);

    return ESP_OK;
}

static int bluetooth_player_read_cb(void *arg, void *data, int len, unsigned int wait)
{
    int ret;
    ret = arb_read(bt_app_av_get_rb(), (uint8_t *) data, len, wait);
    if (ret == RB_READER_UNBLOCK) {
        /* Just a wake-up */
    } else if (ret < 0) {
        /* Stop the streams */
        bt_sink_rb_reset();
    } else {
        /* Normal data */
    }
    return ret;
}

void bluetooth_player_wakeup_reader_cb(void *arg)
{
    arb_wakeup_reader(bt_app_av_get_rb());
}

esp_err_t bluetooth_init(bt_event_handler_t event_handler)
{
    ESP_LOGD(TAG, "Bluetooth init");
    bt.current_profile = A2DP_NONE;

    /* Bluetooth device name, connection mode and profile set up */
    if (bluetooth_stack_up() != ESP_OK)
        return ESP_FAIL;

    bt_ct_enable_disable_mutex = xSemaphoreCreateMutex();
    if (!bt_ct_enable_disable_mutex) {
        ESP_LOGE(TAG, "Error creating BT controller mutex");
        return ESP_FAIL;
    }
    bt_event_group = xEventGroupCreate();
    if (bt_event_group == NULL) {
        ESP_LOGE(TAG, "bt_event_group create failed");
        vSemaphoreDelete(bt_ct_enable_disable_mutex);
        return ESP_FAIL;
    }
    event_handler(EVENT_BT_GET_PAIRED_DEVICES, (void *) &paird);

    /* XXX: Required only for BT sink */
    bt.playback_requester.read_cb = &bluetooth_player_read_cb;
    bt.playback_requester.wakeup_reader_cb = &bluetooth_player_wakeup_reader_cb;
    bt.playback_requester.audio_info.channels = 2;
    bt.playback_requester.audio_info.bits_per_sample = 16;
    bt.playback_requester.audio_info.sample_rate = 44100;

    bt.event_handler = event_handler;

    if (ESP_OK != bt_av_init(event_handler)) {
        ESP_LOGE(TAG, "bt_av_init failed");
        vSemaphoreDelete(bt_ct_enable_disable_mutex);
        vEventGroupDelete(bt_event_group);
        bt_event_group = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* These start/stop calls are mainly used for device to exit/enter out of/into low power state */
void bluetooth_stop(void *arg)
{
    esp_err_t err;

    if (scand.discovering) {
        err = esp_bt_gap_cancel_discovery();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to cancel GAP discovery");
        }
        err = esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop scanning");
        }
    }

    if (paird.connect_status == BT_CONNECTED) {
        ESP_LOGE(TAG, "About to disconnect");
        err = bluetooth_disconnect_device();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to disconnect source");
        }
    }

    /* Deinitialize BT */
    err = esp_avrc_ct_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit avrc ct");
        goto stop_end;
    }

    if (bt.current_profile == A2DP_SINK) {
        bluetooth_sink_stop();
    } else if (bt.current_profile == A2DP_SINK) {
        bluetooth_source_stop();
    }

    if ((err = esp_bluedroid_disable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %d", __func__, err);
        goto stop_end;
    }

    if ((err = esp_bluedroid_deinit()) != ESP_OK) {
        ESP_LOGE(TAG, "%s deinitialize bluedroid failed: %d", __func__, err);
        goto stop_end;
    }

    if ((err = esp_bt_controller_disable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s disable controller failed: %d", __func__, err);
        goto stop_end;
    }

    if ((err = esp_bt_controller_deinit()) != ESP_OK) {
        ESP_LOGE(TAG, "%s deinit controller failed: %d", __func__, err);
        goto stop_end;
    }
    bt_stack_up = false;
stop_end:
    xEventGroupSetBits(bt_event_group, BT_STACK_UP);
    vTaskDelete(NULL);
}

static esp_err_t bluetooth_sink_stop()
{
    esp_err_t err = ESP_OK;

    if (bt.current_profile == A2DP_NONE) {
        ESP_LOGW(TAG, "Nothing to deinit");
        return ESP_OK;
    }

    err = esp_a2d_sink_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit sink");
    }

    bt.current_profile = A2DP_NONE;
    return err;
}

static esp_err_t bluetooth_source_stop()
{
    esp_err_t err = ESP_OK;

    if (bt.current_profile == A2DP_NONE) {
        ESP_LOGW(TAG, "Nothing to deinit");
        return ESP_OK;
    }

    err = esp_a2d_source_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit source");
    }

    bt.current_profile = A2DP_NONE;
    return err;
}

static esp_err_t bluetooth_sink_start()
{
#if !ALEXA_BT_SINK
    return ESP_FAIL;
#endif
    if (bt.current_profile == A2DP_SOURCE) {
        bluetooth_source_stop();
    } else if (bt.current_profile == A2DP_SINK) {
        ESP_LOGI(TAG, "A2DP-SINK is already started!");
        return ESP_OK;
    }

    esp_a2d_register_callback(&bt_app_a2d_sink_cb);
    esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);

    if (esp_a2d_sink_init() != ESP_OK) {
        ESP_LOGE(TAG, "%s A2DP sink init failed\n", __func__);
        return ESP_FAIL;
    }

    bt.current_profile = A2DP_SINK;
    return ESP_OK;
}

static esp_err_t bluetooth_source_start()
{
#if !ALEXA_BT_SOURCE
    return ESP_FAIL;
#endif
    if (bt.current_profile == A2DP_SINK) {
        bluetooth_sink_stop();
    } else if (bt.current_profile == A2DP_SOURCE) {
        ESP_LOGI(TAG, "A2DP-SOURCE is already started!");
        return ESP_OK;
    }

    esp_a2d_register_callback(&bt_app_a2d_source_cb);
    esp_a2d_source_register_data_callback(bt_app_a2d_source_data_cb);

    if (esp_a2d_source_init() != ESP_OK) {
        ESP_LOGE(TAG, "%s A2DP source init failed\n", __func__);
        return ESP_FAIL;
    }

    bt.current_profile = A2DP_SOURCE;
    return ESP_OK;
}

static esp_err_t bluetooth_connect(esp_bd_addr_t bda, enum supported_profiles profile_to_connect)
{
    /**
     * profile_to_connect should be opposite of current_profile.
     * If not, we stop old profile.
     */
    if (profile_to_connect == bt.current_profile) {
        if (bt.current_profile == A2DP_SINK) {
#if !ALEXA_BT_SOURCE
            // Only SINK is supported, do not stop
            return ESP_FAIL;
#endif
            bluetooth_sink_stop();
        } else {
#if !ALEXA_BT_SINK
            // Only SOURCE is supported, do not stop
            return ESP_FAIL;
#endif
            bluetooth_source_stop();
        }
    }

    /* If no profile is active at this point, init one we need to connect */
    if (bt.current_profile == A2DP_NONE) {
        if (profile_to_connect == A2DP_SINK) {
            bluetooth_source_start();
        } else {
            bluetooth_sink_start();
        }
    }

    /* Finally, connect a bt device */
    if (profile_to_connect == A2DP_SINK) {
#if !ALEXA_BT_SOURCE
        return ESP_FAIL;
#endif
        return esp_a2d_source_connect(bda);
    } else {
#if !ALEXA_BT_SINK
        return ESP_FAIL;
#endif
        return esp_a2d_sink_connect(bda);
    }
}

static esp_err_t bluetooth_disconnect(esp_bd_addr_t bda)
{
    if (bt.current_profile == A2DP_SOURCE) {
        esp_a2d_source_disconnect(bda);
    } else {
        esp_a2d_sink_disconnect(bda);
    }
    return ESP_OK;
}

void bluetooth_start(void *arg)
{
    esp_err_t err;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %d", __func__, err);
        goto start_end;
    }

    if ((err = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %d", __func__, err);
        goto start_end;
    }

    if ((err = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %d", __func__, err);
        goto start_end;
    }

    if ((err = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %d", __func__, err);
        goto start_end;
    }

    bluetooth_set_device_name(NULL);

    esp_bt_gap_register_callback(bt_app_gap_cb);

    /* initialize AVRCP controller */
    if (esp_avrc_ct_init() != ESP_OK) {
        ESP_LOGE(TAG, "%s AVRC controller init failed: %d", __func__, err);
        goto start_end;
    }

    esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
    if (esp_avrc_tg_init() != ESP_OK) {
        ESP_LOGE(TAG, "%s AVRC target init failed: %d", __func__, err);
        goto start_end;
    }

    esp_avrc_tg_register_callback(bt_app_rc_tg_cb);

    esp_avrc_psth_bit_mask_t cmd_set = {0};
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_PLAY);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_PAUSE);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_STOP);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_FORWARD);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_BACKWARD);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_VOL_UP);
    esp_avrc_psth_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &cmd_set, ESP_AVRC_PT_CMD_VOL_DOWN);
    esp_avrc_tg_set_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmd_set);

#if A2DP_SINK_VOLUME_REGISTRATION
    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
    if (esp_avrc_tg_set_rn_evt_cap(&evt_set) != ESP_OK) {
        ESP_LOGE(TAG, "%s AVRC target op failed: %d", __func__, err);
        goto start_end;
    }
#endif

#if ALEXA_BT_SINK
    bluetooth_sink_start(); /* Start in SINK mode by default. */
#else
    bluetooth_source_start();
#endif

    bt_stack_up = true;
start_end:
    xEventGroupSetBits(bt_event_group, BT_STACK_UP);
    vTaskDelete(NULL);
}

esp_err_t bluetooth_enable_host_ct()
{
    ESP_LOGI(TAG, "Enabling BT host and controller");
    esp_err_t err = ESP_OK;
    xSemaphoreTake(bt_ct_enable_disable_mutex, portMAX_DELAY);
    if (bt_enable_ref_count++ == 0) {
        TaskHandle_t task_handle;
        xTaskCreate(bluetooth_start, "bluetooth_start", 8192, NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &task_handle);
        /* Wait for operation to complete */
        xEventGroupWaitBits(bt_event_group, BT_STACK_UP, true, true, portMAX_DELAY);
        if (!bt_stack_up)
            err = ESP_FAIL;
    }

#if (BT_SSP_INCLUDED == TRUE)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    xSemaphoreGive(bt_ct_enable_disable_mutex);
    return err;
}

void bluetooth_disable_host_ct()
{
#ifdef CONFIG_PM_ENABLED
    ESP_LOGI(TAG, "Disabling BT host and controller");
    xSemaphoreTake(bt_ct_enable_disable_mutex, portMAX_DELAY);
    if (--bt_enable_ref_count == 0) {
        TaskHandle_t task_handle;
        xTaskCreate(bluetooth_stop, "bluetooth_stop", 4096, NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, &task_handle);
        /* Wait for operation to complete */
        xEventGroupWaitBits(bt_event_group, BT_STACK_UP, true, true, portMAX_DELAY);
    } else if (bt_enable_ref_count < 0) {
        bt_enable_ref_count = 0;
    } else {
        ESP_LOGE(TAG, "BT enable ref count not 0. Not disabling BT");
    }
    xSemaphoreGive(bt_ct_enable_disable_mutex);
#else
#if ALEXA_BT_SINK
    /**
     * Always fall back to A2DP-SINK when disconnected.
     * This allows mobile devices to connect directly
     */
    bluetooth_sink_start();
#endif
#endif
}

esp_err_t bluetooth_enable_discoverable()
{
    /* set discoverable and connectable mode, wait to be connected */
    return esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
}

esp_err_t bluetooth_enable_connectable()
{
    return esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
}

esp_err_t bluetooth_disable_discoverable()
{
    return esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
}

enum supported_profiles bluetooth_get_connected_profile()
{
    return paird.bd[0].profiles & (1 << A2DP_SINK) ? A2DP_SINK : A2DP_SOURCE;
}

char *bluetooth_get_connected_name()
{
    return paird.bd[0].friendly_name;
}

char *bluetooth_get_connected_uuid()
{
    return paird.bd[0].uuid;
}

char *bluetooth_get_connecting_name()
{
    return paird.connecting_bd.friendly_name;
}

char *bluetooth_get_connecting_uuid()
{
    return paird.connecting_bd.uuid;
}

static uint8_t tl;
static _lock_t s_tl_lock;

esp_err_t bluetooth_mediacontrol_pause()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_mediacontrol_stop()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_STOP, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_STOP, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_volumecontrol_up()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_VOL_UP, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_VOL_UP, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_volumecontrol_down()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_VOL_DOWN, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    vTaskDelay(35);
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_VOL_DOWN, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_set_absolute_volume(uint8_t vol)
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_set_absolute_volume_cmd(tl, vol);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_mediacontrol_play()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_mediacontrol_previous()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_BACKWARD, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_BACKWARD, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_mediacontrol_next()
{
    esp_err_t ret;
    _lock_acquire(&s_tl_lock);
    ret = esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_FORWARD, ESP_AVRC_PT_CMD_STATE_PRESSED);
    tl = (tl + 1) % 16;
    ret |= esp_avrc_ct_send_passthrough_cmd(tl, ESP_AVRC_PT_CMD_FORWARD, ESP_AVRC_PT_CMD_STATE_RELEASED);
    tl = (tl + 1) % 16;
    _lock_release(&s_tl_lock);
    return ret;
}

esp_err_t bluetooth_connect_device_by_bda(esp_bd_addr_t bda)
{
    esp_err_t ret = ESP_FAIL;
    /* Check if already paired device. */
    for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
        if (memcmp(paird.bd[i].bda, bda, sizeof (esp_bd_addr_t)) == 0) {
            if (paird.bd[i].profiles & (1 << A2DP_SINK)) {
                /* Profile to connect is A2DP SINK. */
                ret = bluetooth_connect(paird.bd[i].bda, A2DP_SINK);
            } else {
                ret = bluetooth_connect(paird.bd[i].bda, A2DP_SOURCE);
            }

            if (ret == ESP_OK) {
                memcpy(&paird.connecting_bd, &paird.bd[i], sizeof(struct bluetooth_dev_info));
                ESP_LOGD(TAG, "connecting to %s", paird.bd[i].friendly_name);
            } else {
                ESP_LOGE(TAG, "Failed to connect to %s", paird.bd[i].friendly_name);
            }
            return ret;
        }
    }

    /* Device is not already paired */
    ESP_LOGW(TAG, "Cannot find device in list of paired devices. Pairing and connecting it...");

    /* Zero out `connecting bda` that this will be populated again. */
    bzero(paird.connecting_bd.bda, sizeof (struct bluetooth_dev_info));

    /* This is tricky, in a way that we do not know what profile we are connecting! */
#if ALEXA_BT_SINK
    ret = bluetooth_connect(bda, A2DP_SINK);
#else
    ret = bluetooth_connect(bda, A2DP_SOURCE);
#endif

    return ret;
}

/* XXX: Assumption here is device should already be paired and hence must be present in
 * list of paired devices */
esp_err_t bluetooth_connect_device_by_id(char uuid[])
{
    for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
        if (strncmp(paird.bd[i].uuid, uuid, MAX_UUID_SIZE) == 0) {
            esp_err_t ret = ESP_FAIL;
            if (paird.bd[i].profiles & (1 << A2DP_SINK)) {
                /* Profile to connect is A2DP-SINK. */
                ret = bluetooth_connect(paird.bd[i].bda, A2DP_SINK);
            } else {
                ret = bluetooth_connect(paird.bd[i].bda, A2DP_SOURCE);
            }

            if (ret == ESP_OK) {
                memcpy(&paird.connecting_bd, &paird.bd[i], sizeof(struct bluetooth_dev_info));
                ESP_LOGD(TAG, "connecting to %s", paird.bd[i].friendly_name);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to connect to %s", paird.bd[i].friendly_name);
                return ESP_FAIL;
            }
        }
    }
    ESP_LOGE(TAG, "Cannot find device in list of paired devices");
    return ESP_FAIL;
}

esp_err_t bluetooth_connect_device_by_profile(uint16_t profile, bool next)
{
    static int next_connect_dev_idx = 0;
    if (!next) {
        next_connect_dev_idx = 0;
    }

    for (int i = next_connect_dev_idx; i < MAX_PAIRED_DEVICES; i++) {
        ESP_LOGI(TAG, "Device profiles = 0x%02x, friendly_name = %s", paird.bd[i].profiles, paird.bd[i].friendly_name);
        if (paird.bd[i].friendly_name[0]) {
            /*TODO: For now, we connect to any device in paired list, since we are not
             * getting supported profiles for some of the devices */

            esp_err_t err = ESP_FAIL;

            if (paird.bd[i].profiles & profile) {
                if (paird.bd[i].profiles & (1 << A2DP_SINK)) {
                    err = bluetooth_connect(paird.bd[i].bda, A2DP_SINK);
                } else {
                    err = bluetooth_connect(paird.bd[i].bda, A2DP_SOURCE);
                }

                if (err == ESP_OK) {
                    memcpy(&paird.connecting_bd, &paird.bd[i], sizeof(struct bluetooth_dev_info));
                    printf("%s : Connecting to %s\n", TAG, paird.bd[i].friendly_name);
                    next_connect_dev_idx = ++i;
                    return ESP_OK;
                } else {
                    ESP_LOGE(TAG, "Failed to connect to %s profile %d. Trying other paired devices", paird.bd[i].friendly_name, profile);
                    continue;
                }
            }
        }
    }
    ESP_LOGE(TAG, "No device with profile 0x%2x found.", profile);
    next_connect_dev_idx = 0;
    return ESP_FAIL;
}

static inline void bluetooth_remove_ram_entry(int index)
{
    int dev_shifts = paird.count-index-1;
    memset(&paird.bd[index], 0, sizeof(struct bluetooth_dev_info));

    for (int i = index; dev_shifts > 0; dev_shifts--,i++) {
        memcpy(&paird.bd[i], &paird.bd[i+1], sizeof(struct bluetooth_dev_info));
    }
}

static int bluetooth_remove_pairing(int index)
{
    bluetooth_remove_ram_entry(index);
    paird.count--;
    bt.event_handler(EVENT_BT_PAIRED_DEVICES_UPDATED, &paird);
    return ESP_OK;
}

static inline void bluetooth_add_move_ram_entry(struct bluetooth_dev_info *connected_dev)
{
    int current_idx = -1;
    /* Check if already exists */
    for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
        if (memcmp(connected_dev->bda, paird.bd[i].bda, sizeof(esp_bd_addr_t)) == 0)
            current_idx = i;
    }

    int dev_shifts = 0;
    if (current_idx > -1) {
        /* Device already paired */
        dev_shifts = current_idx;
    } else {
        /* New pairing */
        if (paird.count >= MAX_PAIRED_DEVICES) {
            /* XXX: UnpairDeviceSucceeded event needed? */
            ESP_LOGW(TAG, "Max pairing count reached...");
            ESP_LOGW(TAG, "Removing : %s device from the paired list", paird.bd[MAX_PAIRED_DEVICES-1].friendly_name);
            esp_bt_gap_remove_bond_device(paird.bd[MAX_PAIRED_DEVICES-1].bda);
            bluetooth_remove_pairing(MAX_PAIRED_DEVICES-1);
        }
        dev_shifts = paird.count;
        paird.count++;
    }
    for (int i = dev_shifts; i > 0; i--) {
        /* Right shift device list */
        memcpy(&paird.bd[i], &paird.bd[i-1], sizeof(struct bluetooth_dev_info));
    }
    /* Add connected device info to top */
    memcpy(&paird.bd[0], connected_dev, sizeof(struct bluetooth_dev_info));
}

/* Returns index from RAM copy of paired device list */
static int bluetooth_store_pairing(struct bluetooth_dev_info *dev)
{
    bluetooth_add_move_ram_entry(dev);
    bt.event_handler(EVENT_BT_PAIRED_DEVICES_UPDATED, &paird);
    return ESP_OK;
}

void bluetooth_on_connect_success(esp_bd_addr_t connect_bda)
{
    ESP_LOGI(TAG, "connect_bda: ");
    for (int i = 0; i < ESP_BD_ADDR_LEN; i++) {
        ESP_LOGI(TAG, "%02X:", connect_bda[i]);
    }
    ESP_LOGI(TAG, "connecting_bd: ");
    for (int i = 0; i < ESP_BD_ADDR_LEN; i++) {
        ESP_LOGI(TAG, "%02X:", paird.connecting_bd.bda[i]);
    }

    if (memcmp(connect_bda, paird.connecting_bd.bda, ESP_BD_ADDR_LEN) == 0) {
        esp_err_t ret = esp_bt_gap_get_remote_services((uint8_t *)paird.connecting_bd.bda);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error calling get remote services. Setting supported profiles to None");
        } else {
            EventBits_t ret = xEventGroupWaitBits(bt_event_group, REMOTE_SERVICES_GET, true, true, pdMS_TO_TICKS(1000));
            if (ret == 0) {
                ESP_LOGE(TAG, "Timeout getting remote services. Supported profiles could be set to None");
                if (!paird.connecting_bd.profiles) {
                    if (bt.current_profile == A2DP_SOURCE) {
                        /* If current profile is SOURCE! We must have been connected to a SINK device */
                        paird.connecting_bd.profiles |= 1 << A2DP_SINK;
                    } else { /* SINK or NONE */
                        paird.connecting_bd.profiles |= 1 << A2DP_SOURCE;
                    }
                }
            }
        }
        bluetooth_store_pairing(&paird.connecting_bd);
    } else {
        ESP_LOGW(TAG, "Connected to a device other than attempting to connect");
        /* This is the case when speaker gets connected without Alexa's utterance */
        for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
            if (memcmp(connect_bda, paird.bd[i].bda, ESP_BD_ADDR_LEN) == 0) {
                memcpy(&paird.connecting_bd, &paird.bd[i], sizeof(struct bluetooth_dev_info));
                paird.connecting_bd.profiles = paird.bd[i].profiles;
                bluetooth_store_pairing(&paird.connecting_bd);
            }
        }
    }
    paird.connect_status = BT_CONNECTED;

    if (bt.event_handler) {
        bt.event_handler(EVENT_BT_CONNECTED, NULL);
    }
}

void bluetooth_on_disconnect_success()
{
    if (bt.event_handler) {
        bt.event_handler(EVENT_BT_DISCONNECTED, NULL);
    }
    xEventGroupSetBits(bt_event_group, DISCONNECT_SUCCESS);
    paird.connect_status = BT_DISCONNECTED;
}

esp_err_t bluetooth_pair_device(char uuid[])
{
    /* TODO: Check for max paired devices count */
    /* Create new BluetoothDevice */
    /* BluetoothDevice new_device = BLUETOOTH_DEVICE__INIT; */

    /* Read list of existing paired devices from nvs */

    /* See if new_device already exists */

    /* Add new_device to list of paired devices */

    bluetooth_scan_stop();

    if (paird.count >= MAX_PAIRED_DEVICES) {
        ESP_LOGE(TAG, "Max pairing count reached...can't pair a new device!");
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_FAIL;
    /* Write list of paired devices to nvs */
    /* XXX: Check for MAX_SCAN_DEVICES to avoid race condition wherein device sets has_more false,
     * and scan followed by pairing is received. (for older scan list). If checked for scand.count,
     * pairing will fail (since it becomes 0 in bluetooth_scan_start).
     */
    for (int i = 0 ; i < MAX_SCAN_DEVICES; i++) {
        if (strncmp(scand.bd[i].uuid, uuid, MAX_UUID_SIZE) == 0) {
            if (scand.bd[i].profiles & (1 << A2DP_SINK)) {
                /* Device to connect is A2DP-SINK */
                ret = bluetooth_connect((uint8_t *)scand.bd[i].bda, A2DP_SINK);
            } else {
                ret = bluetooth_connect((uint8_t *)scand.bd[i].bda, A2DP_SOURCE);
            }

            if (ret == ESP_OK) {
                memcpy(paird.connecting_bd.bda, scand.bd[i].bda, sizeof(esp_bd_addr_t));
                strlcpy(paird.connecting_bd.uuid, scand.bd[i].uuid, MAX_UUID_SIZE);
                strlcpy(paird.connecting_bd.friendly_name, scand.bd[i].friendly_name, MAX_FRIENDLY_NAME_LEN);
                paird.connecting_bd.profiles = scand.bd[i].profiles;
                ESP_LOGD(TAG, "connecting to %s", scand.bd[i].friendly_name);
            } else {
                ESP_LOGE(TAG, "Failed to connect to %s", scand.bd[i].friendly_name);
            }
            break;
        }
    }
    return ret;
}

/* XXX: This could come on boot up or from PairDevice or ConnectById/Profile request. Need to handle appropriately */
esp_err_t bluetooth_retry_connection()
{
    esp_err_t ret = ESP_FAIL;

    if (paird.connecting_bd.profiles & (1 << A2DP_SINK)) {
        ret = bluetooth_connect(paird.connecting_bd.bda, A2DP_SINK);
    } else {
        ret = bluetooth_connect(paird.connecting_bd.bda, A2DP_SOURCE);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error re-trying connecting to BT device");
    }
    return ret;
}

esp_err_t bluetooth_disconnect_device()
{
    if (paird.connect_status != BT_CONNECTED) {
        ESP_LOGW(TAG, "No device to disconnect! line %d", __LINE__);
        return ESP_FAIL;
    }
    bt_app_stop_media();

    xEventGroupClearBits(bt_event_group, 0xFF);
    esp_err_t ret = ESP_FAIL;

    ret = bluetooth_disconnect(paird.bd[0].bda);

    xEventGroupWaitBits(bt_event_group, DISCONNECT_SUCCESS, true, true, pdMS_TO_TICKS(5000));
    /* Enter low-power mode */
    paird.connect_status = BT_DISCONNECTED;
    return ret;
}

esp_err_t bluetooth_unpair_device_by_bda(const esp_bd_addr_t bda, char *uuid, char *friendly_name_buf, size_t friendly_name_buf_size)
{
    int i = 0;
    for (; i < MAX_PAIRED_DEVICES; i++) {
        if (memcmp(paird.bd[i].bda, bda, sizeof (esp_bd_addr_t)) == 0) {
            break;
        }
    }

    if (i >= MAX_PAIRED_DEVICES) {
        ESP_LOGE(TAG, "No device with matching bda found in paired device list");
        return ESP_FAIL;
    }

    if (i == 0) {
        ESP_LOGD(TAG, "Disconnecting...");
        bluetooth_disconnect_device();
    } else {
        ESP_LOGD(TAG, "Removing pairing info of non-connected device");
    }
    memcpy(uuid, paird.bd[i].uuid, MAX_UUID_SIZE);

    strlcpy(friendly_name_buf, paird.bd[i].friendly_name, friendly_name_buf_size);
    esp_bt_gap_remove_bond_device(paird.bd[i].bda);
    bluetooth_remove_pairing(i);
    return ESP_OK;
}

esp_err_t bluetooth_unpair_device_by_id(const char *uuid, char *friendly_name_buf, size_t friendly_name_buf_size)
{
    int i = 0;
    for (; i < MAX_PAIRED_DEVICES; i++) {
        if (memcmp(paird.bd[i].uuid, uuid, MAX_UUID_SIZE) == 0) {
            break;
        }
    }

    if (i >= MAX_PAIRED_DEVICES) {
        ESP_LOGE(TAG, "No device with matching uuid found in paired device list: %s", uuid);
        return ESP_FAIL;
    }

    if (i == 0) {
        ESP_LOGD(TAG, "Disconnecting...");
        bluetooth_disconnect_device();
    } else {
        ESP_LOGD(TAG, "Removing pairing info of non-connected device");
    }

    strlcpy(friendly_name_buf, paird.bd[i].friendly_name, friendly_name_buf_size);
    esp_bt_gap_remove_bond_device(paird.bd[i].bda);
    bluetooth_remove_pairing(i);
    return ESP_OK;
}

esp_err_t bluetooth_scan_start(bool connect_on_found)
{
    if (!scand.discovering) {
        ESP_LOGD(TAG, "Starting scan ...");
        scand.count = 0;
        scand.connect_on_found = connect_on_found;
        /* TODO need to check if bluetooth has been started before this */
        return esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, ESP_BT_GAP_MAX_INQ_LEN/2, 0);
    }
    else
        return ESP_OK;
}

esp_err_t bluetooth_scan_stop()
{
    ESP_LOGD(TAG, "Stopping discovery ...");
    esp_err_t ret = esp_bt_gap_cancel_discovery();
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "... Done");
    } else {
        ESP_LOGE(TAG, "Failed to cancel discovery");
    }
    /* Remove all scand entries */
    return ret;
}

static esp_err_t bluetooth_scan_add(char *peer_bdname, uint8_t peer_bdname_len,
                                    esp_bd_addr_t peer_bd_addr, enum supported_profiles peer_profile)
{
    bool exists = false;

    /* Check if it exits in paired device list */
    for (int i = 0; i < paird.count; i++) {
        if (memcmp(paird.bd[i].bda, (uint8_t *)peer_bd_addr, ESP_BD_ADDR_LEN) == 0) {
            exists = true;
            if (scand.connect_on_found) {
                paird.connect_status = BT_CONNECTING;
                bluetooth_scan_stop();
                /* If already exists, check profile and connect. */
                if (paird.bd[i].profiles & (1 << A2DP_SINK)) {
                    bluetooth_connect(paird.bd[i].bda, A2DP_SINK);
                } else {
                    bluetooth_connect(paird.bd[i].bda, A2DP_SOURCE);
                }
                memcpy(&paird.connecting_bd, &paird.bd[i], sizeof(struct bluetooth_dev_info));
                ESP_LOGI(TAG, "%s : Connecting to %s", TAG, paird.bd[i].friendly_name);
            }
            break;
        }
    }

    if (scand.connect_on_found)
        return ESP_OK;

    if (!exists) {
        /* Check if already exist in scan list */
        for (int i = 0 ; i < scand.count ; i++) {
            if (memcmp(scand.bd[i].bda, (uint8_t *)peer_bd_addr, ESP_BD_ADDR_LEN) == 0) {
                exists = true;
                if (peer_bdname && strncmp(scand.bd[i].friendly_name, peer_bdname, MAX_FRIENDLY_NAME_LEN) != 0) {
                    ESP_LOGD(TAG, "Friendly name for an existing device updated");
                    strlcpy(scand.bd[i].friendly_name, peer_bdname, MAX_FRIENDLY_NAME_LEN);
                    if (bt.event_handler) {
                        bt.event_handler(EVENT_SCAN_DEVICES_UPDATED, NULL);
                    }
                }
                break;
            }
        }
    }

    if (!exists) {
        ESP_LOGD(TAG, "Adding to scan list --");
        if (scand.count >= MAX_SCAN_DEVICES) {
            ESP_LOGE(TAG, "Reached max scan device count");
            return ESP_OK;
        }

        char bdname[MAX_FRIENDLY_NAME_LEN] = {0};

        if (!peer_bdname) {
            int i;
            /* XXX: Check if we already have a name from last session */
            for (i = 0; i < MAX_SCAN_DEVICES; i++) {
                if (memcmp(scand.bd[i].bda, (uint8_t *)peer_bd_addr, ESP_BD_ADDR_LEN) == 0) {
                    strlcpy(bdname, scand.bd[i].friendly_name, MAX_FRIENDLY_NAME_LEN);
                    break;
                }
            }

            if (i >= MAX_SCAN_DEVICES)
                snprintf(bdname, MAX_FRIENDLY_NAME_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
                        peer_bd_addr[0], peer_bd_addr[1], peer_bd_addr[2],
                        peer_bd_addr[3], peer_bd_addr[4], peer_bd_addr[5]);
        } else {
            strlcpy(bdname, peer_bdname, MAX_FRIENDLY_NAME_LEN);
        }

        /* esp_log_buffer_char(TAG, peer_bdname , peer_bdname_len); */
        /* esp_log_buffer_hex(TAG, peer_bd_addr, ESP_BD_ADDR_LEN); */

        memcpy(scand.bd[scand.count].bda, (uint8_t *)peer_bd_addr, ESP_BD_ADDR_LEN);
        strlcpy(scand.bd[scand.count].friendly_name, bdname, MAX_FRIENDLY_NAME_LEN);
        scand.bd[scand.count].profiles = peer_profile;
        char *tmp_uuid = (char *) generateUUID();
        memcpy(scand.bd[scand.count].uuid, tmp_uuid, MAX_UUID_SIZE);
        free(tmp_uuid);

        scand.count++;
        for (int i = 0 ; i < scand.count ; i++) {
            esp_log_buffer_hex(TAG, scand.bd[i].bda, ESP_BD_ADDR_LEN + 1);
            ESP_LOGD(TAG, "%s", scand.bd[i].friendly_name);
        }

        if (bt.event_handler) {
            bt.event_handler(EVENT_SCAN_DEVICES_UPDATED, NULL);
        }
    }
    return ESP_OK;
}

int bluetooth_get_paired_device_count()
{
    return paird.count;
}

esp_err_t bluetooth_get_paired_device_info(int index, struct bluetooth_dev_info *dev)
{
    if (index < 0 || index >= paird.count) {
        //ESP_LOGE(TAG, "Invalid index: %d", index);
        return ESP_FAIL;
    }

    /* Device has been unpaired/removed */
    if (!paird.bd[index].uuid[0] || !paird.bd[index].friendly_name[0]) {
        return ESP_FAIL;
    }

    memcpy(dev->uuid, paird.bd[index].uuid, MAX_UUID_SIZE);
    strlcpy(dev->friendly_name, paird.bd[index].friendly_name, MAX_FRIENDLY_NAME_LEN);
    return ESP_OK;
}

const struct scanned_device_info *bluetooth_get_scan_list()
{
    return &scand;
}

sys_playback_requester_t *bluetooth_get_playback_requester()
{
    return &bt.playback_requester;
}

void bluetooth_set_device_name(const char *device_name)
{
    if (device_name) {
        strlcpy(bt.device_name, device_name, sizeof(bt.device_name));
    }
    esp_bt_dev_set_device_name(bt.device_name);
}

const char *bluetooth_get_device_name()
{
    return bt.device_name;
}
