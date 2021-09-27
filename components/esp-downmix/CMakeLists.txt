idf_component_register(INCLUDE_DIRS "include")

add_prebuilt_library(esp-downmix ${COMPONENT_DIR}/lib/libesp-downmix.a)

target_link_libraries(${COMPONENT_LIB} INTERFACE esp-downmix)
