#
# Component Makefile
#

VOICE_ASSISTANT ?= alexa

ifeq ("$(VOICE_ASSISTANT)","dialogflow")
COMPONENT_ADD_INCLUDEDIRS += proto-c
COMPONENT_SRCDIRS += proto-c
endif
