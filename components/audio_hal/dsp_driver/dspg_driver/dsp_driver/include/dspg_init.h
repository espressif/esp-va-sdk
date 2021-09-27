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

#ifndef DSPG_INIT_H

#define DSPG_INIT_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


///////////////////////////////////////////////////////

typedef enum {
    TOTAL_2_MICS = 2,
    TOTAL_3_MICS = 3

} total_mics_num_t;

// ****** VERY IMPORTANT *******
// In 3-MIC array system (e.g. ESP32-LyraTD 3-Mic ev-kit),  TOTAL_NUM_OF_MICS  = TOTAL_3_MICS	
// In 2-MIC array system (e.g. ESP32-Vaquita 2-Mic ev-kit),  TOTAL_NUM_OF_MICS  = TOTAL_2_MICS	
#define TOTAL_NUM_OF_MICS TOTAL_3_MICS			//TOTAL_2_MICS  or TOTAL_3_MICS

// ****** VERY IMPORTANT *******
//In the current ESP32-D5p ev-kits, only 1 speaker is supported.
#define TOTAL_NUM_OF_SPKS  1 						// 1 or 2


// ****** VERY IMPORTANT *******
// To support low-power-mode, need HW rework to run vt-core clock in 32K768Hz
// The SW change for `vt_mclk` is handled by the `enable_low_power` configuration in `dbmd5_firmware_conf_t`
// When using low power, make sure the HW rework has been done


#define ADD_MULTI_TURN_FIX				//  multi-turn fix work with D8x_FW_4611
#define PRELIM_TEST_FOR_AQT  			// SW fix in AQT

///////////////////////////////////////////////////////


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ADD_CONFIG_CLOCKS
#define SWITCH_TO_TDM_CLK_TRUE 		TRUE
#define SWITCH_TO_TDM_CLK_FALSE		FALSE

#define SWITCH_TO_MCLK_TRUE 			TRUE
#define SWITCH_TO_MCLK_FALSE 			FALSE

#define WANTED_PLL_0					0	
#define WANTED_AHB_CLK_0				0
#define WANTED_APB_CLK_0				0
#define USE_PLL_POST_DIV_FALSE			FALSE

#define MIC_TYPE_NONE		0
#define MIC_TYPE_ANALOG	1
#define MIC_TYPE_DIGITAL	2	
#define MIC_TYPE_VESPER	3


#define WWE_NONE			0
#define WWE_AMAZON		1
#define WWE_GOOGLE			2
#define WWE_SENSORY		3
#define WWE_DUAL			8


///////////////////////////////////////////////////////




typedef struct {
    int8_t apr_rst_pin;
    int8_t vt_rst_pin;
    int8_t apr_wakeup_pin;
    int8_t vt_intr_pin;
    int8_t pa_pin;
    int8_t level_shift;
    int8_t spi_mosi_pin;
    int8_t spi_miso_pin;
    int8_t spi_clk_pin;
    int8_t vt_cs_pin;
    int8_t apr_cs_pin;
} dbmd5_pin_conf_t;

typedef struct {
    total_mics_num_t total_mics_num;
    int8_t total_spks_num;
    bool enable_low_power;
    uint32_t grace_period;
} dbmd5_firmware_conf_t;

typedef struct {
    dbmd5_pin_conf_t pin_conf;
    dbmd5_firmware_conf_t fw_conf;
} dbmd5_config_t;

void dbmd5_reset();

void dbmd5_configure(dbmd5_pin_conf_t *pin_conf, dbmd5_firmware_conf_t *fw_conf);

void dbmd5_get_config(dbmd5_pin_conf_t *out_pin_conf, dbmd5_firmware_conf_t *out_fw_conf);

void dbmd5_init(QueueHandle_t queue);

void dbmd5_low_power_mode();

void dspg_low_power_mode_enter();
void dspg_low_power_mode_exit();
void dspg_aec_mode_enter();
void dspg_aec_mode_exit();

void dbmd5_aec_mode();

void dbmd5_mic_mute();

void dbmd5_mic_unmute();

void dbmd5_tap_to_talk();

void dbmd5_start_capture();

void dbmd5_stop_capture();

int dbmd5_stream_audio(uint8_t *buf, size_t max_len);

size_t dbmd5_get_ww_len();

void dbmd5_production_mode_init(QueueHandle_t queue);

void dbmd5_production_mode_enter();

void dbmd5_production_mode_exit();

#endif /* DSPG_INIT_H */
