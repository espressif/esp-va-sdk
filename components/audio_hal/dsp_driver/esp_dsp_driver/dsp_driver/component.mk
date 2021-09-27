COMPONENT_ADD_INCLUDEDIRS := . include/

COMPONENT_SRCDIRS := . include/

# USE_OTHER_DSP_DRIVER here configures the dsp_driver to use the va_dsp_hal from some other dsp_driver.
ifneq ($(USE_OTHER_DSP_DRIVER),)
    COMPONENT_OBJEXCLUDE += va_dsp_hal.o
endif
