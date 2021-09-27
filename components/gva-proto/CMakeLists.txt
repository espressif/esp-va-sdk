# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES protobuf-c)

register_component()

if (VOICE_ASSISTANT STREQUAL "gva")
    FILE(GLOB SRCS proto-c/*.c)
    target_sources(${COMPONENT_TARGET} INTERFACE ${SRCS})
    target_include_directories(${COMPONENT_TARGET} INTERFACE proto-c)
endif()
