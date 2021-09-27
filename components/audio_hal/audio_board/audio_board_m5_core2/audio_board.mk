ifeq ($(AWS_EDUKIT_PATH),)
	$(error Please specify core2forAWS path via AWS_EDUKIT_PATH)
endif
EXTRA_COMPONENT_DIRS += $(AWS_EDUKIT_PATH)

AUDIO_HAL_PATH := $(AUDIO_BOARD_PATH)/../../

# Board
EXTRA_COMPONENT_DIRS += $(AUDIO_BOARD_PATH)/audio_board

# DSP driver
export DSP_DRIVER_PATH = $(AUDIO_HAL_PATH)/dsp_driver/esp_dsp_driver
EXTRA_COMPONENT_DIRS += $(DSP_DRIVER_PATH)/
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/dsp_driver/common_dsp

# codec
export ESP_CODEC_PATH = no_codec
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/esp_codec

# LED driver
export LED_DRIVER_PATH = m5_core2
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_driver

# LED pattern
LED_PATTERN_TYPE ?= hollow
ifeq ($(LED_PATTERN_TYPE),hollow)
	export LED_PATTERN_PATH = no_pattern/$(LED_PATTERN_TYPE)
else
	export LED_PATTERN_PATH = linear_5/$(LED_PATTERN_TYPE)
endif
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_pattern

# Button driver
export BUTTON_DRIVER_PATH = m5_core2
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/button_driver

# Display driver
export DISPLAY_DRIVER_PATH = m5_core2
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/display_driver
