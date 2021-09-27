set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES codecs audio_hal audio_utils)

set(COMPONENT_SRCS ./media_hal_playback.c)

register_component()
