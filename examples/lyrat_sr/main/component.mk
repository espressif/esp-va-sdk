#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_SRCDIRS := .

ifdef CONFIG_AWS_IOT_SDK
COMPONENT_EMBED_TXTFILES := certs/aws-root-ca.pem certs/certificate.pem.crt certs/private.pem.key
endif
