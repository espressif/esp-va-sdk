idf_component_register(INCLUDE_DIRS "include")

add_prebuilt_library(esp-ssdp ${COMPONENT_DIR}/lib/libesp-ssdp.a REQUIRES esp_http_server audio_utils)

target_link_libraries(${COMPONENT_LIB} INTERFACE esp-ssdp)
