# Designed to be included from an IDF app's CMakeLists.txt file
#
cmake_minimum_required(VERSION 3.5)

set(AUDIO_HAL_PATH $ENV{AUDIO_BOARD_PATH}/../../)
set(DSP_DRIVER_PATH "${AUDIO_HAL_PATH}/dsp_driver/hollow_driver")

set(ESP_CODEC_PATH "hollow_codec" CACHE INTERNAL "")
set(BUTTON_DRIVER_PATH "hollow_button" CACHE INTERNAL "")
set(LED_DRIVER_PATH "hollow_led" CACHE INTERNAL "")
set(LED_PATTERN_PATH "no_pattern/hollow" CACHE INTERNAL "")

list(APPEND EXTRA_COMPONENT_DIRS
    $ENV{AUDIO_BOARD_PATH}/audio_board
    ${DSP_DRIVER_PATH}/
    ${AUDIO_HAL_PATH}/dsp_driver/common_dsp
    ${AUDIO_HAL_PATH}/led_driver
    ${AUDIO_HAL_PATH}/led_pattern
    ${AUDIO_HAL_PATH}/button_driver
    ${AUDIO_HAL_PATH}/esp_codec)
