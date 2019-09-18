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
#include <string.h>

#include <httpc.h>

FILE *fp;

static int validate_status_code(httpc_conn_t *h, int expected_code)
{
    if (http_response_get_code(h) != expected_code) {
        printf("Fail\n");
        printf("Expected Status Code: %d, got %d\n", expected_code, http_response_get_code(h));
        return -1;
    }
    return 0;
}

static int validate_data(int from_index, const char *expected, const char *received, int received_len)
{
    if (strncmp(expected + from_index, received, received_len)) {
        printf("Fail\n");
        printf("Expected Data @index %d: %s, got %.*s (len %d)\n", from_index, expected, received_len, received, received_len);
        return -1;
    }
    return 0;
}

static int test_postman_get_multi()
{
    printf("test: open connection https://postman-echo.com/ ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("https://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    printf("Success\n");

    {
        printf("test:       - GET /get ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[500];
        int data_read = http_response_recv(h, buf, sizeof(buf));
        if (validate_status_code(h, 200)) {
            return -1;
        }

        char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
        if (validate_data(0, expected_result, buf, data_read)) {
            return -1;
        }
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get (don't read full response) ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[10];
        int current_index = 0;
        int data_read = http_response_recv(h, buf, sizeof(buf));
        if (data_read < 0) {
            return -1;
        }
        if (validate_status_code(h, 200)) {
            return -1;
        }

        char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
        if (validate_data(current_index, expected_result, buf, data_read)) {
            return -1;
        }
        current_index += data_read;
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get (10-bytes read buffer) ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[10];
        int current_index = 0;
        while (1) {
            int data_read = http_response_recv(h, buf, sizeof(buf));
            if (data_read == 0) {
                break;
            }
            if (data_read < 0) {
                return -1;
            }
            if (validate_status_code(h, 200)) {
                return -1;
            }

            char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
            if (validate_data(current_index, expected_result, buf, data_read)) {
                return -1;
            }
            current_index += data_read;
        }
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get?variable=value&a=b ....");
        http_request_new(h, ESP_HTTP_GET, "/get?variable=value&a=b");
        http_request_send(h, NULL, 0);
        char buf[200];
        int current_index = 0;
        while (1) {
            int data_read = http_response_recv(h, buf, sizeof(buf));
            if (data_read == 0) {
                break;
            }
            if (data_read < 0) {
                return -1;
            }

            if (validate_status_code(h, 200)) {
                return -1;
            }

            char *expected_result = "{\"args\":{\"variable\":\"value\",\"a\":\"b\"},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get?variable=value&a=b\"}";

            if (validate_data(current_index, expected_result, buf, data_read)) {
                return -1;
            }
            current_index += data_read;
        }
        printf("Success\n");
        http_request_delete(h);
    }

    http_connection_delete(h);

    return 0;
}

static int test_postman_get_multi_with_header_fetch()
{
    printf("test: open connection https://postman-echo.com/ with header fetch api call ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("https://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    printf("Success\n");

    {
        printf("test:       - GET /get ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[500];
        http_header_fetch(h);
        int data_read = http_response_recv(h, buf, sizeof(buf));
        if (validate_status_code(h, 200)) {
            return -1;
        }

        char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
        if (validate_data(0, expected_result, buf, data_read)) {
            return -1;
        }
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get (don't read full response) ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[10];
        http_header_fetch(h);
        int current_index = 0;
        int data_read = http_response_recv(h, buf, sizeof(buf));
        if (data_read < 0) {
            return -1;
        }
        if (validate_status_code(h, 200)) {
            return -1;
        }

        char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
        if (validate_data(current_index, expected_result, buf, data_read)) {
            return -1;
        }
        current_index += data_read;
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get (10-bytes read buffer) ....");
        http_request_new(h, ESP_HTTP_GET, "/get");
        http_request_send(h, NULL, 0);
        char buf[10];
        http_header_fetch(h);
        int current_index = 0;
        while (1) {
            int data_read = http_response_recv(h, buf, sizeof(buf));
            if (data_read == 0) {
                break;
            }
            if (data_read < 0) {
                return -1;
            }
            if (validate_status_code(h, 200)) {
                return -1;
            }

            char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
            if (validate_data(current_index, expected_result, buf, data_read)) {
                return -1;
            }
            current_index += data_read;
        }
        printf("Success\n");
        http_request_delete(h);
    }

    {
        printf("test:       - GET /get?variable=value&a=b ....");
        http_request_new(h, ESP_HTTP_GET, "/get?variable=value&a=b");
        http_request_send(h, NULL, 0);
        char buf[200];
        http_header_fetch(h);
        int current_index = 0;
        while (1) {
            int data_read = http_response_recv(h, buf, sizeof(buf));
            if (data_read == 0) {
                break;
            }
            if (data_read < 0) {
                return -1;
            }

            if (validate_status_code(h, 200)) {
                return -1;
            }

            char *expected_result = "{\"args\":{\"variable\":\"value\",\"a\":\"b\"},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get?variable=value&a=b\"}";
            if (validate_data(current_index, expected_result, buf, data_read)) {
                return -1;
            }
            current_index += data_read;
        }
        printf("Success\n");
        http_request_delete(h);
    }
    http_connection_delete(h);

    return 0;
}

static int test_postman_get()
{
    printf("test: GET https://postman-echo.com/get ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("https://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    http_request_new(h, ESP_HTTP_GET, "/get");
    http_request_send(h, NULL, 0);
    char buf[500];
    int data_read = http_response_recv(h, buf, sizeof(buf));

    if (validate_status_code(h, 200)) {
        return -1;
    }

    char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
    if (validate_data(0, expected_result, buf, data_read)) {
        return -1;
    }
    printf("Success\n");
    http_request_delete(h);
    http_connection_delete(h);
    return 0;
}

static int test_postman_send_custom_hdrs()
{
    printf("test: GET https://postman-echo.com/get with custom hdrs ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("https://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    http_request_new(h, ESP_HTTP_GET, "/get");
    const char *hdrs =                 \
                                       "Host:postman-echo.com\r\n" \
                                       "custom-hdr:my-hdr\r\n\r\n";
    http_request_send_custom_hdr(h, hdrs);
    http_request_send(h, NULL, 0);
    char buf[500];
    int data_read = http_response_recv(h, buf, sizeof(buf));

    if (validate_status_code(h, 200)) {
        printf("Full response: %s\n", buf);
        return -1;
    }

    char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"custom-hdr\":\"my-hdr\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get\"}";
    if (validate_data(0, expected_result, buf, data_read)) {
        return -1;
    }
    printf("Success\n");
    http_request_delete(h);
    http_connection_delete(h);
    return 0;
}

static int test_postman_http_get()
{
    printf("test: GET http://postman-echo.com/get ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("http://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    http_request_new(h, ESP_HTTP_GET, "/get");
    http_request_send(h, NULL, 0);
    char buf[500];
    int data_read = http_response_recv(h, buf, sizeof(buf));

    if (validate_status_code(h, 200)) {
        return -1;
    }

    char *expected_result = "{\"args\":{},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"80\"},\"url\":\"https://postman-echo.com/get\"}";
    if (validate_data(0, expected_result, buf, data_read)) {
        return -1;
    }
    printf("Success\n");
    http_request_delete(h);
    http_connection_delete(h);
    return 0;
}

static int test_postman_get_with_full_path()
{
#define FULL_PATH_URI "https://postman-echo.com/get?a=b&c=d"
    printf("test: GET https://postman-echo.com/get (with full path and query parameters) ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new(FULL_PATH_URI, &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    http_request_new(h, ESP_HTTP_GET, FULL_PATH_URI);
    http_request_send(h, NULL, 0);
    char buf[500];
    int data_read = http_response_recv(h, buf, sizeof(buf));
#undef FULL_PATH_URI
    if (validate_status_code(h, 200)) {
        return -1;
    }
    char *expected_result = "{\"args\":{\"a\":\"b\",\"c\":\"d\"},\"headers\":{\"x-forwarded-proto\":\"https\",\"host\":\"postman-echo.com\",\"user-agent\":\"ESP32 HTTP Client/1.0\",\"x-forwarded-port\":\"443\"},\"url\":\"https://postman-echo.com/get?a=b&c=d\"}";
    if (validate_data(0, expected_result, buf, data_read)) {
        return -1;
    }
    printf("Success\n");
    http_request_delete(h);
    http_connection_delete(h);
    return 0;
}

struct hdr_value_check {
    char hdr[MAX_HDR_VAL_LEN];
    char value[MAX_HDR_VAL_LEN];
    bool seen;
    bool skip_check;
} hdr_value_check[] = {
    {"Content-Type", "application/json; charset=utf-8", false, false},
    {"Date", "", false, true},
    {"ETAG", "", false, true},
    /* long header value field greater than MAX_HDR_VAL_LEN
       truncated to size of MAX_HDR_VAL_LEN
     */
    {"onetwothreefourfivesixseveneightninetenonetwothre", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvw", false, false},
    {"Server", "nginx", false, false},
    {"set-cookie", "", false, true},
    {"Vary", "Accept-Encoding", false, false},
    {"Content-Length", "143", false, false},
    {"Connection", "keep-alive", false, false},
    {"Access-Control-Allow-Credentials", "", false, false},
    {"Access-Control-Allow-Headers", "", false, false},
    {"Access-Control-Allow-Methods", "", false, false},
    {"Access-Control-Allow-Origin", "", false, false},
    {"Access-Control-Expose-Headers", "", false, false},
};

static int i = 0;

static void validate_response_hdr_val(const char *hdr_buf, const char *val_buf, void *arg)
{
    if (hdr_value_check[i].skip_check == false) {
        if (strcasecmp(hdr_buf, hdr_value_check[i].hdr) != 0 || strcasecmp(val_buf, hdr_value_check[i].value) != 0) {
            printf("\nIncorrect Match:\n Expected header value: %s  %s \t Recieved Header value: %s  %s", hdr_value_check[i].hdr, hdr_value_check[i].value, hdr_buf, val_buf);
            i++;
            return;
        }
    }
    hdr_value_check[i].seen = true;
    i++;
}

/* all_hdr_val_check() checks whether recieved header value field
   are same as expected values in hdr_val_check[]
 */
static void all_hdr_val_check()
{
    for (i = 0; i < sizeof(hdr_value_check) / sizeof(struct hdr_value_check) ; i++) {
        if (hdr_value_check[i].seen == false) {
            printf("all_hdr_val_check failed!\n");
            return;
        }
    }
    printf("Success\n");
}

static int test_validate_header_values()
{
    printf("test: header callback \n - all headers seen \n - long hdr/value truncated ....");
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new("https://postman-echo.com", &tls_cfg);
    if (!h) {
        printf("Fail, couldn't open connection\n");
        return -1;
    }
    /* Long header value field greater than MAX_HDR_VAL_LEN
     */
    http_request_new(h, ESP_HTTP_GET, "/response-headers?onetwothreefourfivesixseveneightninetenonetwothreefourfivesixseveneightnineten=abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdef");
    http_response_set_header_cb(h, validate_response_hdr_val, NULL);
    http_request_send(h, NULL, 0);
    http_header_fetch(h);
    char buf[500];
    all_hdr_val_check();
    int data_read = http_response_recv(h, buf, sizeof(buf));

    if (validate_status_code(h, 200)) {
        return -1;
    }
    char *expected_result = "{\"onetwothreefourfivesixseveneightninetenonetwothreefourfivesixseveneightnineten\":\"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdef\"}";
    if (validate_data(0, expected_result, buf, data_read)) {
        return -1;
    }
    http_request_delete(h);
    http_connection_delete(h);
    return 0;
}

int main_test_func()
{
    test_postman_http_get();
    test_postman_get();
    test_postman_get_multi();
    test_postman_get_with_full_path();
    test_postman_send_custom_hdrs();
    test_postman_get_multi_with_header_fetch();
    test_validate_header_values();
    return 0;
}

int main(int argc, char *argv[])
{
    char *op_str = "GET";
    char *url = "https://postman-echo.com";
    char *path = "/get";
    char *data = "";
    char *file_name = NULL;
    bool out_file_defined = false;

    /* Assume that argv[1] is url, and argv[2] is path */
    if (argc >= 4) {
        op_str = argv[1];
        url = argv[2];
        path = argv[3];
        if (argc == 5) {
            data = argv[4];
        }
        if (argc == 6) {
            file_name = argv[5];
            fp = fopen(file_name, "w+");
            if (fp) {
                out_file_defined = true;
            }
        }
    } else if (argc == 2) {
        op_str = argv[1];
    } else {
        printf("Using default parameters \"GET %s %s\n", url, path);
        printf("Usage:\n");
        printf("      %s GET https://postman-echo.com \"/get?a=b&c=d\" <-o out_file> \n", argv[0]);
        printf("      %s POST https://postman-echo.com /post \"a=b&c=d\"\n", argv[0]);
        printf("      %s TEST\n", argv[0]);
        return 0;
    }

    httpc_ops_t op = -1;
    if (strcmp(op_str, "GET") == 0) {
        op = ESP_HTTP_GET;
    } else if (strcmp(op_str, "POST") == 0) {
        op = ESP_HTTP_POST;
    } else if (strcmp(op_str, "TEST") == 0) {
        return main_test_func();
    }
    esp_tls_cfg_t tls_cfg;
    memset(&tls_cfg, 0, sizeof(tls_cfg));
    httpc_conn_t *h = http_connection_new(url, &tls_cfg);
    http_request_new(h, op, path);
    http_request_send(h, data, strlen(data));
    char buf[700];
    int data_read, total_data_read = 0;
    while ((data_read = http_response_recv(h, buf, sizeof(buf)))) {
        total_data_read += data_read;
        if (out_file_defined) {
            //Write data to file
            fwrite(buf, data_read, 1, fp);
        } else {
            printf("Data is: %.*s\n", data_read, buf);
        }
    }

    printf ("\nTotal data read = %d bytes\n", total_data_read);

    if (out_file_defined) {
        fclose(fp);
    }
    return 0;
}
