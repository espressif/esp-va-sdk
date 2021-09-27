set(COMPONENT_ADD_INCLUDEDIRS . include/)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES spi_flash audio_hal streams codecs common_dsp)

# USE_OTHER_DSP_DRIVER here configures the dsp_driver to use the va_dsp_hal from some other dsp_driver.
if(DEFINED ENV{USE_OTHER_DSP_DRIVER} OR DEFINED USE_OTHER_DSP_DRIVER)
    set(COMPONENT_SRCS ./esp_dsp.c)
else()
    set(COMPONENT_SRCS ./esp_dsp.c ./va_dsp_hal.c)
endif()

register_component()
