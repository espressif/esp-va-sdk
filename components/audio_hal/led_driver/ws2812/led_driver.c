/*
*
* Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <esp_log.h>
#include <driver/rmt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <led_driver.h>

//WS2812B
#define LED_STRIP_RMT_TICKS_BIT_1_HIGH_WS2812 34 // 400ns
#define LED_STRIP_RMT_TICKS_BIT_1_LOW_WS2812  16 // 850ns
#define LED_STRIP_RMT_TICKS_BIT_0_HIGH_WS2812 16 // 800ns
#define LED_STRIP_RMT_TICKS_BIT_0_LOW_WS2812  34 // 450ns

#define COLORS_RGB                             3
#define NO_OF_BITS_PER_COLOR                   8

static bool is_init_done = false;
static uint8_t *led_val;
static int chancnt;
static int ledcnt;
static int led_gpios;
static uint8_t led_count;

static rmt_item32_t *rmtdata;

static SemaphoreHandle_t mux;
static const char* TAG = "led_driver_ws2812";

int leds_init(int cnt, int gpio, int no)
{
    rmt_config_t rmt_cfg = {
        .rmt_mode = RMT_MODE_TX,
        .clk_div = 2, //10MHz pulse
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_freq_hz = 100, // Not used, but has to be set to avoid divide by 0 err
            .carrier_duty_percent = 50,
            .carrier_level = RMT_CARRIER_LEVEL_LOW,
            .carrier_en = false,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .idle_output_en = true,
        }
    };
    chancnt = no;

    ledcnt = cnt;
    led_gpios = gpio;

    rmt_cfg.channel = 0;
    rmt_cfg.gpio_num = led_gpios;
    esp_err_t cfg_ok = rmt_config(&rmt_cfg);
    if (cfg_ok != ESP_OK) {
        ESP_LOGE(TAG,"Can't configure RMT chan %d!\n", 0);
        return false;
    }
    esp_err_t install_ok = rmt_driver_install(0, 0, 0);
    if (install_ok != ESP_OK) {
        ESP_LOGE(TAG,"Can't install RMT ist for chan %d!\n", 0);
        return false;
    }
    /*
        malloc will allocate the number of bytes required for 
        (number of leds present on GPIO i.e ledcnt * No of colors i.e. R, G, B (3) * no of bits 
        per color i.e intensity in form of uint8_t (8) + 1 (RESET pulse) ) * rmt_item32_t structure)
    */
    rmtdata = malloc((ledcnt * COLORS_RGB * NO_OF_BITS_PER_COLOR + 1) * sizeof(rmt_item32_t));
    if (rmtdata == NULL) {
        ESP_LOGE(TAG,"Can't allocate data for %d leds on channel %d\n", ledcnt, 0);
        return false;
    }
    mux = xSemaphoreCreateMutex();
    if (mux == NULL) {
        ESP_LOGE(TAG,"Can't create semaphore\n");
        return false;
    }
    return true;
}

static const rmt_item32_t wsOne = {
    .duration0 = LED_STRIP_RMT_TICKS_BIT_1_HIGH_WS2812,
    .level0 = 1,
    .duration1 = LED_STRIP_RMT_TICKS_BIT_1_LOW_WS2812,
    .level1 = 0,
};

static const rmt_item32_t wsZero = {
    .duration0 = LED_STRIP_RMT_TICKS_BIT_0_HIGH_WS2812,
    .level0 = 1,
    .duration1 = LED_STRIP_RMT_TICKS_BIT_0_LOW_WS2812,
    .level1 = 0,
};

static const rmt_item32_t wsReset = {
    .duration0 = LED_STRIP_RMT_TICKS_BIT_0_HIGH_WS2812, //50uS
    .level0 = 1,
    .duration1 = 5000,
    .level1 = 0,
};

static void encByte(rmt_item32_t *rmtdata, uint8_t byte)
{
    int j = 0;
    for (int mask = 0x80; mask != 0; mask >>= 1) {
        if (byte & mask) {
            rmtdata[j++] = wsOne;
        } else {
            rmtdata[j++] = wsZero;
        }
    }
}

void leds_send(uint8_t *data)
{
    int i = 0;
    int j = 0;
    int n = 0;
    int chn = 0;
    xSemaphoreTake(mux, portMAX_DELAY);
    while (chn != chancnt) {

        encByte(&rmtdata[j], data[i + 1]); //G
        encByte(&rmtdata[j + 8], data[i]); //R            
        encByte(&rmtdata[j + 16], data[i + 2]); //B
        j += 8 * 3;
        i += 3;
        n++;
        if (n >= ledcnt) {
            rmtdata[j++] = wsReset;
            rmt_write_items(chn, rmtdata, j, false);
            chn++;
            n = 0;
            j = 0;
        }
    }
    for (i = 0; i < chancnt; i++) {
        rmt_wait_tx_done(i, portMAX_DELAY);
    }
    xSemaphoreGive(mux);
}

void init_led_colour()
{
    led_count = ledcnt;
    led_val = (uint8_t *)calloc((3 * led_count), sizeof(uint8_t));
    if (led_val == NULL) {
        ESP_LOGE(TAG,"memory allocation failed for calloc\n");
    }
}

void glow_led(uint8_t red, uint8_t green, uint8_t blue, uint8_t position)
{
    for (int j=0; j <led_count; j++) {

        if ((j+1) == position) {
            led_val[3 * j]       = red;
            led_val[(3 * j) + 1] = green;
            led_val[(3 * j) + 2] = blue;
        } else {
            led_val[3 * j]       = 0;
            led_val[(3 * j) + 1] = 0;
            led_val[(3 * j) + 2] = 0;
        }
    }
    leds_send(led_val);
}

esp_err_t ws2812_init(int led_cnt, int gpio_no)
{
    esp_err_t res;
    int no = 1;
    res = leds_init(led_cnt, gpio_no, no);
    if (!res) {
        ESP_LOGE(TAG,"ws2812 (neo pixel) led init failed");
        return res;
    }
    init_led_colour();
    return res;
}

void led_driver_set_value(const uint32_t *led_value)
{
    for (int i = 0; i < led_count; i++) {
        led_val[3 * i] = (led_value[i] & 0xff0000UL) >> 16;
        led_val[(3 * i) + 1] = (led_value[i] & 0x00ff00UL) >> 8;
        led_val[(3 * i) + 2] = (led_value[i] & 0x0000ffUL);
    }
    leds_send(led_val);
}

bool led_driver_is_init_done()
{
    return is_init_done;
}

esp_err_t led_driver_init(led_driver_config_t *led_driver_config)
{
    ESP_LOGI(TAG, "Initialising led driver");
    esp_err_t err = ws2812_init(led_driver_config->num_of_leds, led_driver_config->start_gpio_pin);

    is_init_done = true;
    return err;
}
