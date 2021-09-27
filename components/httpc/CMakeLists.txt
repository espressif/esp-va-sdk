set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp-tls nghttp)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS ./httpc.c)

register_component()
