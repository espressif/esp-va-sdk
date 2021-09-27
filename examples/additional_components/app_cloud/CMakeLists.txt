set(requires_list console)

idf_build_get_property(cloud_agent CLOUD_AGENT)
if(${cloud_agent})
    list(APPEND requires_list esp_rainmaker)
endif()

idf_component_register(SRCS "app_cloud_rainmaker.c"
                INCLUDE_DIRS "."
                REQUIRES "${requires_list}")
