// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUTH_DELEGATE_H_
#define _AUTH_DELEGATE_H_

#include "voice_assistant.h"

/* TODO: This should be parsed from LWA response. Hard-coding it right now */
#define MAX_TOKEN_SIZE  800

typedef struct {
    char *auth_code;
    char *client_id;
    char *redirect_uri;
    char *code_verifier;
} comp_app_config_t;

typedef struct {
    char *client_id;
    char *refresh_token;
    char *client_secret;
} subsequent_auth_config_t;

/** Auth Delegate Configuration Type
 *
 * Please refer to the documentation for \ref va_auth_type_t for more details.
 */
typedef enum {
    /** Subsequent authentication */
    auth_type_subsequent = 1,
    /** Authentication tokens from companion app */
    auth_type_comp_app,
} va_auth_type_t;

/** Auth Delegate Configuration
 *
 * There can be multiple ways in which the auth delegate can be configured. For example:
 * - comp_app: Authentication was performed by a companion app, we need to exchange these tokens for the real access tokens.
 * - subsequent: We have the real access tokens in NVS, and we should use that for authentication. This step typically happens after the comp_app authentication has been used once.
 */
typedef struct {
    va_auth_type_t type;
    union {
        comp_app_config_t comp_app;
        subsequent_auth_config_t subsequent;
    } u;
} auth_delegate_config_t;

/** Auth delegate sign in handler
 *
 * Handler will be called when the device has successfully signed in.
 * @param: client_id Device client ID
 * @param: refresh_token Device refresh token
 */
typedef void (*auth_delegate_signin_handler_t)(const char *client_id, const char *refresh_token);
typedef void (*auth_delegate_signout_handler_t)(void);

int auth_delegate_init(auth_delegate_signin_handler_t signin_handler, auth_delegate_signout_handler_t signout_handler);
/* For now, this handler will only be called if signing is done via companion app based provisioning (BLE or WiFi) */
int auth_delegate_sign_in(const char *oauth_url, auth_delegate_config_t *cfg);
void auth_delegate_sign_out();
int auth_delegate_get_token(char *buf, int size);

#endif /* _AUTH_DELEGATE_H_ */
