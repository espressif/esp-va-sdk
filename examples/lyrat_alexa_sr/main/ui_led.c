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

#include <mem_utils.h>
#include <ui_led.h>
#include <app_dsp.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <alexa_app_cb.h>

static const char *UI_LED_TAG = "UI_LED";

esp_err_t ui_led_set(int alexa_state)
{
    if (alexa_state == ALEXA_LISTENING) {
        gpio_set_level(LED_GPIO_PIN, 1);
    } else if (alexa_state == LED_RESET){
        gpio_set_level(LED_BOOT_PIN, 1);
    } else if (alexa_state == LED_OFF) {
        gpio_set_level(LED_GPIO_PIN, 0);
        gpio_set_level(LED_BOOT_PIN, 0);
    } else {
        gpio_set_level(LED_GPIO_PIN, 0);
    }
    return ESP_OK;
}

esp_err_t ui_led_init()
{
    esp_err_t ret;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BIT(LED_GPIO_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    ret = gpio_config(&io_conf);
    gpio_set_level(LED_GPIO_PIN, 0);

    io_conf.pin_bit_mask = BIT(LED_BOOT_PIN);
    ret |= gpio_config(&io_conf);
    gpio_set_level(LED_BOOT_PIN, 0);
    ESP_LOGI(UI_LED_TAG, "LED initialized");
    return ret;
}
