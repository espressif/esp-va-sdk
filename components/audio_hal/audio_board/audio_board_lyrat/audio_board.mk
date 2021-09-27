AUDIO_HAL_PATH := $(AUDIO_BOARD_PATH)/../../

# Board
EXTRA_COMPONENT_DIRS += $(AUDIO_BOARD_PATH)/audio_board

# DSP driver
export DSP_DRIVER_PATH = $(AUDIO_HAL_PATH)/dsp_driver/esp_dsp_driver
EXTRA_COMPONENT_DIRS += $(DSP_DRIVER_PATH)/
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/dsp_driver/common_dsp

# codec
export ESP_CODEC_PATH = es8388
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/esp_codec

# LED driver
export LED_DRIVER_PATH = esp_ledc
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_driver

# LED pattern
LED_PATTERN_TYPE ?= hollow
ifeq ($(LED_PATTERN_TYPE),hollow)
	export LED_PATTERN_PATH = no_pattern/$(LED_PATTERN_TYPE)
else
	export LED_PATTERN_PATH = single/single_color/$(LED_PATTERN_TYPE)
endif
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_pattern

# Button driver
export BUTTON_DRIVER_PATH = gpio
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/button_driver

# Note: This board requires partition table 'partitions_4mb_flash.csv' to be set in menuconfig. This sets the correct partition table in the example's makefile.
ENABLE_4MB_FLASH_PARTITION = 1
