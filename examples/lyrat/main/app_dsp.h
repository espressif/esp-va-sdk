// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _APP_DSP_H_
#define _APP_DSP_H_

#include <voice_assistant_app_cb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dsp_called_states {
    VA_CAN_START = (VA_END_STATES + 1),
} dsp_called_states_t;

void app_dsp_init(void);

void app_dsp_send_recognize();

void app_dsp_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_DSP_H_ */
