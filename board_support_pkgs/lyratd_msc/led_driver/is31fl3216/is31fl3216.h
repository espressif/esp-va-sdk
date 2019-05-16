// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _IS31FL3216_H_
#define _IS31FL3216_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"
#include <voice_assistant.h>

//define led task stack size
#define UI_LED_TASK_STACK_SZ   (1024)

//define i2c config
#define IS31FL3216_CH_NUM_MAX 16
#define IS31FL3216_ADDRESS 0xE8   //I2C addr

#define IS31FL3216_REG_CONFIG      0x00   //configuration register
#define IS31FL3216_REG_LED_CTRL_H  0x01   //LED control register OUT9-OUT16 enable bit
#define IS31FL3216_REG_LED_CTRL_L  0x02   //LED control register OUT1-OUT8 enable bit
#define IS31FL3216_REG_LED_EFFECT  0x03   //set the output current and the audio gain
#define IS31FL3216_REG_CH_CONFIG   0x04   //set the operating mode of OUT9~OUT16
#define IS31FL3216_REG_GPIO_CONFIG 0x05   //set the direction mode of OUT9~OUT16 as the GPIO port
#define IS31FL3216_REG_OUTPUT      0x06   //set the logic level of OUT9~OUT16 as the output port 
#define IS31FL3216_REG_INPUT_CTRL  0x07   //set the interrupt function of OUT9~OUT16
#define IS31FL3216_REG_STATE       0x08   //store the state of OUT9~OUT16 as the input port(read only)
#define IS31FL3216_REG_ADC_RATE    0x09   //set the ADC sample rate of the input signal

typedef enum {
    IS31FL3216_PWR_NORMAL = 0,   //normal operation
    IS31FL3216_PWR_SHUTDOWN,     //software shutdown
    IS31FL3216_PWR_MAX,
} is31fl3216_set_opr_t;

typedef enum {
    IS31FL3216_MODE_PWM = 0,      //PWM control mode
    IS31FL3216_MODE_AUTO_FRAME,   //auto frame play mode
    IS31FL3216_MODE_FRAME,        //audio frame mode
    IS31FL3216_MODE_MAX,
} is31fl3216_set_mode_t;

typedef enum {
    IS31FL3216_CUR_MOD_REXT = 0,   //output current is set by register
    IS31FL3216_CUR_MODE_AUDIO,     //output current is modulated by audio signal
    IS31FL3216_CUR_MODE_MAX,
} is31fl3216_cur_modulation_t;

typedef enum {
    IS31FL3216_CUR_1_00 = 0,      /**< Output Current Selection */
    IS31FL3216_CUR_0_75,
    IS31FL3216_CUR_0_50,
    IS31FL3216_CUR_0_25,
    IS31FL3216_CUR_2_00,
    IS31FL3216_CUR_1_75,
    IS31FL3216_CUR_1_50,
    IS31FL3216_CUR_1_25,
    IS31FL3216_CUR_MAX,
} Is31fl3216CurValue;

typedef enum {
    IS31FL3216_CASCADE_MASTER = 0,  //chip in non-cascade mode
    IS31FL3216_CASCADE_SLAVE,       //chip in cascade mode  
} is31fl3216_cascade_t;

typedef enum {
    IS31FL3216_LED_OUTPUT = 0,  //OUT9-16 are LED  output
    IS31FL3216_GPIO_OUTPUT,     //OUT9-16 are GPIO output
} is31fl3216_channel_op_t;


typedef enum {
    IS31FL3216_AGS_0DB = 0,      /**< Audio Gain Selection */
    IS31FL3216_AGS_3DB,
    IS31FL3216_AGS_6DB,
    IS31FL3216_AGS_9DB,
    IS31FL3216_AGS_12DB,
    IS31FL3216_AGS_15DB,
    IS31FL3216_AGS_18DB,
    IS31FL3216_AGS_21DB,
    IS31FL3216_AGS_MAX,
} Is31fl3216AgsValue;

typedef enum {
    IS31FL3216_DELAY_32MS = 0,   //frame delay time for 32   millisec
    IS31FL3216_DELAY_64MS,       //frame delay time for 64   millisec
    IS31FL3216_DELAY_128MS,      //frame delay time for 128  millisec
    IS31FL3216_DELAY_256MS,      //frame delay time for 256  millisec
    IS31FL3216_DELAY_512MS,      //frame delay time for 512  millisec
    IS31FL3216_DELAY_1024MS,     //frame delay time for 1024 millisec
    IS31FL3216_DELAY_2048MS,     //frame delay time for 2048 millisec
    IS31FL3216_DELAY_4096MS,     //frame delay time for 4096 millisec
    IS31FL3216_DELAY_MAX,
} is31fl3216_frame_delay_t;

typedef enum {

    IS31FL3216_REG_PWM_16 = 0x10,       /* Set the PWM duty cycle data */
    IS31FL3216_REG_PWM_15,
    IS31FL3216_REG_PWM_14,
    IS31FL3216_REG_PWM_13,
    IS31FL3216_REG_PWM_12,
    IS31FL3216_REG_PWM_11,
    IS31FL3216_REG_PWM_10,
    IS31FL3216_REG_PWM_09,
    IS31FL3216_REG_PWM_08,
    IS31FL3216_REG_PWM_07,
    IS31FL3216_REG_PWM_06,
    IS31FL3216_REG_PWM_05,
    IS31FL3216_REG_PWM_04,
    IS31FL3216_REG_PWM_03,
    IS31FL3216_REG_PWM_02,
    IS31FL3216_REG_PWM_01,

    IS31FL3216_REG_FRAME1_CTRL = 0x20, /* Store the data of 8 frames */
    IS31FL3216_REG_FRAME1_PWM = 0x22,
    IS31FL3216_REG_FRAME2_CTRL = 0x32,
    IS31FL3216_REG_FRAME2_PWM = 0x34,
    IS31FL3216_REG_FRAME3_CTRL = 0x44,
    IS31FL3216_REG_FRAME3_PWM = 0x46,
    IS31FL3216_REG_FRAME4_CTRL = 0x56,
    IS31FL3216_REG_FRAME4_PWM = 0x58,
    IS31FL3216_REG_FRAME5_CTRL = 0x68,
    IS31FL3216_REG_FRAME5_PWM = 0x6A,
    IS31FL3216_REG_FRAME6_CTRL = 0x7A,
    IS31FL3216_REG_FRAME6_PWM = 0x7C,
    IS31FL3216_REG_FRAME7_CTRL = 0x8C,
    IS31FL3216_REG_FRAME7_PWM = 0x8E,
    IS31FL3216_REG_FRAME8_CTRL = 0x9E,
    IS31FL3216_REG_FRAME8_PWM = 0xA0,

    IS31FL3216_REG_UPDATE = 0xB0,       /* Load PWM Register data */
    IS31FL3216_REG_FRAME_DELAY = 0xB6,  /* Set the delay time between each frame */
    IS31FL3216_REG_FRAME_START = 0xB7,  /* Set the start frame in Auto Frame Play Mode */
    IS31FL3216_REG_MAX,
} Is31fl3216Reg;

typedef enum {
    IS31FL3216_CH_1  = 0x0001,      /**< channel by bit shit */
    IS31FL3216_CH_2  = 0x0002,
    IS31FL3216_CH_3  = 0x0004,
    IS31FL3216_CH_4  = 0x0008,
    IS31FL3216_CH_5  = 0x0010,
    IS31FL3216_CH_6  = 0x0020,
    IS31FL3216_CH_7  = 0x0040,
    IS31FL3216_CH_8  = 0x0080,
    IS31FL3216_CH_9  = 0x0100,
    IS31FL3216_CH_10 = 0x0200,
    IS31FL3216_CH_11 = 0x0400,
    IS31FL3216_CH_12 = 0x0800,
    IS31FL3216_CH_13 = 0x1000,
    IS31FL3216_CH_14 = 0x2000,
    IS31FL3216_CH_15 = 0x4000,
    IS31FL3216_CH_16 = 0x8000,
    IS31FL3216_CH_ALL = 0xFFFF,
} Is31PwmChannel;

typedef struct {
    is31fl3216_set_opr_t set_opr;
    is31fl3216_set_mode_t set_mode;
    is31fl3216_cur_modulation_t cur_mod;
    int num_of_channel;
    is31fl3216_channel_op_t channel_op;
} is31fl3216_config_t;

#endif /* _IS31FL3216_H_ */
