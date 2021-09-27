// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <esp_err.h>
#include "sys_playback.h"
#include <esp_bt_defs.h>

#define MAX_UUID_SIZE       37
#define MAX_PAIRED_DEVICES  5
#define MAX_SCAN_DEVICES    15
#define MAX_FRIENDLY_NAME_LEN    50 //Although it should be ESP_BT_GAP_MAX_BDNAME_LEN

/* Enable at least one option */
#define ALEXA_BT_SINK 1 /* Enable A2DP-SINK */
//#define ALEXA_BT_SOURCE 1 /* Enable A2DP-SOURCE */

/* Comment line below to disable volume registration for SINK */
#define A2DP_SINK_VOLUME_REGISTRATION 1

/**
 * @brief Supported profiles of a BT device.
 * Currently only first three of them are used internally.
 * Rest features are assumed.
 */
enum supported_profiles {
    A2DP_NONE = -1,
    A2DP_SOURCE = 0,
    A2DP_SINK,
    AVRC_TARGET,
    AVRC,
    AVRC_CONTROLLER,
};

/**
 * @brief Current state of bluetooth state machine.
 */
enum bt_connection_states {
    BT_DISCONNECTED,
    BT_CONNECTING,
    BT_CONNECTED,
};

/**
 * @brief Info of a BT device.
 */
struct bluetooth_dev_info {
    esp_bd_addr_t bda;
    char uuid[MAX_UUID_SIZE];
    char friendly_name[MAX_FRIENDLY_NAME_LEN];
    uint8_t profiles; /* Supported profiles. */
};

struct paired_device_info {
    struct bluetooth_dev_info bd[MAX_PAIRED_DEVICES];
    struct bluetooth_dev_info connecting_bd;
    int count;
    enum bt_connection_states connect_status;
};

struct scanned_device_info {
    struct bluetooth_dev_info bd[MAX_SCAN_DEVICES];
    int count;
    bool discovering;
    bool connect_on_found;
};

/**
 * @brief These events will be sent to event handler
 */
typedef enum bluetooth_events {
    EVENT_SCAN_DEVICES_UPDATED = 0, /* Scan device list updated */
    EVENT_NO_SCAN_DEVICES,          /* There are no available devices */
    EVENT_BT_CONNECTED,             /* Device connected */
    EVENT_BT_DISCONNECTED,          /* Device disconnected */
    EVENT_BT_AUDIO_STREAM_STARTED,  /* Audio streaming started */
    EVENT_BT_AUDIO_STREAM_STOPPED,  /* Audio streaming stopped */
    EVENT_BT_UPDATE_FREQ,           /* Frequency changed event. Useful for when BT-SINK mode. */
    EVENT_BT_VOLUME_CHANGED,        /* Volume change done by remote device. */
    EVENT_BT_PLAY,                  /* Play command received. Music should be played */
    EVENT_BT_PAUSE,                 /* Pause command received */
    EVENT_BT_NEXT,                  /* Next command received */
    EVENT_BT_PREVIOUS,              /* Previous command received */
    EVENT_BT_GET_PAIRED_DEVICES,    /* Request for list of already paired devices. */
    EVENT_BT_REPAIR_FAILED,         /* Remote device has deleted pairing info and denied repair request */
    EVENT_BT_PAIRED_DEVICES_UPDATED /* List of paired devices updated */
} bluetooth_events_t;

/**
 * @brief   Event handler to be passed to `bluetooth_init`
 *
 * @note    Bluetooth will raise events specified in `bluetooth_events_t` with data.
 */
typedef void (*bt_event_handler_t) (bluetooth_events_t event, void *data);

/**
 *  @brief  Initialize bluetooth with event handler.
 */
esp_err_t bluetooth_init(bt_event_handler_t event_handler);

void bt_sink_rb_mark_aborted();
void bt_sink_rb_reset();

/**
 * @brief   Send data to bt source ringbuffer.
 *
 * @note    This data will be buffered and will be provided to bt-source's data callback.
 */
int bluetooth_source_play(const uint8_t *data, size_t len, unsigned int wait);

/**
 * @brief   Get profile of current connected device
 *
 * \return  profile of the connected device
 */
enum supported_profiles bluetooth_get_connected_profile();

/**
 * @brief   Get friendly name of the connected device.
 *
 * \return  pointer to friendly name
 */
char *bluetooth_get_connected_name();

/**
 * @brief   Get UUID of the connected device.
 *
 * \return  pointer to UUID
 */
char *bluetooth_get_connected_uuid();

/**
 * @brief   Get friendly name of the connecting device.
 *
 * \return  pointer to friendly name
 */
char *bluetooth_get_connecting_name();

/**
 * @brief   Get UUID of the connecting device.
 *
 * \return  pointer to UUID
 */
char *bluetooth_get_connecting_uuid();

/**
 * @brief   Start both discoverable and connectable mode
 */
esp_err_t bluetooth_enable_discoverable();

/**
 * @brief   Disable discoverable mode
 */
esp_err_t bluetooth_disable_discoverable();

/**
 * @brief   Starts just connectable mode
 */
esp_err_t bluetooth_enable_connectable();

/**
 * Media control APIs
 * Use these APIs when we are in SINK mode and want to control remote device.
 */
esp_err_t bluetooth_mediacontrol_stop();
esp_err_t bluetooth_mediacontrol_play();
esp_err_t bluetooth_mediacontrol_pause();
esp_err_t bluetooth_mediacontrol_previous();
esp_err_t bluetooth_mediacontrol_next();

esp_err_t bluetooth_volumecontrol_up();
esp_err_t bluetooth_volumecontrol_down();

/* pairing APIs */
esp_err_t bluetooth_pair_device(char *uuid);
esp_err_t bluetooth_unpair_device_by_id(const char *uuid, char *friendly_name_buf, size_t friendly_name_buf_size);
esp_err_t bluetooth_unpair_device_by_bda(const esp_bd_addr_t bda, char *uuid, char *friendly_name_buf, size_t friendly_name_buf_size);
esp_err_t bluetooth_get_all_paired(char *all_paired_devices);
esp_err_t bluetooth_retry_connection();

/* Scan APIs */
esp_err_t bluetooth_scan_start(bool connect_on_found);
esp_err_t bluetooth_scan_stop();

const struct scanned_device_info *bluetooth_get_scan_list();
esp_err_t bluetooth_get_paired_device_info(int index, struct bluetooth_dev_info *dev);

/**
 * @brief   Get number of paired devices
 */
int bluetooth_get_paired_device_count();

/**
 * @brief   Set bluetooth-device name.
 *
 * @note    Default is set to `DEFAULT_BT_DEVICE_NAME`
 */
void bluetooth_set_device_name(const char *device_name);

/**
 * @brief   Get bluetooth-device name.
 *
 * \return  String pinting to device name.
 */
const char *bluetooth_get_device_name();

/**
 * @brief   Connect a new device by bda.
 */
esp_err_t bluetooth_connect_device_by_bda(esp_bd_addr_t bda);

/**
 * @brief   Connect a new device by UUID.
 */
esp_err_t bluetooth_connect_device_by_id(char uuid[]);

/**
 * @brief   Connects device from pairing list by profile.
 *
 * @note    next parameter specifies whether to check for next device in the list of paired device (true), else
 *          start scanning from the start
 */
esp_err_t bluetooth_connect_device_by_profile(uint16_t profile, bool next);

/**
 * @brief   Stops streaming media if any and disconnects the connected device.
 */
esp_err_t bluetooth_disconnect_device();

/**
 * @brief   Get sys_playback requester. Useful for BT-SINK mode.
 */
sys_playback_requester_t *bluetooth_get_playback_requester();

/**
 * APIs to enable/disable host controller. These help to save some battery in battery operated devices.
 * Do nothing if Power Save mode is not turned on.
 */
void bluetooth_disable_host_ct();
int bluetooth_enable_host_ct();
