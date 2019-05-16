// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <esp_log.h>
#include <http_parser.h>

#include "sh2lib.h"

static const char *TAG = "[sh2lib]";

#define DBG_FRAME_SEND 1

/*
 * The implementation of nghttp2_send_callback type. Here we write
 * |data| with size |length| to the network and return the number of
 * bytes actually written. See the documentation of
 * nghttp2_send_callback for the details.
 */
static ssize_t callback_send_inner(struct sh2lib_handle *hd, const uint8_t *data,
                                   size_t length)
{
    int rv = esp_tls_conn_write(hd->http2_tls, data, length);
    if (rv <= 0) {
        if (rv == MBEDTLS_ERR_SSL_WANT_WRITE || rv == MBEDTLS_ERR_SSL_WANT_READ) {
            rv = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            rv = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    }
    return rv;
}

static ssize_t callback_send(nghttp2_session *session, const uint8_t *data,
                             size_t length, int flags, void *user_data)
{
    int rv = 0;
    struct sh2lib_handle *hd = user_data;

    int copy_offset = 0;
    int pending_data = length;

    //    printf("len %d\n", length);
    /* Send data in 1000 byte chunks */
    while (copy_offset != length) {
        //  printf("copy offset %d\n", copy_offset);
        int chunk_len = pending_data > 1000 ? 1000 : pending_data;
        int subrv = callback_send_inner(hd, data + copy_offset, chunk_len);
        if (subrv <= 0) {
            if (copy_offset == 0) {
                /* If no data is transferred, send the error code */
                rv = subrv;
            }
            break;
        }
        copy_offset += subrv;
        pending_data -= subrv;
        rv += subrv;
    }
    //    printf("ret %d\n", rv);
    return rv;
}

/*
 * The implementation of nghttp2_recv_callback type. Here we read data
 * from the network and write them in |buf|. The capacity of |buf| is
 * |length| bytes. Returns the number of bytes stored in |buf|. See
 * the documentation of nghttp2_recv_callback for the details.
 */
static ssize_t callback_recv(nghttp2_session *session, uint8_t *buf,
                             size_t length, int flags, void *user_data)
{
    struct sh2lib_handle *hd = user_data;
    int rv;
    rv = esp_tls_conn_read(hd->http2_tls, buf, (int)length);
    if (rv < 0) {
        if (rv == MBEDTLS_ERR_SSL_WANT_WRITE || rv == MBEDTLS_ERR_SSL_WANT_READ) {
            rv = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            rv = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    } else if (rv == 0) {
        rv = NGHTTP2_ERR_EOF;
    }
    return rv;
}

char *sh2lib_frame_type_str(int type)
{
    switch (type) {
    case NGHTTP2_HEADERS:
        return "HEADERS";
        break;
    case NGHTTP2_RST_STREAM:
        return "RST_STREAM";
        break;
    case NGHTTP2_GOAWAY:
        return "GOAWAY";
        break;
    case NGHTTP2_DATA:
        return "DATA";
        break;
    case NGHTTP2_SETTINGS:
        return "SETTINGS";
        break;
    case NGHTTP2_PUSH_PROMISE:
        return "PUSH_PROMISE";
        break;
    case NGHTTP2_PING:
        return "PING";
        break;
    default:
        return "other";
        break;
    }
}

static int callback_on_frame_send(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data)
{
    ESP_LOGD(TAG, "[frame-send] frame type %s", sh2lib_frame_type_str(frame->hd.type));
    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (nghttp2_session_get_stream_user_data(session, frame->hd.stream_id)) {
            ESP_LOGD(TAG, "[frame-send] C ----------------------------> S (HEADERS)");
#if DBG_FRAME_SEND
            ESP_LOGD(TAG, "[frame-send] headers nv-len = %d", frame->headers.nvlen);
            const nghttp2_nv *nva = frame->headers.nva;
            size_t i;
            for (i = 0; i < frame->headers.nvlen; ++i) {
                ESP_LOGD(TAG, "[frame-send] %s : %s", nva[i].name, nva[i].value);
            }
#endif
        }
        break;
    }
    return 0;
}

static int callback_on_frame_recv(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data)
{
    ESP_LOGD(TAG, "[frame-recv][sid: %d] frame type  %s", frame->hd.stream_id, sh2lib_frame_type_str(frame->hd.type));
    if (frame->hd.type == NGHTTP2_GOAWAY) {
        struct sh2lib_handle *hd = (struct sh2lib_handle *)user_data;
        if (hd->go_away_cb) {
            printf("%s: goaway received: Invoking application's callback", TAG);
            hd->go_away_cb(hd);
        }
    }
#if 0
    if (frame->hd.type != NGHTTP2_DATA) {
        return 0;
    }
    /* Subsequent processing only for data frame */
    sh2lib_frame_data_recv_cb_t data_recv_cb = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (data_recv_cb) {
        struct sh2lib_handle *h2 = user_data;
        (*data_recv_cb)(h2, NULL, 0, DATA_RECV_FRAME_COMPLETE);
    }
#endif
    return 0;
}

static int do_http2_connect(struct sh2lib_handle *hd,
                            nghttp2_on_header_callback hdr_cb,
                            nghttp2_on_data_chunk_recv_callback data_chunk_recv_cb,
                            nghttp2_on_stream_close_callback stream_close_cb,
                            sh2lib_on_goaway_receive_callback goaway_handle_cb)
{
    int ret;
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, callback_send);
    nghttp2_session_callbacks_set_recv_callback(callbacks, callback_recv);
    nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, callback_on_frame_send);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, callback_on_frame_recv);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, stream_close_cb);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, data_chunk_recv_cb);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, hdr_cb);
    ret = nghttp2_session_client_new(&hd->http2_sess, callbacks, hd);
    if (ret != 0) {
        ESP_LOGE(TAG, "[sh2-connect] New http2 session failed");
        nghttp2_session_callbacks_del(callbacks);
        return -1;
    }
    nghttp2_session_callbacks_del(callbacks);

    /* Create the SETTINGS frame */
    ret = nghttp2_submit_settings(hd->http2_sess, NGHTTP2_FLAG_NONE, NULL, 0);
    if (ret != 0) {
        ESP_LOGE(TAG, "[sh2-connect] Submit settings failed");
        return -1;
    }

    if (goaway_handle_cb) {
        hd->go_away_cb = goaway_handle_cb;
    }

    return 0;
}

int sh2lib_connect(struct sh2lib_handle *hd, const char *uri,
                   nghttp2_on_header_callback hdr_cb,
                   nghttp2_on_data_chunk_recv_callback data_chunk_recv_cb,
                   nghttp2_on_stream_close_callback stream_close_cb,
                   sh2lib_on_goaway_receive_callback goaway_handle_cb,
                   esp_tls_cfg_t *tls_cfg)
{
    memset(hd, 0, sizeof(*hd));
    const char *proto[] = {"h2", NULL};
    if (tls_cfg->alpn_protos == NULL) {
        ESP_LOGI(TAG, "[sh2-connect] Setting default tls_cfg parameter for alpn_proto.");
        tls_cfg->alpn_protos = proto;
        tls_cfg->non_block = true;
    }
    if ((hd->http2_tls = esp_tls_conn_http_new(uri, tls_cfg)) == NULL) {
        ESP_LOGE(TAG, "[sh2-connect] esp-tls connection failed");
        goto error;
    }
    struct http_parser_url u;
    http_parser_url_init(&u);
    http_parser_parse_url(uri, strlen(uri), 0, &u);
    hd->hostname = strndup(&uri[u.field_data[UF_HOST].off], u.field_data[UF_HOST].len);

    /* HTTP/2 Connection */
    if (do_http2_connect(hd, hdr_cb, data_chunk_recv_cb, stream_close_cb, goaway_handle_cb) != 0) {
        ESP_LOGE(TAG, "[sh2-connect] HTTP2 Connection failed with %s", uri);
        goto error;
    }

    return 0;
error:
    sh2lib_free(hd);
    return -1;
}

void sh2lib_free(struct sh2lib_handle *hd)
{
    if (hd->http2_sess) {
        nghttp2_session_del(hd->http2_sess);
        hd->http2_sess = NULL;
    }
    if (hd->http2_tls) {
        esp_tls_conn_delete(hd->http2_tls);
        hd->http2_tls = NULL;
    }
    if (hd->hostname) {
        free(hd->hostname);
        hd->hostname = NULL;
    }
}

int sh2lib_wait_for_io(struct sh2lib_handle *hd, int timeout_s, int timeout_ms)
{
    struct timeval tv = {
        .tv_sec = timeout_s,
        .tv_usec = timeout_ms * 1000,
    };
    struct timeval *tv_ptr = &tv;
    /* For infinite blocking ... */
    if (timeout_s == -1) {
        tv_ptr = NULL;
    }

    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(hd->http2_tls->sockfd, &read_fds);
    /* read_fds is always set, because we always want to check if a socket is
     * ready to read. The other end could send read data anytime
     */
    if (nghttp2_session_want_write(hd->http2_sess)) {
        /* We want to check if the sockfd is writeable, only and only
         * when the nghttp2 wants to write. Otherwise, the fd is pretty
         * much always writeable, and select() will keep returning
         * immediately.
         */
        FD_SET(hd->http2_tls->sockfd, &write_fds);
    }
    int ret = select(hd->http2_tls->sockfd + 1, &read_fds, &write_fds, NULL, tv_ptr);

    return ret;
}

int sh2lib_resume_deferred_data(struct sh2lib_handle *hd, int32_t sid)
{
    return nghttp2_session_resume_data(hd->http2_sess, sid);
}

int sh2lib_execute(struct sh2lib_handle *hd)
{
    int ret;
    ret = nghttp2_session_send(hd->http2_sess);
    if (ret != 0) {
        ESP_LOGE(TAG, "[sh2-execute] HTTP2 session send failed %d", ret);
        return -1;
    }
    ret = nghttp2_session_recv(hd->http2_sess);
    if (ret != 0) {
        ESP_LOGE(TAG, "[sh2-execute] HTTP2 session recv failed %d", ret);
        return -1;
    }

    return 0;
}

int sh2lib_do_get_with_nv(struct sh2lib_handle *hd, const nghttp2_nv *nva, size_t nvlen, void *arg)
{
    int ret = nghttp2_submit_request(hd->http2_sess, NULL, nva, nvlen, NULL, arg);
    if (ret < 0) {
        ESP_LOGE(TAG, "[sh2-do-get] HEADERS call failed");
        return -1;
    }
    return ret;
}

int sh2lib_do_get(struct sh2lib_handle *hd, const char *path, const char *token, void *arg)
{
    const nghttp2_nv nva[] = { SH2LIB_MAKE_NV(":method", "GET"),
                               SH2LIB_MAKE_NV(":scheme", "https"),
                               SH2LIB_MAKE_NV(":path", path),
                               SH2LIB_MAKE_NV("authorization", token),
                             };
    return sh2lib_do_get_with_nv(hd, nva, sizeof(nva) / sizeof(nva[0]), arg);
}

int sh2lib_do_putpost_with_nv(struct sh2lib_handle *hd, const nghttp2_nv *nva, size_t nvlen,
                              nghttp2_data_source_read_callback data_prd,
                              void *arg)
{

    nghttp2_data_provider sh2lib_data_provider;
    sh2lib_data_provider.read_callback = data_prd;
    sh2lib_data_provider.source.ptr = NULL;
    int ret = nghttp2_submit_request(hd->http2_sess, NULL, nva, nvlen, &sh2lib_data_provider, arg);
    if (ret < 0) {
        ESP_LOGE(TAG, "[sh2-do-putpost] HEADERS call failed");
        return -1;
    }
    return ret;
}

int sh2lib_do_post(struct sh2lib_handle *hd, const char *path, const char *token, const char *content_type,
                   nghttp2_data_source_read_callback data_prd,
                   void *arg)
{
    const nghttp2_nv nva[] = { SH2LIB_MAKE_NV(":method", "POST"),
                               SH2LIB_MAKE_NV(":scheme", "https"),
                               SH2LIB_MAKE_NV(":path", path),
                               SH2LIB_MAKE_NV("authorization", token),
                               SH2LIB_MAKE_NV("content-type", content_type),
                             };
    return sh2lib_do_putpost_with_nv(hd, nva, sizeof(nva) / sizeof(nva[0]), data_prd, arg);
}
