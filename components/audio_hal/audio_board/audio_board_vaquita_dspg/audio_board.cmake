# Designed to be included from an IDF app's CMakeLists.txt file
#
cmake_minimum_required(VERSION 3.5)

set(AUDIO_HAL_PATH $ENV{AUDIO_BOARD_PATH}/../../)
set(DSP_DRIVER_PATH "${AUDIO_HAL_PATH}/dsp_driver/dspg_driver")
# USE_OTHER_DSP_DRIVER here configures the dsp_driver to use ESP_WWE instead of the WWE from the dsp_driver. The dsp_driver just passes the processed mic data to common_dsp. A different va_dsp_hal is used from the dsp_driver.
# set(USE_OTHER_DSP_DRIVER 1)

set(ESP_CODEC_PATH "es8311" CACHE INTERNAL "")
set(BUTTON_DRIVER_PATH "adc" CACHE INTERNAL "")
set(LED_DRIVER_PATH "ws2812" CACHE INTERNAL "")
if(NOT(DEFINED LED_PATTERN_TYPE) OR LED_PATTERN_TYPE STREQUAL "hollow")
    set(LED_PATTERN_PATH "no_pattern/hollow" CACHE INTERNAL "")
else()
    set(LED_PATTERN_PATH "linear_5/${LED_PATTERN_TYPE}" CACHE INTERNAL "")
endif()

list(APPEND EXTRA_COMPONENT_DIRS
    $ENV{AUDIO_BOARD_PATH}/audio_board
    ${DSP_DRIVER_PATH}/
    ${AUDIO_HAL_PATH}/dsp_driver/common_dsp
    ${AUDIO_HAL_PATH}/led_driver
    ${AUDIO_HAL_PATH}/led_pattern
    ${AUDIO_HAL_PATH}/button_driver
    ${AUDIO_HAL_PATH}/esp_codec)
