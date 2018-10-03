#include <stdio.h>
#include <esp_log.h>
#include <string.h>
#include <esp_err.h>
#include <alexa.h>
#include <mem_utils.h>

#include "avsconfig.pb-c.h"
static const char *TAG = "avsconfig";

static AVSConfigStatus app_handler_avs_config(const char *auth_code, const char *client_id, const char *redirect_uri, const char *code_verifier, void *priv_data)
{
    alexa_config_t *alexa_cfg = (alexa_config_t *) priv_data;

    alexa_cfg->auth_delegate.type = auth_type_comp_app;
    alexa_cfg->auth_delegate.u.comp_app.auth_code = mem_strdup(auth_code, EXTERNAL);
    alexa_cfg->auth_delegate.u.comp_app.client_id = mem_strdup(client_id, EXTERNAL);
    alexa_cfg->auth_delegate.u.comp_app.redirect_uri = mem_strdup(redirect_uri, EXTERNAL);
    alexa_cfg->auth_delegate.u.comp_app.code_verifier = mem_strdup(code_verifier, EXTERNAL);

    printf("APP Got: %s %s %s %s\n", auth_code, client_id, redirect_uri, code_verifier);
    return AVSCONFIG_STATUS__Success;
}

int avs_config_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    AVSConfigRequest *req;
    AVSConfigResponse resp;
    int ret;

    req = avsconfig_request__unpack(NULL, inlen, inbuf);
    if (!req) {
        ESP_LOGE(TAG, "Unable to unpack config data");
        return ESP_ERR_INVALID_ARG;
    }

    ret = app_handler_avs_config(req->authcode, req->clientid, req->redirecturi, req->codeverifier, priv_data);

    avsconfig_request__free_unpacked(req, NULL);

    avsconfig_response__init(&resp);
    resp.status = ret;
    /* Mask a bug in protobuf-c library which fails to encode message with single element with value zero. resp.dummy is
     * an unused field in the protocol */
    resp.dummy = 67162;

    *outlen = avsconfig_response__get_packed_size(&resp);
    if (*outlen <= 0) {
        ESP_LOGE(TAG, "Invalid encoding for response");
        return ESP_FAIL;
    }

    *outbuf = (uint8_t *) malloc(*outlen);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory\n");
        return ESP_ERR_NO_MEM;
    }

    avsconfig_response__pack(&resp, *outbuf);

    return ESP_OK;
}
