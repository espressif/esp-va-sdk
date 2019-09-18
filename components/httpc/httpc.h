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

#ifndef _ESP_HTTPC_H_
#define _ESP_HTTPC_H_

#include <esp_tls.h>
#include <http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HTTPC_BUF_SIZE 50

typedef enum {
    ESP_HTTP_GET,
    ESP_HTTP_POST,
    ESP_HTTP_PUT,
    ESP_HTTP_NOTIFY,
} httpc_ops_t;

enum httpc_conn_state {
    ESP_HTTP_UNINITED = 0,
    ESP_HTTP_INIT,
    ESP_HTTP_TLS_CONNECT,
    ESP_HTTP_CONNECTION_DONE,
    ESP_HTTP_REQ_NEW,
    ESP_HTTP_REQ_HDR_SENT,
    ESP_HTTP_RESP_STARTED,
    ESP_HTTP_RESP_HDR_RECEIVED,
    ESP_HTTP_RESP_BDY_RECEIVED,
};

typedef struct redirect_location {
    char *uri;
    int buf_size;
} redirect_location_t;

typedef void  (* httpc_response_header_cb)(const char *, const char *, void *arg);

/* The maximum length of a header or value that we are interested in */
#ifdef ESP_PLATFORM
#define MAX_HDR_VAL_LEN   CONFIG_HTTP_CLIENT_MAX_HDR_VAL_LEN
#else
#define MAX_HDR_VAL_LEN 50
#endif
typedef struct httpc_conn {
    struct http_parser_url u; /* Used for url parsing */
    struct esp_tls *tls;
    bool is_tls;
    bool is_async;
    char *host;

    /* State maintained by us */
    enum httpc_conn_state state;
    struct httpc_req {
        /* Request parameters */
        httpc_ops_t op;
        const char *url;
#define DEFAULT_CONTENT_TYPE "application/x-www-form-urlencoded"
        const char *content_type;
        redirect_location_t location; //redirect url when response 301/302/303 etc.
        size_t content_length;
        http_parser_settings parser_settings;
        http_parser parser;
        struct parser_state {
            bool last_was_hdr;
            char hdr_buf[MAX_HDR_VAL_LEN];
            char val_buf[MAX_HDR_VAL_LEN];
            httpc_response_header_cb response_hdr_cb;
	    void *response_hdr_cb_arg;
        } parser_state;
        /* Pointer to user buffer to copy data into */
        char *out_buf;
        size_t out_buf_len;
        int out_buf_index;
        /* Pointer to allocate extra body content read in
         * http_header_fetch API call
         */
        char *hdr_overflow_buf;
        int hdr_overflow_buf_len;
        int hdr_overflow_buf_index;
        char response_content_type[MAX_HDR_VAL_LEN];
    } request;
} httpc_conn_t;

httpc_conn_t *http_connection_new(const char *url, esp_tls_cfg_t *tls_cfg);
/**
 * Returns 1 on success.
 * 0 on EAGAIN. If this return code is received application may decide to retry.
 * -1 on fatal error.
 */
int http_connection_new_async(const char *url, esp_tls_cfg_t *tls_cfg, httpc_conn_t **hc);

/* Cleanup the connection. */
void http_connection_delete(httpc_conn_t *httpc);

/**
 * Function checks if old host is same as new and old protocol is same as new.
 * Return true or false in result
 */
bool http_connection_new_needed(httpc_conn_t *httpc, const char *url);

int http_request_new(httpc_conn_t *httpc, httpc_ops_t op, const char *url);
void http_request_delete(httpc_conn_t *httpc);
int http_header_fetch(httpc_conn_t *h);
int http_request_send(httpc_conn_t *httpc, const char *data, size_t data_len);
int http_response_recv(httpc_conn_t *httpc, char *data, size_t data_len);
void http_response_set_header_cb(httpc_conn_t *httpc, httpc_response_header_cb cb, void *arg);

static inline int http_response_get_code(httpc_conn_t *httpc)
{
    return httpc->request.parser.status_code;
}

static inline size_t http_response_get_content_len(httpc_conn_t *httpc)
{
    return httpc->request.content_length;
}

static inline char *http_response_get_content_type(httpc_conn_t *httpc)
{
    return httpc->request.response_content_type;
}

/* In case of status 302 we get redirect location. */
static inline char *http_response_get_redirect_location(httpc_conn_t *httpc)
{
    return httpc->request.location.uri;
}

/* All the headers will be the responsibility of the caller.
 * The httpc module will only prefix this with the request line.
 */
int http_request_send_custom_hdr(httpc_conn_t *httpc, const char *hdr);
int http_send_chunk(httpc_conn_t *httpc, const char *data, size_t data_len);
int http_send_last_chunk(httpc_conn_t *httpc);
int http_connection_get_sockfd(httpc_conn_t *http_conn);

/**
 * Set keepalive and recv_timeout for current connection.
 * Currently keepalive time is 30s and recv timeout is 10 ms hard coded.
 */
void http_connection_set_keepalive_and_recv_timeout(httpc_conn_t *httpc);

#ifdef __cplusplus
}
#endif

#endif /* ! ESP_HTTPC_H */
