set(requires_list audio_hal)

idf_build_get_property(enable_aws_edukit ENABLE_AWS_EDUKIT)
if(${enable_aws_edukit})
    list(APPEND requires_list core2forAWS)
endif()

idf_component_register(SRCS "${DISPLAY_DRIVER_PATH}/display_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${DISPLAY_DRIVER_PATH}"
                    REQUIRES ${requires_list})
