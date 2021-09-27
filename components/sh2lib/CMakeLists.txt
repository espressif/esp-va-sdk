set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp-tls)
set(COMPONENT_PRIV_REQUIRES nghttp)

set(COMPONENT_SRCS ./sh2lib.c)

register_component()
