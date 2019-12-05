#ifndef _SSDP_SERVICE_H_
#define _SSDP_SERVICE_H_

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
#endif
