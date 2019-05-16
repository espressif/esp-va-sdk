// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef __LEDC_LYRAT_H__
#define __LEDC_LYRAT_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/ledc.h"


#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (22)  //red
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO       (19)  //green
#define LEDC_HS_CH1_CHANNEL    LEDC_CHANNEL_1

#define LEDC_MAX_CH_NUM       (2)

esp_err_t ledc_lyrat_init();

#endif /* __LEDC_LYRAT_H__ */