AUDIO_HAL_PATH := $(AUDIO_BOARD_PATH)/../../

# Board
EXTRA_COMPONENT_DIRS += $(AUDIO_BOARD_PATH)/audio_board

# DSP driver
export DSP_DRIVER_PATH = $(AUDIO_HAL_PATH)/dsp_driver/hollow_driver
EXTRA_COMPONENT_DIRS += $(DSP_DRIVER_PATH)/
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/dsp_driver/common_dsp

# codec
export ESP_CODEC_PATH = hollow_codec
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/esp_codec

# LED driver
export LED_DRIVER_PATH = hollow_led
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_driver

# LED pattern
export LED_PATTERN_PATH = no_pattern/hollow
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_pattern

# Button driver
export BUTTON_DRIVER_PATH = hollow_button
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/button_driver
