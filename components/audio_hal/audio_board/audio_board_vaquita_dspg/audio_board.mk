AUDIO_HAL_PATH := $(AUDIO_BOARD_PATH)/../../

# Board
EXTRA_COMPONENT_DIRS += $(AUDIO_BOARD_PATH)/audio_board

# DSP driver
export DSP_DRIVER_PATH = $(AUDIO_HAL_PATH)/dsp_driver/dspg_driver
# USE_OTHER_DSP_DRIVER here configures the dsp_driver to use ESP_WWE instead of the WWE from the dsp_driver. The dsp_driver just passes the processed mic data to common_dsp. A different va_dsp_hal is used from the dsp_driver.
# export USE_OTHER_DSP_DRIVER = 1
EXTRA_COMPONENT_DIRS += $(DSP_DRIVER_PATH)/
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/dsp_driver/common_dsp

# codec
export ESP_CODEC_PATH = es8311
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/esp_codec

# LED driver
export LED_DRIVER_PATH = ws2812
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
export BUTTON_DRIVER_PATH = adc
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/button_driver
