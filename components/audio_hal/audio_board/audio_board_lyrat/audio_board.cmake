# Designed to be included from an IDF app's CMakeLists.txt file
#
cmake_minimum_required(VERSION 3.5)

set(AUDIO_HAL_PATH $ENV{AUDIO_BOARD_PATH}/../../)
set(DSP_DRIVER_PATH "${AUDIO_HAL_PATH}/dsp_driver/esp_dsp_driver")

set(ESP_CODEC_PATH "es8388" CACHE INTERNAL "")
set(BUTTON_DRIVER_PATH "gpio" CACHE INTERNAL "")
set(LED_DRIVER_PATH "esp_ledc" CACHE INTERNAL "")
if(NOT(DEFINED LED_PATTERN_TYPE) OR LED_PATTERN_TYPE STREQUAL "hollow")
    set(LED_PATTERN_PATH "no_pattern/hollow" CACHE INTERNAL "")
else()
    set(LED_PATTERN_PATH "single/single_color/${LED_PATTERN_TYPE}" CACHE INTERNAL "")
endif()

list(APPEND EXTRA_COMPONENT_DIRS
    $ENV{AUDIO_BOARD_PATH}/audio_board
    ${DSP_DRIVER_PATH}/
    ${AUDIO_HAL_PATH}/dsp_driver/common_dsp
    ${AUDIO_HAL_PATH}/led_driver
    ${AUDIO_HAL_PATH}/led_pattern
    ${AUDIO_HAL_PATH}/button_driver
    ${AUDIO_HAL_PATH}/esp_codec)

idf_build_set_property(ENABLE_4MB_FLASH_PARTITION 1)

# Note: This board requires partition table 'partitions_4mb_flash.csv' to be set in menuconfig.
