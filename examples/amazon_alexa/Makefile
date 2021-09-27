#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := avs
PROJECT_VER := 1.0
export VOICE_ASSISTANT = avs
export LED_PATTERN_TYPE = alexa
CPPFLAGS += -DVOICE_ASSISTANT_AVS

ifeq ($(IDF_PATH),)
  $(error Please specify idf path via IDF_PATH)
endif

ifeq ($(AUDIO_BOARD_PATH),)
  $(error Please specify audio board path via AUDIO_BOARD_PATH)
endif

include $(AUDIO_BOARD_PATH)/audio_board.mk

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../additional_components $(PROJECT_PATH)/../common $(PROJECT_PATH)/../../components $(AUDIO_BOARD_PATH)/../../../

ifneq ($(CLOUD_AGENT_PATH),)
  EXTRA_COMPONENT_DIRS += $(CLOUD_AGENT_PATH)/components
endif

ifdef ENABLE_DSP_SPIFFS_PARTITION
export PARTITION_TABLE_CSV_PATH:=$(CURDIR)/partitions_spiffs.csv
endif

ifdef ENABLE_4MB_FLASH_PARTITION
# Set partition to `partitions_4mb_flash.csv`
# This makes sure boards with smaller flash size of 4Mb work without issue.
# var `ENABLE_4MB_FLASH_PARTITION` is set in `audio_board.mk`
export PARTITION_TABLE_CSV_PATH:=$(CURDIR)/partitions_4mb_flash.csv
CPPFLAGS += -DENABLE_4MB_FLASH_PARTITION
endif

ifeq ($(ALEXA_BT),1)
  CPPFLAGS += -DALEXA_BT
  export SDKCONFIG_DEFAULTS = sdkconfig.bt.defaults
endif

include $(IDF_PATH)/make/project.mk

ifdef ENABLE_DSP_SPIFFS_PARTITION
SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
$(eval $(call spiffs_create_partition_image,storage,$(DSP_DRIVER_PATH)/firmware))
endif
