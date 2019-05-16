#
# Component Makefile
#

VOICE_ASSISTANT ?= alexa
COMPONENT_ADD_INCLUDEDIRS := include


ifeq ("$(VOICE_ASSISTANT)","gva")
VA_LIB_PATH := $(COMPONENT_PATH)/lib/libgva.a
else ifeq ("$(VOICE_ASSISTANT)","dialogflow")
VA_LIB_PATH := $(COMPONENT_PATH)/lib/libdialogflow.a
else
VA_LIB_PATH := $(COMPONENT_PATH)/lib/libalexa.a
endif

COMPONENT_ADD_LDFLAGS += $(VA_LIB_PATH)
COMPONENT_ADD_LINKER_DEPS += $(VA_LIB_PATH)
