#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ifdef CONFIG_ALEXA_ENABLE_EQUALIZER
COMPONENT_ADD_INCLUDEDIRS += alexa_equalizer/
COMPONENT_SRCDIRS += alexa_equalizer/
endif

ifdef CONFIG_ALEXA_ENABLE_LOCAL_PLAYER
COMPONENT_ADD_INCLUDEDIRS += local_player/
COMPONENT_SRCDIRS += local_player/
endif

ifdef CONFIG_ALEXA_ENABLE_AWS_IOT
COMPONENT_ADD_INCLUDEDIRS += va_aws_iot/
COMPONENT_SRCDIRS += va_aws_iot/
COMPONENT_EMBED_TXTFILES := va_aws_iot/certs/aws-root-ca.pem va_aws_iot/certs/certificate.pem.crt va_aws_iot/certs/private.pem.key
endif
