# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.0")

set(VOICE_ASSISTANT "avs" CACHE INTERNAL "")
set(LED_PATTERN_TYPE "alexa" CACHE INTERNAL "")

if(NOT(DEFINED ENV{IDF_PATH}))
    MESSAGE(STATUS "IDF_PATH not defined")
    return()
endif()

if(NOT(DEFINED ENV{AUDIO_BOARD_PATH}))
    MESSAGE(STATUS "AUDIO_BOARD_PATH not defined")
    return()
endif()

set(EXTRA_COMPONENT_DIRS ../additional_components ../common ../../components $ENV{AUDIO_BOARD_PATH}/../../../)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{AUDIO_BOARD_PATH}/audio_board.cmake)

if(DEFINED ENV{CLOUD_AGENT_PATH})
    idf_build_set_property(CLOUD_AGENT 1)
    list(APPEND EXTRA_COMPONENT_DIRS $ENV{CLOUD_AGENT_PATH}/components)
endif()

idf_build_set_property(COMPILE_OPTIONS "-DVOICE_ASSISTANT_AVS" APPEND)

if(("$ENV{ALEXA_BT}" STREQUAL "1") OR ("${ALEXA_BT}" STREQUAL "1"))
    set(SDKCONFIG_DEFAULTS sdkconfig.bt.defaults)
    idf_build_set_property(COMPILE_OPTIONS "-DALEXA_BT" APPEND)
endif()

idf_build_get_property(enable_4mb ENABLE_4MB_FLASH_PARTITION)
if(${enable_4mb})
    idf_build_set_property(COMPILE_OPTIONS "-DENABLE_4MB_FLASH_PARTITION" APPEND)
endif()

project(avs)

if (DEFINED ENABLE_DSP_SPIFFS_PARTITION)
    spiffs_create_partition_image(storage ${DSP_DRIVER_PATH}/dsp_firmware FLASH_IN_PROJECT)
endif()
