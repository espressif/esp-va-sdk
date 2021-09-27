set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_PRIV_REQUIRES audio_utils codecs)

set(COMPONENT_SRCS ./basic_recorder.c)

register_component()
