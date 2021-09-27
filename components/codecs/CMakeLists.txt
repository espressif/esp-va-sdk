idf_component_register(INCLUDE_DIRS "include")

add_prebuilt_library(codecs ${COMPONENT_DIR}/lib/libcodecs.a REQUIRES audio_utils fatfs)

target_link_libraries(${COMPONENT_LIB} INTERFACE codecs)
