#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := gva
PROJECT_VER := 1.0
export VOICE_ASSISTANT = gva
export LED_PATTERN_TYPE = google

ifeq ($(IDF_PATH),)
  $(error Please specify idf path via IDF_PATH)
endif

ifeq ($(AUDIO_BOARD_PATH),)
  $(error Please specify audio board path via AUDIO_BOARD_PATH)
endif

include $(AUDIO_BOARD_PATH)/audio_board.mk

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../../components $(AUDIO_BOARD_PATH)/../../../

ifdef ENABLE_4MB_FLASH_PARTITION
# Set partition to `partitions_4mb_flash.csv`
# This makes sure boards with smaller flash size of 4Mb work without issue.
# var `ENABLE_4MB_FLASH_PARTITION` is set in `audio_board.mk`
export PARTITION_TABLE_CSV_PATH:=$(CURDIR)/partitions_4mb_flash.csv
CPPFLAGS += -DENABLE_4MB_FLASH_PARTITION
endif

include $(IDF_PATH)/make/project.mk
