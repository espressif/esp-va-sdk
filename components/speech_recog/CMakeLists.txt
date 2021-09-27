idf_component_register(INCLUDE_DIRS "include")

target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" libesp_wwe.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" libspeech_recog.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" libc_speech_features.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" libdl_lib.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" libnn_model.a)
