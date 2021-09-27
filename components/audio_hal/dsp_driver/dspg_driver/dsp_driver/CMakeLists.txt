idf_component_register(INCLUDE_DIRS "include")

add_prebuilt_library(dsp_driver ${COMPONENT_DIR}/lib/libdsp_driver.a REQUIRES spi_flash audio_hal common_dsp dsp_firmware)

target_link_libraries(${COMPONENT_LIB} INTERFACE dsp_driver)
