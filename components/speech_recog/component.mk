#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := include

RECOG_LIB_PATH := $(COMPONENT_PATH)/lib/libc_speech_features.a $(COMPONENT_PATH)/lib/libdl_lib.a $(COMPONENT_PATH)/lib/libnn_model.a $(COMPONENT_PATH)/lib/libspeech_recog.a $(COMPONENT_PATH)/lib/libesp_wwe.a
COMPONENT_ADD_LDFLAGS += $(RECOG_LIB_PATH)
COMPONENT_ADD_LINKER_DEPS += $(RECOG_LIB_PATH)
