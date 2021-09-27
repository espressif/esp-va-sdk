/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _M3U8_PARSER_H_
#define _M3U8_PARSER_H_

#include <esp_err.h>
#include <httpc.h>
#include <http_playlist.h>

/**
 * Parse m3u8 playlist.
 * Inputs:
 *          1. h : http connection handle of already established connection.
 *          2. playlist: Playlist to insert to. If this is not provided, new playlist will be created.
 *          3. url     : url of the connection. This will be used when found urls in m3u8 are relative etc.
 *          4. offset_in_ms: Time offset to which we want to skip urls.
 * Return:
 *      m3u8 playlist. (Same as `playlist` provided or new playlist pointer if not)
 */
http_playlist_t *m3u8_parse(httpc_conn_t *h, http_playlist_t *playlist, const char *url, int *offset_in_ms);

#endif  /* _M3U8_PARSER_H_ */
