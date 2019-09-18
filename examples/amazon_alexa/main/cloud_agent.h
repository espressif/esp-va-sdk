// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

/** Cloud Agent
This file can be used as a reference for implementing the OTA cloud agent.
*/

#if 0

/** Events which should be sent in the callback. */
typedef enum cloud_agent_event {
    /* Init done should be sent when the cloud agent is connected to the cloud and is checking for an update. */
    CLOUD_AGENT_INIT_DONE,
    /* OTA start should be sent when the cloud agent has found an update and is about to start the update. */
    CLOUD_AGENT_OTA_START,
    /* OTA end should be sent when the cloud agent has completed the update and is about to reboot the device. */
    CLOUD_AGENT_OTA_END,
} cloud_agent_event_t;

/** Device details that might be useful for the cloud agent. */
typedef struct cloud_agent_device_cfg {
    /* Name of the device */
    char *name;
    /* Type of the device */
    char *type;
    /* Model of the device */
    char *model;
    /* Firmware version that the device is running */
    char *fw_version;
} cloud_agent_device_cfg_t;

/** OTA Start
This API needs to be implemented by the application.
It should initialize the connection with the cloud and start checking for an update.
The application should get a callback with CLOUD_AGENT_INIT_DONE after the initialisation has been done and the connection has been made. It should get a callback with CLOUD_AGENT_OTA_START if an update has been found and a callback with CLOUD_AGENT_OTA_END when the update is complete.
*/
void cloud_agent_ota_start(cloud_agent_device_cfg_t *cloud_agent_device_cfg, void (*cloud_agent_app_cb)(cloud_agent_event_t event));

/** OTA Stop
This API needs to be implemented by the application.
This API should stop checking for update from the cloud.
*/
void cloud_agent_ota_stop();

#endif
