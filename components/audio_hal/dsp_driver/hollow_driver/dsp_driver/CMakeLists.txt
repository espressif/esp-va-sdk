# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES spi_flash audio_hal dsp_firmware)

set(COMPONENT_SRCS ./va_dsp_hal.c)

register_component()
