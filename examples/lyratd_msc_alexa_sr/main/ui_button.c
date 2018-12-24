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

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/adc.h>
#include <nvs_flash.h>
#include <esp_adc_cal.h>
#include <media_hal.h>
#include <ui_button.h>
#include <app_dsp.h>

#define BUTTON_TAG "UI_BUTTON"

#define NUM_OF_BUTTON 6
#define BUTTON_ADC_WIDTH 4095
#define V_REF   1100
#define BUTTON_REF_RANGE 200
#define VOL_FACTOR 1
#define FACTORY_RST_DELAY 3000

#define MAX_BUTTON_EVENT_NUM    10

typedef struct {
    uint16_t but_low_val;      //multpile data to be written
    uint16_t but_high_val;   //number of data
} button_val_range_t;

#include "ui_button.h"
#include <va_mem_utils.h>
#include "speaker.h"
#include "app_dsp.h"
#include "avs_nvs_utils.h"
#include "ui_led.h"
#include "zl38063.h"

//#define EN_AUDIO_JACK
#define VOLUME_STEP 5

static const char *UI_BUTTON_TAG = "ui button";

typedef struct {
    bool en_timer;
    int curr_time;
    esp_timer_handle_t esp_timer_handler;
    esp_adc_cal_characteristics_t characteristics;
    TaskHandle_t ui_button_task_handle;
} ui_button_t;

static ui_button_t button_st = {false, 0, NULL, {0}, NULL};
static uint8_t current_vol;
static int set_volume = 0;

static void ui_button_reset_timer_cb()
{
    ESP_LOGI(UI_BUTTON_TAG, "Reset initiated, erasing nvs flash and rebooting");
    ui_led_set(LED_RESET);
    vTaskDelay(3333 / portTICK_RATE_MS);
    ui_led_set(LED_OFF);
    nvs_flash_erase();
    esp_restart();
}

static const button_val_range_t button_val_range[] = {
    {(BUTTON_SET_VAL -  BUTTON_REF_RANGE), (BUTTON_SET_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_PLAY_VAL - BUTTON_REF_RANGE), (BUTTON_PLAY_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_REC_VAL -  BUTTON_REF_RANGE), (BUTTON_REC_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_MODE_VAL - BUTTON_REF_RANGE), (BUTTON_MODE_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_VOL_LOW_VAL -  BUTTON_REF_RANGE), (BUTTON_VOL_LOW_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_VOL_HIGH_VAL - BUTTON_REF_RANGE), (BUTTON_VOL_HIGH_VAL + BUTTON_REF_RANGE)},
    {(BUTTON_IDLE_VAL - BUTTON_REF_RANGE), (BUTTON_IDLE_VAL + BUTTON_REF_RANGE)},
};

static bool button_range_cmp(uint16_t val, uint16_t high_val, uint16_t low_val)
{
    if(val < high_val && val > low_val)
        return true;
    else
        return false;
}

button_event_t button_get_press(int adc_val) 
{
    button_event_t button_evt;
    for(int i = 0; i <= NUM_OF_BUTTON; i++) {
        if(button_range_cmp(adc_val, button_val_range[i].but_high_val, button_val_range[i].but_low_val)) {
            button_evt = i;
            return  button_evt;
        }
    }
    return BUTTON_EVENT_IDLE;
}

esp_err_t button_evt_handler(button_event_t evt)
{
    if(evt == BUTTON_EVENT_SET) {
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_PLAY_PAUSE) {
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_REC) {
        app_dsp_send_recognize();
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_MODE) {
        esp_timer_start_once(button_st.esp_timer_handler, (FACTORY_RST_DELAY * 1000));
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_VOL_DOWN) {
        zl38063_get_volume(&current_vol);
        set_volume = current_vol - VOLUME_STEP;
        if (set_volume < VOLUME_STEP) {
            set_volume = 0;
            ESP_LOGI(UI_BUTTON_TAG, "volume_min");
        }
        zl38063_control_volume(set_volume);
        button_st.curr_time = esp_timer_get_time();
        button_st.en_timer = true;
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_VOL_UP) {
        zl38063_get_volume(&current_vol);
        set_volume = current_vol + VOLUME_STEP;
        if (set_volume > 100) {
            set_volume = 100;
            ESP_LOGI(UI_BUTTON_TAG, "volume_max");
        }
        zl38063_control_volume(set_volume);
        button_st.curr_time = esp_timer_get_time();
        button_st.en_timer = true;
        return ESP_OK;
    }
    if(evt == BUTTON_EVENT_IDLE) {
        esp_timer_stop(button_st.esp_timer_handler);
        return ESP_OK;
    }
    return ESP_OK;
}

void ui_button_task(void *arg)
{
    uint32_t voltage;
    uint32_t voltage_max;
    button_event_t curr_evt;
    esp_adc_cal_get_voltage(ADC1_BUTTON_CHANNEL, &button_st.characteristics, &voltage_max);
    while(1) {
        esp_adc_cal_get_voltage(ADC1_BUTTON_CHANNEL, &button_st.characteristics, &voltage);
        uint32_t voltg_val = (voltage * BUTTON_ADC_WIDTH) / voltage_max;
        curr_evt = button_get_press(voltg_val);
        button_evt_handler(curr_evt);
        if (((esp_timer_get_time() - button_st.curr_time) > 3000000) && button_st.en_timer) {
            speaker_notify_vol_changed(set_volume);
            button_st.en_timer = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static esp_err_t ui_button_adc_init()
{
    esp_err_t ret;
    ret =  adc1_config_width(ADC_WIDTH_BIT_12);
    ret |= adc1_config_channel_atten(ADC1_BUTTON_CHANNEL, ADC_ATTEN_DB_11);
    ret |= esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &button_st.characteristics);
    printf("ret: %d\n", ret);
    return ret;
}

esp_err_t ui_button_init()
{
    ui_button_adc_init();
    StackType_t *ui_button_task_stack = (StackType_t *)va_mem_alloc(UI_BUTTON_TASK_BUFFER_SZ, VA_MEM_EXTERNAL);
    static StaticTask_t ui_button_task_buf;
    button_st.ui_button_task_handle = xTaskCreateStatic(ui_button_task, "ui-button-thread", UI_BUTTON_TASK_BUFFER_SZ,
                                      NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, ui_button_task_stack, &ui_button_task_buf);
    if (button_st.ui_button_task_handle == NULL) {
        ESP_LOGE(UI_BUTTON_TAG, "Could not create button task");
        return ESP_FAIL;
    }
    esp_timer_init();
    esp_timer_create_args_t timer_arg = {
        .callback = ui_button_reset_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ui button reset timer",
    };
    if (esp_timer_create(&timer_arg, &button_st.esp_timer_handler) != ESP_OK) {
        ESP_LOGE(UI_BUTTON_TAG, "Failed to create esp timer for reset-to-factory button");
        return ESP_FAIL;
    }
    return ESP_OK;
}

