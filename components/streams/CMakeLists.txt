set(COMPONENT_ADD_INCLUDEDIRS fs_stream i2s_stream http_stream hollow_stream .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES audio_utils audio_hal media_hal)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS fs_stream/fs_stream.c i2s_stream/i2s_stream.c http_stream/http_hls.c http_stream/http_playback_stream.c http_stream/http_playlist.c http_stream/http_stream.c hollow_stream/hollow_stream.c ./audio_stream.c)

register_component()
