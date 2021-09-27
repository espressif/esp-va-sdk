// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALEXA_SMART_HOME_H_
#define _ALEXA_SMART_HOME_H_

#include <esp_err.h>
#include <smart_home.h>

#define SMART_HOME_PARAM_POWER          "esp.param.power"
#define SMART_HOME_PARAM_TOGGLE         "esp.param.toggle"
#define SMART_HOME_PARAM_RANGE          "esp.param.range"
#define SMART_HOME_PARAM_MODE           "esp.param.mode"

#define SMART_HOME_UI_POWER             "esp.ui.power"
#define SMART_HOME_UI_TOGGLE            "esp.ui.toggle"
#define SMART_HOME_UI_RANGE             "esp.ui.range"
#define SMART_HOME_UI_MODE              "esp.ui.mode"

typedef enum smart_home_device_type {
    ACTIVITY_TRIGGER = 0,
    CAMERA,
    COMPUTER,
    CONTACT_SENSOR,
    DOOR,
    DOORBELL,
    EXTERIOR_BLIND,
    FAN,
    GAME_CONSOLE,
    GARAGE_DOOR,
    INTERIOR_BLIND,
    LAPTOP,
    LIGHT,
    MICROWAVE,
    MOBILE_PHONE,
    MOTION_SENSOR,
    MUSIC_SYSTEM,
    NETWORK_HARDWARE,
    OTHER,
    OVEN,
    PHONE,
    SCENE_TRIGGER,
    SCREEN,
    SECURITY_PANEL,
    SMARTLOCK,
    SMARTPLUG,
    SPEAKER,
    STREAMING_DEVICE,
    SWITCH,
    TABLET,
    TEMPRATURE_SENSOR,
    THERMOSTAT,
    TV,
    WEARABLE,
} smart_home_device_type_t;

typedef struct alexa_smart_home_device_info {
    char *device_serial_num;
    char *manufacturer_name;
    char *device_description;
    char *model_name;
    char *product_id;
} alexa_smart_home_device_info_t;

/** Get device type string
 *
 * This API returns the string equivalent of the device_type.
 */
const char *alexa_smart_home_get_device_type_str(smart_home_device_type_t device_type);

esp_err_t alexa_smart_home_device_add_info(smart_home_device_t *device, alexa_smart_home_device_info_t *device_info);

#endif /* _ALEXA_SMART_HOME_H_ */
