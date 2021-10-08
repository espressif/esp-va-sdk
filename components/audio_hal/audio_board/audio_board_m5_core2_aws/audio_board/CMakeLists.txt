set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_hal media_hal led_pattern led_driver button_driver dsp_driver display_driver)

set(COMPONENT_SRCS ./audio_board.c ./va_board.c)

register_component()
