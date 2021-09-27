set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_http_server)
set(COMPONENT_PRIV_REQUIRES esp-tls spi_flash app_update)

set(COMPONENT_SRCS src/esp_httpd_ota.c)

register_component()
