#include <esp_tls.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <errno.h>
#include <esp_httpd_ota.h>

#define OTA_BUF_SIZE 2048

static const char *TAG = "[esp_httpd_ota]";
static void (*event_callback)(esp_httpd_ota_cb_event_t event);

esp_err_t update_post_handler(httpd_req_t *req);

httpd_uri_t update = {
    .uri       = "/update",
    .method    = HTTP_POST,
    .handler   = update_post_handler,
    .user_ctx  = NULL
};

esp_err_t update_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Got post request");
    event_callback(ESP_HTTPD_OTA_PRE_UPDATE);

    int data_len = -1, binary_file_len = 0, percentage_done = 0, prev_percentage_done = 0;
    size_t total_len = req->content_len;
    size_t remaining = total_len;
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA...");
    httpd_resp_send_chunk(req, "Starting OTA...\n", strlen("Starting OTA...\n"));
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Passive OTA partition not found");
        goto ota_fail;
    }
    ESP_LOGI(TAG, "Initialising OTA subtype %d (OTA_%d). Erasing partition at offset 0x%x", update_partition->subtype, (update_partition->subtype - 16),update_partition->address);
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        goto ota_fail;
    }

    char *upgrade_data_buf = (char *)malloc(OTA_BUF_SIZE);
    if (!upgrade_data_buf) {
        ESP_LOGE(TAG, "Couldn't allocate memory to upgrade data buffer");
        goto ota_fail;
    }
    ESP_LOGI(TAG, "Receiving binary file and writing to partition.");
    httpd_resp_send_chunk(req, "Uploading. This may take a while. Please Wait.\n", strlen("Uploading. This may take a while. Please Wait.\n"));
    while (remaining > 0) {
        /* Read the data for the request */
        data_len = httpd_req_recv(req, upgrade_data_buf, remaining < OTA_BUF_SIZE ? remaining : OTA_BUF_SIZE);
        if (data_len == HTTPD_SOCK_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Got timeout. errno is EAGAIN. Trying again.\n");
            continue;
        } else if (data_len < 0) {
            ESP_LOGE(TAG,"Error in https_req_recv. errno is: %d, data_len: %d\n", errno, data_len);
            break;
        }
        remaining -= data_len;

        err = esp_ota_write(update_handle, (const void *)upgrade_data_buf, data_len);
        if (err != ESP_OK) {
            break;
        }
        binary_file_len += data_len;
        percentage_done = (100 - ((total_len - binary_file_len) * 100 / total_len));
        if (percentage_done % 5 == 0 && percentage_done != prev_percentage_done && percentage_done < 100) {
            ESP_LOGI(TAG, "Percentage done: %d, written image length: %d, remaining length: %d", percentage_done, binary_file_len, remaining);
            prev_percentage_done = percentage_done;
        }
    }
    ESP_LOGI(TAG, "Percentage done: %d, written image length: %d, remaining length: %d", percentage_done, binary_file_len, remaining);
    free(upgrade_data_buf);

    if (data_len < 0) {
        ESP_LOGE(TAG, "Receive failed");
        goto ota_fail;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
        goto ota_fail;
    }

    ESP_LOGI(TAG, "Done updating firmware.");
    httpd_resp_send_chunk(req, "Done Updating Firmware\n", strlen("Done Updating Firmware\n"));
    httpd_resp_send_chunk(req, NULL, 0);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_end failed! err=0x%x. Image is invalid", err);
        goto ota_fail;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        goto ota_fail;
    }

    event_callback(ESP_HTTPD_OTA_POST_UPDATE);

    ESP_LOGI(TAG, "Restarting system in 5 seconds!");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;

ota_fail:
    httpd_resp_send_chunk(req, "OTA Failed. Please try again.\n", strlen("OTA Failed. Please try again.\n"));
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_FAIL;
}

void esp_httpd_ota_update_init(void (*event_cb)(esp_httpd_ota_cb_event_t event), httpd_handle_t server_handle)
{
    esp_err_t err;
    httpd_config_t server_config   = HTTPD_DEFAULT_CONFIG();

    if (server_handle == NULL) {
        if ((err = httpd_start(&server_handle, &server_config)) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start http server: %s", esp_err_to_name(err));
        }
        ESP_LOGI(TAG, "http server started");
    }
    httpd_register_uri_handler(server_handle, &update);
    ESP_LOGI(TAG, "URI handler registered: update");
    event_callback = event_cb;
}
