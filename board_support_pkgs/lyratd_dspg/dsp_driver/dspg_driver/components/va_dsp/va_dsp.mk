#set relevent flags

ifneq ($(ESP_SR_PATH),)
    EXTRA_COMPONENT_DIRS += $(ESP_SR_PATH)/components
endif
