set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES codecs sys_playback)
set(COMPONENT_PRIV_REQUIRES audio_utils)

set(COMPONENT_SRCS ./basic_player.c)

register_component()
