COMPONENT_EMBED_FILES := $(COMPONENT_PATH)/../../../../dspg_fw/dspg/d2_fw.bin $(COMPONENT_PATH)/../../../../dspg_fw/dspg/d4p_fw.bin $(COMPONENT_PATH)/../../../../dspg_fw/dspg/asrp_aec.bin

COMPONENT_ADD_INCLUDEDIRS := include

IPC_LIB_PATH := $(COMPONENT_PATH)/lib/libdspg-ipc.a

COMPONENT_ADD_LDFLAGS += $(IPC_LIB_PATH)

COMPONENT_ADD_LINKER_DEPS += $(IPC_LIB_PATH)
