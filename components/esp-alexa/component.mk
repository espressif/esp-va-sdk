#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := include

#LIBS := alexa

#COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/alexa/lib   \
                          $(addprefix -l,$(LIBS)) \

#ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/%/lib/lib%.a,$(LIBS))

ALEXA_LIB_PATH := $(COMPONENT_PATH)/lib/libalexa.a
COMPONENT_ADD_LDFLAGS += $(ALEXA_LIB_PATH)
COMPONENT_ADD_LINKER_DEPS += $(ALEXA_LIB_PATH)
