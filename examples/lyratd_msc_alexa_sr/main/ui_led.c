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

#include <va_mem_utils.h>
#include <ui_led.h>
#include <app_dsp.h>
#include "audio_board.h"

//#define EN_STACK_MEASUREMENT
#define UI_LED_QUEUE_LENGTH 3

static const char *UI_LED_TAG = "UI_LED";

typedef struct {
    bool ui_led_init_bool;
    TaskHandle_t ui_led_task_handle;
    SemaphoreHandle_t ui_led_mux;
    QueueHandle_t ui_led_queue;
} ui_led_t;

static ui_led_t led_st = {true, NULL, NULL, NULL,};


#define LED_TAG "LED"

#define IS31_ERROR_CHECK(con) if(!(con)) {ESP_LOGE(LED_TAG,"err line: %d", __LINE__); return ESP_FAIL;}
#define IS31_PARAM_CHECK(con) if(!(con)) {ESP_LOGE(LED_TAG,"Parameter error, "); return ESP_FAIL;}
#define IS31_CHECK_I2C_RES(res) if(ret == ESP_FAIL) {ESP_LOGE(LED_TAG, "Is31fl3216[%s]: FAIL\n", __FUNCTION__);} \
                                else if(ret == ESP_ERR_TIMEOUT) {ESP_LOGE(LED_TAG, "Is31fl3216[%s]: TIMEOUT\n", __FUNCTION__);}
#define LED_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(LED_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define IS31FL3216_WRITE_BIT    0x00
#define IS31FL3216_READ_BIT     0x01
#define IS31FL3216_ACK_CHECK_EN 1

#define I2C_MASTER_SCL_IO    23        /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    18        /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM     I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0  /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0  /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000   /*!< I2C master clock frequency */


typedef struct {
 //   i2c_bus_handle_t bus;
    uint16_t addr;
} Is31fl3216Dev;

/**
 * @brief Initialization function for i2c
 */
static esp_err_t ui_led_i2c_init(int i2c_master_port)
{
    int res;
    i2c_config_t pf_i2c_pin = {0};
    
    res = audio_board_i2c_pin_config(i2c_master_port, &pf_i2c_pin);

    pf_i2c_pin.mode = I2C_MODE_MASTER;  
    pf_i2c_pin.master.clk_speed = I2C_MASTER_FREQ_HZ;
    
    res |= i2c_param_config(i2c_master_port, &pf_i2c_pin);
    res |= i2c_driver_install(i2c_master_port, pf_i2c_pin.mode, 0, 0, 0);
    return res;
}

/**
 * @brief Write IS31 register
 *
 * @param slave_add : slave address
 * @param reg_add    : register address
 * @param data      : data to write
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static esp_err_t ui_led_write_reg(uint8_t slave_add, uint8_t reg_add, uint8_t data)
{
    int res = 0;
    xSemaphoreTake(led_st.ui_led_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slave_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(led_st.ui_led_mux);
    LED_ASSERT(res, "ui_led_write_reg error", -1);
    return res;
}

/**
 * @brief Read IS31 register
 *
 * @param reg_add    : register address
 *
 * @return
 *     - (-1)     Error
 *     - (0)      Success
 */
static esp_err_t ui_led_read_reg(uint8_t reg_add, uint8_t *p_data)
{
    uint8_t data;
    esp_err_t res;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res  = i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, IS31FL3216_ADDRESS, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, IS31FL3216_ADDRESS | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    LED_ASSERT(res, "ui_led_read_reg error", -1);
    *p_data = data;
    return res;
}

static esp_err_t update_led_pwm(uint8_t reg_addr, uint8_t val)
{
    esp_err_t res;
    int chk;
    int i;
    for(i = 0; i < (12 - val); i++) {
        chk = (250 - i*25);
        if(chk > 0)
        res = ui_led_write_reg(IS31FL3216_ADDRESS, (reg_addr + i + val), chk);
        else
        res = ui_led_write_reg(IS31FL3216_ADDRESS, (reg_addr + i + val), 0);
    }
    for(i =0; i < val; i++)
    {
        chk = (250 - ((12-val)*25) -25*i);
        if(chk > 0)
        res = ui_led_write_reg(IS31FL3216_ADDRESS, (reg_addr + i), chk);
        else
        res = ui_led_write_reg(IS31FL3216_ADDRESS, (reg_addr + i), 0);
    }
    res = ui_led_write_reg(IS31FL3216_ADDRESS, 0xb0, 0x00);
    return res;
}

static esp_err_t ui_led_solid(uint8_t intensity) 
{
    esp_err_t res;
    int i;
    res = ui_led_write_reg(IS31FL3216_ADDRESS, 0x00, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x01, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x02, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x03, 0x50);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x04, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x05, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x06, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x07, 0xff);
    for( i = 0; i<12; i++) {
        res |= ui_led_write_reg(IS31FL3216_ADDRESS, (0x14 + i), intensity);
    }
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0xb0, 0x00);
    return res;
}

static esp_err_t ui_led_ring()
{
    esp_err_t res;
    res  = ui_led_write_reg(IS31FL3216_ADDRESS, 0x00, 0x20);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x01, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x02, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x03, 0x50);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x04, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x05, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x06, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x07, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x20, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x21, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x32, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x33, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x44, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x45, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x56, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x57, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x68, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x69, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x7a, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x7b, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x8c, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x8d, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x9e, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0x9f, 0xff);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0xB6, 0x00);
    res |= ui_led_write_reg(IS31FL3216_ADDRESS, 0xB7, 0x00);

    update_led_pwm(0x26, 0);
    update_led_pwm(0x38, 0);
    update_led_pwm(0x4a, 2);
    update_led_pwm(0x5c, 4);
    update_led_pwm(0x6e, 6);
    update_led_pwm(0x80, 8);
    update_led_pwm(0x92, 10);
    update_led_pwm(0xa4, 10);
    return res;
}

esp_err_t ui_led_set(int alexa_states)
{
    if (alexa_states != ALEXA_CAN_START) {
        led_st.ui_led_init_bool = false;
    }
#ifdef EN_STACK_MEASUREMENT
    ESP_LOGI("UI_LED_TAG", "Free Task Stack is: %s %u\n\n\n", __func__, uxTaskGetStackHighWaterMark(led_st.ui_led_task_handle));
#endif
    if (xQueueSend(led_st.ui_led_queue, &alexa_states, 0) != pdTRUE) {
        ESP_LOGE(UI_LED_TAG, "Cannot send alexa state");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void ui_led_task(void *arg)
{
    int alexa_states;
    while (1) {
        if (xQueueReceive(led_st.ui_led_queue, &alexa_states, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(UI_LED_TAG, "Failed to receive from led queue");
        }
        switch (alexa_states) {
        case ALEXA_CAN_START :
            while (led_st.ui_led_init_bool == true) {
                ui_led_solid(100);
                vTaskDelay(333/portTICK_RATE_MS);
                ui_led_solid(0);
                vTaskDelay(333/portTICK_RATE_MS);
            }
            break;
        case ALEXA_IDLE :
            //Do nothing
            ui_led_solid(0);
            break;
        case ALEXA_LISTENING :
            ui_led_solid(100);
            break;
        case ALEXA_THINKING :
            ui_led_ring();
            break;
        case ALEXA_SPEAKING :
            //Do nothing
            ui_led_solid(0);
            break;
        case LED_OFF :
            ui_led_solid(0);
            break;
        default :
            break;
        }
    }
}

esp_err_t ui_led_init()
{
    esp_err_t ret;
    static StaticTask_t ui_led_buf;

    StackType_t *ui_led_task_stack = (StackType_t *)va_mem_alloc(UI_LED_TASK_STACK_SZ, VA_MEM_EXTERNAL);
    if (ui_led_task_stack == NULL) {
        ESP_LOGE(UI_LED_TAG, "Could not allocate mememory for ui led thread");
        return ESP_FAIL;
    }
    led_st.ui_led_mux = xSemaphoreCreateMutex();
    if (led_st.ui_led_mux == NULL) {
        ESP_LOGE(UI_LED_TAG, "Could not create mutex");
        return ESP_FAIL;
    }
    led_st.ui_led_queue = xQueueCreate(UI_LED_QUEUE_LENGTH, sizeof(uint8_t));
    if (led_st.ui_led_queue == NULL) {
        ESP_LOGE(UI_LED_TAG, "Could not create queue");
        return ESP_FAIL;
    }
    ret = ui_led_i2c_init(0);
    if (ret != ESP_OK) {
        ESP_LOGE(UI_LED_TAG, "Led driver init failed");
        return ESP_FAIL;
    }
    led_st.ui_led_task_handle = xTaskCreateStatic(ui_led_task, "ui-led-thread", UI_LED_TASK_STACK_SZ, NULL,
                                CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, ui_led_task_stack, &ui_led_buf);
    if (led_st.ui_led_task_handle == NULL) {
        ESP_LOGE(UI_LED_TAG, "Could not creake ui led task");
        return ESP_FAIL;
    }
    ui_led_set(ALEXA_CAN_START);
    return ret;
}
