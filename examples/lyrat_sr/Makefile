#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

ifeq ("$(VOICE_ASSISTANT)","gva")
$(error GVA is not supported with lyrat_sr app. Please use lyrat app.)
else ifeq ("$(VOICE_ASSISTANT)","dialogflow")
$(error DialogFlow is not supported with lyrat_sr app. Please use lyrat app.)
else
PROJECT_NAME := alexa
CPPFLAGS += -DALEXA
endif

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../../components $(PROJECT_PATH)/../../components/speech_recog 

include $(IDF_PATH)/make/project.mk
