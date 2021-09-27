set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./app_auth.c ./app_main.c ./app_va_cb.c)

register_component()
