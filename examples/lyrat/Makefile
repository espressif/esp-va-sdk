#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

ifeq ("$(VOICE_ASSISTANT)","gva")
PROJECT_NAME := gva
CPPFLAGS += -DGVA
else ifeq ("$(VOICE_ASSISTANT)","dialogflow")
PROJECT_NAME := dialogflow
CPPFLAGS += -DDIALOGFLOW
else
PROJECT_NAME := alexa
CPPFLAGS += -DALEXA
endif

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../../components 

include $(IDF_PATH)/make/project.mk
