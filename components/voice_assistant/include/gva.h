// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _GVA_H_
#define _GVA_H_

#include "voice_assistant.h"

/** Device specific configuration */
typedef struct {
    /** The registered device model */
    char *device_model;
    /** A unique device id */
    char *device_id;
    /** The supported device language (default: "en-IN") */
    char *device_language;
} gva_device_config_t;

/** The GVA Configuration Structure */
typedef struct {
    /** Configurations for the device */
    gva_device_config_t device_config;
} gva_config_t;

int gva_init(gva_config_t *gva_cfg);

#endif /*_GVA_H_ */
