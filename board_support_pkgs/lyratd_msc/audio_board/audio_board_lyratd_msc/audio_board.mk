AUDIO_HAL_PATH := $(AUDIO_BOARD_PATH)/../../
IPC_DRV_PATH := $(AUDIO_HAL_PATH)/audio_board/audio_board_lyratd_msc

# Board
EXTRA_COMPONENT_DIRS += $(AUDIO_BOARD_PATH)/

# LED driver
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/led_driver/is31fl3216

# codec
EXTRA_COMPONENT_DIRS += $(AUDIO_HAL_PATH)/esp_codec/zl38063/components/

# va_dsp
include $(IPC_DRV_PATH)/va_dsp/va_dsp.mk

# This var sets partition file to partitions_4mb_flash.csv
# Please take a look at example makefile.
PARTITIONS_4MB_FLASH_CSV = 1
