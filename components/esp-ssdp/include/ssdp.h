/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018-2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifndef _SSDP_SERVICE_H_
#define _SSDP_SERVICE_H_

#include <stdbool.h>
#define ESPDLNA_SERVER_STRING "ESP32 UPnP/DLNA Renderer"
#define RENDERER_HTTP_PATH "/rootDesc.xml"
#define RENDERER_HTTP_PORT 8080
#define SSDP_NOTIFY_INTERVAL     1800
// #define SSDP_NOTIFY_PORT      8080

struct lssdp_ctx;

typedef struct ssdp_t {
    struct lssdp_ctx *ctx;
    int run;
    int stopped;
} ssdp_t;

ssdp_t *ssdp_init(char *udn, int port);
void ssdp_add_services(ssdp_t *ssdp, const char *services[]);
int ssdp_scan_renderer(ssdp_t *ssdp);
void ssdp_destroy(ssdp_t *ssdp);
void ssdp_disable_discovery(bool val);
#endif
