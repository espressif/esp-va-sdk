#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := dialogflow
export VOICE_ASSISTANT=dialogflow

ifeq ($(AUDIO_BOARD_PATH),)
  $(error Please specify audio board path via AUDIO_BOARD_PATH)
endif

include $(AUDIO_BOARD_PATH)/audio_board.mk

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../../components $(AUDIO_BOARD_PATH)/../../../

ifneq ($(SHF_PATH),)
    EXTRA_COMPONENT_DIRS += $(SHF_PATH)/components
endif

ifdef PARTITIONS_4MB_FLASH_CSV
# Set partition to `partitions_4mb_flash.csv`
# This makes sure boards with smaller flash size of 4Mb work without issue.
# var `PARTITIONS_4MB_FLASH_CSV` is set in `audio_board.mk`
export PARTITION_TABLE_CSV_PATH:=$(CURDIR)/partitions_4mb_flash.csv
endif

include $(IDF_PATH)/make/project.mk
