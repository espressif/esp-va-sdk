/*
 *      Copyright 2018, Espressif Systems (Shanghai) Pte Ltd.
 *  All rights regarding this code and its modifications reserved.
 *
 * This code contains confidential information of Espressif Systems
 * (Shanghai) Pte Ltd. No licenses or other rights express or implied,
 * by estoppel or otherwise are granted herein.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UI_BUTTON_H_
#define _UI_BUTTON_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_adc_cal.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BUTTON_SET_VAL         372
#define BUTTON_PLAY_VAL       1016
#define BUTTON_REC_VAL        1657
#define BUTTON_MODE_VAL       2233
#define BUTTON_VOL_LOW_VAL    2815
#define BUTTON_VOL_HIGH_VAL   3570
#define BUTTON_IDLE_VAL       4000

#define BUTTON_TASK_PRIORITY 3
#define BUTTON_STACK_SIZE    2048

typedef enum {
    BUTTON_EVENT_SET = 0x00,
    BUTTON_EVENT_PLAY_PAUSE,
    BUTTON_EVENT_REC,
    BUTTON_EVENT_MODE,
    BUTTON_EVENT_VOL_DOWN,
    BUTTON_EVENT_VOL_UP,
    BUTTON_EVENT_IDLE,
} button_event_t;

typedef struct {
    uint8_t set;
    uint8_t mode;
    uint8_t play_pause;
    uint8_t volume_up;
    uint8_t volume_down;
} button_gpio_t;

#define UI_BUTTON_TASK_BUFFER_SZ   (8 * 1024)

#define V_REF   1100

/* ADC1 GPIO36 for button V+ / V- / X / BT Pair
 * ADC1 GPIO39 for HWID
 */
#define ADC1_BUTTON_CHANNEL (ADC1_CHANNEL_3)

#define ESP_INTR_FLAG_DEFAULT        0

/**
 * @brief  initialize button service
 */
esp_err_t ui_button_init();

#ifdef __cplusplus
}
#endif

#endif /* _UI_BUTTON_H_ */
