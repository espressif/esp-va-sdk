set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES esp_codec media_hal audio_hal audio_utils)

set(COMPONENT_SRCS media_hal.c va_dsp.c)

register_component()

# This fixes the circular dependecies error seen in cmake. The linking error is seen in a few more components, but just adding it here fixes all of them.
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 4)
