idf_component_register(INCLUDE_DIRS "include")

set(requires_list misc esp_ringbuf nvs_flash streams mbedtls uuid-gen codecs sh2lib protobuf-c audio_hal multipart_parser console sys_playback basic_player bt audio_utils libsodium protocomm wpa_supplicant wifi_provisioning main)

if(VOICE_ASSISTANT STREQUAL "gva")
    list(APPEND requires_list gva-proto)
    add_prebuilt_library(voice_assistant ${COMPONENT_DIR}/lib/libgva.a REQUIRES ${requires_list})
elseif(VOICE_ASSISTANT STREQUAL "dialogflow")
    list(APPEND requires_list dialogflow-proto)
    add_prebuilt_library(voice_assistant ${COMPONENT_DIR}/lib/libdialogflow.a REQUIRES ${requires_list})
elseif(VOICE_ASSISTANT STREQUAL "aia")
    add_prebuilt_library(voice_assistant ${COMPONENT_DIR}/lib/libaia.a REQUIRES ${requires_list})
elseif(VOICE_ASSISTANT STREQUAL "avs")
    list(APPEND requires_list bluetooth)
    add_prebuilt_library(voice_assistant ${COMPONENT_DIR}/lib/libavs.a REQUIRES ${requires_list})
endif()

target_link_libraries(${COMPONENT_LIB} INTERFACE voice_assistant)
