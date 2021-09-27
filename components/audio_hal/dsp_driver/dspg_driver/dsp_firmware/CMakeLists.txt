idf_component_register(INCLUDE_DIRS )

add_prebuilt_library(dsp_firmware ${COMPONENT_DIR}/lib/libdsp_firmware.a REQUIRES )

target_link_libraries(${COMPONENT_LIB} INTERFACE dsp_firmware)
