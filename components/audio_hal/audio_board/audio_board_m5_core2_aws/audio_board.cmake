# Designed to be included from an IDF app's CMakeLists.txt file
#
cmake_minimum_required(VERSION 3.5)

if(NOT(DEFINED ENV{AWS_EDUKIT_PATH}))
    MESSAGE(STATUS "Please specify core2forAWS path via AWS_EDUKIT_PATH")
    return()
endif()

set(AUDIO_HAL_PATH $ENV{AUDIO_BOARD_PATH}/../../)
set(DSP_DRIVER_PATH "${AUDIO_HAL_PATH}/dsp_driver/esp_dsp_driver")

set(ESP_CODEC_PATH "no_codec" CACHE INTERNAL "")
set(BUTTON_DRIVER_PATH "m5_core2_aws" CACHE INTERNAL "")
set(DISPLAY_DRIVER_PATH "m5_core2_aws" CACHE INTERNAL "")
set(LED_DRIVER_PATH "m5_core2_aws" CACHE INTERNAL "")
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
    ${AUDIO_HAL_PATH}/display_driver
    ${AUDIO_HAL_PATH}/esp_codec
    $ENV{AWS_EDUKIT_PATH})

idf_build_set_property(ENABLE_AWS_EDUKIT 1)
