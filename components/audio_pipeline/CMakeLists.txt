set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES streams codecs)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./audio_pipeline.c)

register_component()
