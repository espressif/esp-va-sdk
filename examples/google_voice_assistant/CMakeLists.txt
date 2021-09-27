# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.0")

set(VOICE_ASSISTANT "gva" CACHE INTERNAL "")
set(LED_PATTERN_TYPE "google" CACHE INTERNAL "")

if(NOT(DEFINED ENV{IDF_PATH}))
    MESSAGE(STATUS "IDF_PATH not defined")
    return()
endif()

if(NOT(DEFINED ENV{AUDIO_BOARD_PATH}))
    MESSAGE(STATUS "AUDIO_BOARD_PATH not defined")
    return()
endif()

set(EXTRA_COMPONENT_DIRS ../../components $ENV{AUDIO_BOARD_PATH}/../../../)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{AUDIO_BOARD_PATH}/audio_board.cmake)

idf_build_get_property(enable_4mb ENABLE_4MB_FLASH_PARTITION)
if(${enable_4mb})
    idf_build_set_property(COMPILE_OPTIONS "-DENABLE_4MB_FLASH_PARTITION" APPEND)
endif()

project(gva)
