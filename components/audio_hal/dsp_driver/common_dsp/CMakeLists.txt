idf_component_register(SRCS "common_dsp.c"
                    INCLUDE_DIRS .
                    REQUIRES spi_flash audio_hal speech_recog streams codecs)
