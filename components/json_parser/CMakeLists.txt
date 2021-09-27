set(COMPONENT_ADD_INCLUDEDIRS ./ ./jsmn/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./json_parser.c ./jsmn/src/jsmn-changed.c)

register_component()
