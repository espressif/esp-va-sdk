// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_log.h>
#include <core2forAWS.h>
#include <lvgl/lvgl.h>

#include <display_driver.h>

#define TAG "display_driver_m5_core2_aws"

extern SemaphoreHandle_t xGuiSemaphore;

static void brightness_slider_event_cb(lv_obj_t * slider, lv_event_t event) {
    if(event == LV_EVENT_VALUE_CHANGED) {
        Core2ForAWS_Display_SetBrightness(lv_slider_get_value(slider));
    }
}

void display_function()
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    lv_obj_t * time_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_set_pos(time_label, 10, 5);
    lv_label_set_align(time_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(time_label, "******ALEXA APP*******");

    lv_obj_t * brightness_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(brightness_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 45);
    lv_label_set_text(brightness_label, "Screen brightness");

    lv_obj_t * brightness_slider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_set_width(brightness_slider, 130);
    lv_obj_align(brightness_slider, brightness_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_set_event_cb(brightness_slider, brightness_slider_event_cb);
    lv_slider_set_value(brightness_slider, 50, LV_ANIM_OFF);
    lv_slider_set_range(brightness_slider, 30, 100);

    xSemaphoreGive(xGuiSemaphore);
}

esp_err_t display_driver_init()
{
    /* M5_Core2_AWS specific Touch and display drivers are already initalized in Core2ForAWS_Init() */

    display_function();

    return ESP_OK;
}
