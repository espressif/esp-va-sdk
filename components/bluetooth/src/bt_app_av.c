// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2s.h"
#include <esp_timer.h>
#include "esp_log.h"

#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "bluetooth.h"
#include "bluetooth_priv.h"
#include <media_hal.h>

#define BT_AV_TAG    "BT_AV"

#if A2DP_SINK_VOLUME_REGISTRATION
static bool bt_app_av_volume_notify;
static bool bt_app_av_volume_change_by_controller;
#endif

static uint32_t m_pkt_cnt = 0;
static esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
// AVRCP used transaction label
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)


static int prev_avrc_key_code;
static int64_t prev_avrc_time;

rb_handle_t bt_sink_rb;
rb_handle_t bt_source_rb;
bt_event_handler_t event_handler;

/* Notify volume change. */
void bt_app_av_notify_vol_change(int volume)
{
#if A2DP_SINK_VOLUME_REGISTRATION
    /* Scale volume from [0, 100] to [0, 127] */
    volume = (volume * 0x7f) / 100;

    if (bt_app_av_volume_change_by_controller) {
        /* If this volume change is due to controller, don't send command. */
        bt_app_av_volume_change_by_controller = false;
    } else if (bt_app_av_volume_notify) {
        esp_avrc_rn_param_t rn_param;
        rn_param.volume = volume;
        esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
        bt_app_av_volume_notify = false;
    }
#endif
}

/* Callback for A2DP sink */
void bt_app_a2d_sink_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_A2D_CONNECTION_STATE_EVT w/ event %d", param->conn_stat.state);
        switch (param->conn_stat.state) {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            ESP_LOGI(BT_AV_TAG, "%s : BT device disconnected, reason: %s", __FUNCTION__,
                     param->conn_stat.disc_rsn ? "ABNORMAL" : "NORMAL");
            /* XXX: bluetooth_enqueue_event DISCONNECTED so that PAIRFAILED can be sent to AVS */
            bluetooth_on_disconnect_success();
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            ESP_LOGI(BT_AV_TAG, "%s : BT device connected", __FUNCTION__);
            bluetooth_on_connect_success(param->conn_stat.remote_bda);
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            ESP_LOGI(BT_AV_TAG, "%s : BT device connecting to %02X:%02X:%02X:%02X:%02X:%02X", __FUNCTION__,
                     param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1], param->conn_stat.remote_bda[2],
                     param->conn_stat.remote_bda[3], param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            ESP_LOGI(BT_AV_TAG, "Device disconnecting...");
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "Errorneous BT state %d. Device disconnected?", (int) param->conn_stat.state);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_A2D_AUDIO_STATE_EV w/ event %d", param->audio_stat.state);
        m_audio_state = param->audio_stat.state;
        switch (param->audio_stat.state) {
        case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND:
            if (event_handler) {
                event_handler(EVENT_BT_AUDIO_STREAM_STOPPED, NULL);
            }
            ESP_LOGD(BT_AV_TAG, "Audio suspended");
            break;
        case ESP_A2D_AUDIO_STATE_STOPPED:
            ESP_LOGD(BT_AV_TAG, "Audio stopped. Trying to restart audio datapath");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            break;
        case ESP_A2D_AUDIO_STATE_STARTED:
            ESP_LOGD(BT_AV_TAG, "Audio started");
            arb_reset(bt_source_rb);
            if (event_handler) {
                event_handler(EVENT_BT_AUDIO_STREAM_STARTED, NULL);
            }
            m_pkt_cnt = 0;
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "Error in Audio state");
        }
        break;
    case ESP_A2D_AUDIO_CFG_EVT: {
        ESP_LOGD(BT_AV_TAG, "a2dp audio_cfg_cb , codec type %d", param->audio_cfg.mcc.type);
        if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            int sample_rate = 16000;
            char oct0 = param->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                sample_rate = 48000;
            }

            ESP_LOGD(BT_AV_TAG, "configure audio player %x-%x-%x-%x",
                     param->audio_cfg.mcc.cie.sbc[0],
                     param->audio_cfg.mcc.cie.sbc[1],
                     param->audio_cfg.mcc.cie.sbc[2],
                     param->audio_cfg.mcc.cie.sbc[3]);
            ESP_LOGD(BT_AV_TAG, "audio player configured, samplerate=%d", sample_rate);

            if (event_handler) {
                event_handler(EVENT_BT_UPDATE_FREQ, (void *) &sample_rate);
            }
        }
    }
    break;
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        if (param->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY) {
            if (param->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {
                ESP_LOGI(BT_AV_TAG, "media control ready");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            } else {
                ESP_LOGI(BT_AV_TAG, "error starting media control");
            }
        }
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "a2dp invalid cb event: %d", event);
        break;
    }
}

/* Callback for A2DP SOURCE */
void bt_app_a2d_source_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_A2D_CONNECTION_STATE_EVT w/ event %d", param->conn_stat.state);
        switch (param->conn_stat.state) {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            ESP_LOGI(BT_AV_TAG, "%s : BT device disconnected, reason: %s", __FUNCTION__,
                     param->conn_stat.disc_rsn ? "ABNORMAL" : "NORMAL");
            /* XXX: bluetooth_enqueue_event DISCONNECTED so that PAIRFAILED can be sent to AVS */
            bluetooth_on_disconnect_success();
            arb_abort(bt_source_rb);

            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            ESP_LOGI(BT_AV_TAG, "%s : BT device connected", BT_AV_TAG);
            bluetooth_on_connect_success(param->conn_stat.remote_bda);
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            ESP_LOGI(BT_AV_TAG, "%s : BT device connecting to %02X:%02X:%02X:%02X:%02X:%02X", __FUNCTION__,
                     param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1], param->conn_stat.remote_bda[2],
                     param->conn_stat.remote_bda[3], param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            ESP_LOGI(BT_AV_TAG, "Device disconnecting...");
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "Errorneous BT state %d. Device disconnected?", (int) param->conn_stat.state);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_A2D_AUDIO_STATE_EV w/ event %d", param->audio_stat.state);
        m_audio_state = param->audio_stat.state;
        switch (param->audio_stat.state) {
        case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND:
            if (event_handler) {
                event_handler(EVENT_BT_AUDIO_STREAM_STOPPED, NULL);
            }
            ESP_LOGD(BT_AV_TAG, "Audio suspended");
            break;
        case ESP_A2D_AUDIO_STATE_STOPPED:
            ESP_LOGD(BT_AV_TAG, "Audio stopped. Trying to restart audio datapath");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            break;
        case ESP_A2D_AUDIO_STATE_STARTED:
            ESP_LOGD(BT_AV_TAG, "Audio started");
            arb_reset(bt_source_rb);
            if (event_handler) {
                event_handler(EVENT_BT_AUDIO_STREAM_STARTED, NULL);
            }
            m_pkt_cnt = 0;
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "Error in Audio state");
        }
        break;
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        if (param->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY) {
            if (param->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {
                ESP_LOGI(BT_AV_TAG, "media control ready");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            } else {
                ESP_LOGI(BT_AV_TAG, "error starting media control");
            }
        }
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "a2dp invalid cb event: %d", event);
        break;
    }
}

void bt_sink_rb_mark_aborted()
{
    arb_abort(bt_sink_rb);
}

void bt_sink_rb_reset()
{
    arb_reset(bt_sink_rb);
}

static esp_err_t bt_rb_write(const uint8_t *data, uint32_t len)
{
    /* Try for 10 ms and give up if full */
    arb_write(bt_sink_rb, (void *) data, len, pdMS_TO_TICKS(10));
    return ESP_OK;
}

void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    bt_rb_write(data, len);
    if (++m_pkt_cnt % 1000 == 0) {
        ESP_LOGW(BT_AV_TAG, "audio data pkt cnt %u", m_pkt_cnt);
    }
}

/* Defined in media_hal.c */
extern bool media_hal_is_codec_mute();

int bluetooth_source_play(const uint8_t *data, size_t len, unsigned int wait)
{
    if (media_hal_is_codec_mute()) {
        /* We know that it is OK to zero out this buffer. */
        bzero((void *) data, len);
    }
    return arb_write(bt_source_rb, (uint8_t *) data, len, wait);
}

void bt_app_source_start()
{
    esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
    esp_err_t err = esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
    ESP_LOGI(BT_AV_TAG, "esp_a2d_media_ctrl return val: %d", err);
}

int32_t bt_app_a2d_source_data_cb(uint8_t *data, int32_t len)
{
    //ESP_LOGW(BT_AV_TAG, "Received bt source data cb");
    if (len < 0) {
        ESP_LOGE(BT_AV_TAG, "Received len -1, flushing data...");
        arb_reset(bt_source_rb);
        return 0;
    }

    if (!data || len == 0) {
        ESP_LOGE(BT_AV_TAG, "Null data pointer or len = 0");
        return 0;
    }

    memset(data, 0, len);
    ssize_t data_read = arb_read(bt_source_rb, data, len, 0);
    if (data_read < 0) {
        ESP_LOGI(BT_AV_TAG, "No more data to read for BT");
        len = 0;
        arb_reset(bt_source_rb);
    }
    return len;
}

void bt_app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);
    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;

    rc->meta_rsp.attr_text = attr_text;
}

void bt_app_stop_media()
{
    esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);
}

static void bt_av_new_track(void)
{
    // request metadata
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE;
    esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, attr_mask);

    // register notification if peer support the event_id
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_TRACK_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, ESP_AVRC_RN_TRACK_CHANGE, 0);
    }
}

static void bt_av_playback_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

static void bt_av_play_pos_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_POS_CHANGED)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE, ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
    }
}

/**
 * Ring buffer to hold data received from A2DP_SOURCE
 * If Size = (32 * 1024), We hold about (32 * 1024 * 1000)/(44100 * 2 * 2) = 186 ms of data.
 * Beware! Increasing this buffer too much could add to delay and decreasing it could lead to stutters!
 */
#define BT_SINK_RB_SIZE (96 * 1024)
#define BT_SOURCE_RB_SIZE (50 * 1024)

rb_handle_t bt_app_av_get_rb()
{
    return bt_sink_rb;
}

int bt_av_init(bt_event_handler_t event_cb)
{
    abstract_rb_cfg_t arb_cfg = DEFAULT_RB_TYPE_BASIC_FUNC();
    bt_source_rb = arb_init("bt_source_rb", BT_SOURCE_RB_SIZE, arb_cfg);
    if (!bt_source_rb) {
        ESP_LOGE(BT_AV_TAG, "Error creating A2DP source buffer");
        return ESP_FAIL;
    }
    bt_sink_rb = arb_init("bt_sink_rb", BT_SINK_RB_SIZE, arb_cfg);
    if (!bt_sink_rb) {
        ESP_LOGE(BT_AV_TAG, "Error creating A2DP sink buffer");
        arb_deinit(bt_source_rb);
        return ESP_FAIL;
    }
    media_hal_register_volume_change_cb(media_hal_get_handle(), bt_app_av_notify_vol_change);
    event_handler = event_cb;
    return ESP_OK;
}

/* XXX: Maybe not required for A2DP source */

void bt_av_notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter)
{
    switch (event_id) {
    case ESP_AVRC_RN_TRACK_CHANGE:
        bt_av_new_track();
        break;
    case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
        ESP_LOGI(BT_AV_TAG, "Playback status changed: 0x%x", event_parameter->playback);
        bt_av_playback_changed();
        break;
    case ESP_AVRC_RN_PLAY_POS_CHANGED:
        ESP_LOGI(BT_AV_TAG, "Play position changed: %d-ms", event_parameter->play_pos);
        bt_av_play_pos_changed();
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "Event %d not handled in notifications", event_id);
        break;
    }
}

void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG,"Event in bt_app_rc_ct_cb - %d", event);
    switch (event) {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        bt_app_alloc_meta_buffer(param);
        ESP_LOGI(BT_AV_TAG, "avrc metadata rsp: attribute id 0x%x, %s", param->meta_rsp.attr_id, param->meta_rsp.attr_text);
        free(param->meta_rsp.attr_text);
        break;
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = param->conn_stat.remote_bda;
        ESP_LOGD(BT_AV_TAG, "avrc conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 param->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (param->conn_stat.connected) {
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        } else {
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        //bt_sink_rb_mark_aborted();
        ESP_LOGD(BT_AV_TAG, "avrc passthrough rsp: key_code 0x%x, key_state %d", param->psth_rsp.key_code, param->psth_rsp.key_state);
        break;
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        ESP_LOGI(BT_AV_TAG, "AVRC event notification: %d", param->change_ntf.event_id);
        /* Required only in case of A2DP-Sink */
        bt_av_notify_evt_handler(param->change_ntf.event_id, &param->change_ntf.event_parameter);
        break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        ESP_LOGD(BT_AV_TAG, "avrc remote features %x", param->rmt_feats.feat_mask);
        /* Need to save this to NVS and send this to AVS */
        ESP_LOGI(BT_AV_TAG, "XXX : Address of remote device -");
        esp_log_buffer_hex("XXX", param->rmt_feats.remote_bda, ESP_BD_ADDR_LEN);
        break;
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        ESP_LOGD(BT_AV_TAG,"Count %d Bitmask %04x",param->get_rn_caps_rsp.cap_count, param->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = param->get_rn_caps_rsp.evt_set.bits;
        bt_av_new_track();
        bt_av_playback_changed();
        bt_av_play_pos_changed();
        break;
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
        ESP_LOGI(BT_AV_TAG, "Set absolute volume rsp: volume %d", param->set_volume_rsp.volume);
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "avrc invalid cb event: %d", event);
        break;
    }
}

void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG,"Event in bt_app_rc_tg_cb - %d", event);
    switch (event) {
        case ESP_AVRC_TG_CONNECTION_STATE_EVT:
        case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
            ESP_LOGD(BT_AV_TAG, "AVRC remote features %x, CT features %x", param->rmt_feats.feat_mask, param->rmt_feats.ct_feat_flag);
            break;
        case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
            ESP_LOGD(BT_AV_TAG, "AVRC register event notification: %d, param: 0x%x", param->reg_ntf.event_id, param->reg_ntf.event_parameter);
#if A2DP_SINK_VOLUME_REGISTRATION
            if (param->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
                bt_app_av_volume_notify = true;
                esp_avrc_rn_param_t rn_param;
                media_hal_get_volume(media_hal_get_handle(), &rn_param.volume);
                rn_param.volume = (rn_param.volume * 0x7f) / 100;
                esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
            }
#endif
            break;
        case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
            {
                int volume_normalized = (param->set_abs_vol.volume * 100) / 0x7f;
                ESP_LOGI(BT_AV_TAG, "Volume changed by SOURCE to %d, line %d", volume_normalized, __LINE__);
#if A2DP_SINK_VOLUME_REGISTRATION
                bt_app_av_volume_change_by_controller = true;
                media_hal_control_volume(media_hal_get_handle(), volume_normalized);
                if (event_handler) {
                    event_handler(EVENT_BT_VOLUME_CHANGED, (void *) &volume_normalized);
                }
#endif
            }
            break;
        case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
            ESP_LOGI(BT_AV_TAG, "AVRC passthrough cmd: key_code 0x%x, key_state %d", param->psth_cmd.key_code, param->psth_cmd.key_state);
            if (param->psth_cmd.key_code == prev_avrc_key_code) {
                int64_t current_time = esp_timer_get_time();
                if (current_time - prev_avrc_time < (350 * 1000)) {
                    /* Do not execute the command if the same command is received within 350ms. */
                    ESP_LOGD(BT_AV_TAG, "Ignoring avrc key code: %lld, %lld", current_time, prev_avrc_time);
                    break;
                }
            }
            prev_avrc_key_code = param->psth_cmd.key_code;
            prev_avrc_time = esp_timer_get_time();

            switch (param->psth_cmd.key_code) {
                case ESP_AVRC_PT_CMD_VOL_UP:
                    break;
                case ESP_AVRC_PT_CMD_VOL_DOWN:
                    break;
                case ESP_AVRC_PT_CMD_MUTE:
                    break;
                case ESP_AVRC_PT_CMD_PLAY:
                    if (event_handler) {
                        event_handler(EVENT_BT_PLAY, NULL);
                    }
                    esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
                    break;
                case ESP_AVRC_PT_CMD_STOP:
                    break;
                case ESP_AVRC_PT_CMD_PAUSE:
                    if (event_handler) {
                        event_handler(EVENT_BT_PAUSE, NULL);
                    }
                    break;
                case ESP_AVRC_PT_CMD_FORWARD:
                    if (event_handler) {
                        event_handler(EVENT_BT_NEXT, NULL);
                    }
                    break;
                case ESP_AVRC_PT_CMD_BACKWARD:
                    if (event_handler) {
                        event_handler(EVENT_BT_PREVIOUS, NULL);
                    }
                    break;
                default:
                    ESP_LOGE(BT_AV_TAG, "Invalid keycode received");
            }
            break;
        default:
            ESP_LOGE(BT_AV_TAG, "Invalid AVRC event: %d", event);
            break;
    }
}
