// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <stdint.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_console.h>
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "lwip/init.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"

#include <va_mem_utils.h>
#include <network_diagnostics.h>

#define PRINT_LOOP_DELAY (500)                              /* 500 milliseconds */
#define PRINT_INTERVAL_RSSI (60 * 1000 * 1000)              /* 60 seconds */
#define PRINT_INTERVAL_SPEED (1 * 1000 * 1000)              /* 1 second */
#define PRINT_INTERVAL_PING_SUCCESS (60 * 1000 * 1000)      /* 60 seconds */
#define PRINT_INTERVAL_PING_TIMEOUT (5 * 1000 * 1000)       /* 5 seconds */
#define PING_TIMEOUT_WAIT (1)                               /* 1 second */
#define PING_HOSTNAME "8.8.8.8"                             /* Ping to DNS server */

static const char *TAG = "[network_diagnostics]";

struct network_diagnostics_ping {
    uint32_t print_interval;
    int sock;
    struct sockaddr_storage target_addr;
    struct icmp_echo_hdr *packet_hdr;
    ip_addr_t recv_addr;
};

struct network_speed {
    bool status;
    int data;
    uint64_t prev_time;
    int enable_count;
    SemaphoreHandle_t data_lock;
};

static struct network_speed network_speed[NETWORK_PATH_MAX];
static struct network_diagnostics_ping *network_diagnostics_ping;
static bool init_done = false;

static void network_diagnostics_ping_perform();

static int ping_cli_handler(int argc, char *argv[])
{
    if (argc < 2) {
        printf("%s: Incorrect arguments\n", TAG);
        return 0;
    }
    /* Note: Can add init ping for a specific host and deinit ping in future */
    /*
    if (strcmp(argv[1], "start") == 0) {
        if (argc != 3) {
            // Will free ping, if already initialised
            network_diagnostics_ping_deinit();
            esp_err_t err = network_diagnostics_ping_init(PING_HOSTNAME);
        } else {
            // Will free ping, if already initialised
            network_diagnostics_ping_deinit();
            esp_err_t err = network_diagnostics_ping_init(argv[2]);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "Error initialising ping");
                network_diagnostics_ping_deinit();
            }
        }
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "Error initialising ping");
            network_diagnostics_ping_deinit();
        }
    } else if (strcmp(argv[1], "stop") == 0) {
        network_diagnostics_ping_deinit();
    }
    */
    if (strcmp(argv[1], "perform") == 0){
        network_diagnostics_ping_perform();
    } else {
        printf("\n%s: Invalid argument:%s:\n", TAG, argv[1]);
    }
    return 0;
}

static esp_console_cmd_t ping_cmds[] = {
    {
        .command = "ping",
        .help = "Usage: ping perform",
        .func = ping_cli_handler,
    }
};

static int network_diagnostics_register_cli()
{
    int cmds_num = sizeof(ping_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        ESP_LOGI(TAG, "Registering command: %s", ping_cmds[i].command);
        esp_console_cmd_register(&ping_cmds[i]);
    }
    return 0;
}

void network_diagnostics_speed_add(network_path_t path, int data)
{
    if (!init_done) {
        return;
    }
    xSemaphoreTake(network_speed[path].data_lock, portMAX_DELAY);
    network_speed[path].data += data;
    xSemaphoreGive(network_speed[path].data_lock);
}

void network_diagnostics_speed_enable(network_path_t path, bool status)
{
    if (!init_done) {
        return;
    }
    xSemaphoreTake(network_speed[path].data_lock, portMAX_DELAY);

    /* The count can go negative in case of disconnects where multiple sources disable */
    network_speed[path].enable_count += status ? 1 : -1;
    network_speed[path].enable_count = network_speed[path].enable_count < 0 ? 0 : network_speed[path].enable_count;
    /* Check if first one to enable or if last one to disable */
    if ((status == true && network_speed[path].enable_count != 1) || (status == false && network_speed[path].enable_count != 0)) {
        ESP_LOGD(TAG, "Already enabled so not enabling again. (Or someone else enabled too, so not disabling.)");
        xSemaphoreGive(network_speed[path].data_lock);
        return;
    }

    if (status == true) {
        network_speed[path].prev_time = esp_timer_get_time();
    }
    network_speed[path].status = status;
    xSemaphoreGive(network_speed[path].data_lock);
}

static int network_diagnostics_speed_get(network_path_t path)
{
    xSemaphoreTake(network_speed[path].data_lock, portMAX_DELAY);
    if (network_speed[path].status == false && network_speed[path].data <= 0) {
        xSemaphoreGive(network_speed[path].data_lock);
        return -1;
    }
    uint32_t current_time = 0;
    current_time = esp_timer_get_time();
    uint64_t time_diff = (current_time - network_speed[path].prev_time);
    if (time_diff < PRINT_INTERVAL_SPEED) {
        xSemaphoreGive(network_speed[path].data_lock);
        return -1;
    }
    int data = network_speed[path].data;
    int speed = data / (time_diff / (1000 * 1000));
    network_speed[path].prev_time = current_time;
    network_speed[path].data -= data;
    xSemaphoreGive(network_speed[path].data_lock);
    return speed;
}

static esp_err_t network_diagnostics_check_network_speed()
{
    int speed = 0;

    /* Download */
    speed = network_diagnostics_speed_get(NETWORK_PATH_DOWNLOAD);
    if (speed != -1) {
        ESP_LOGI(TAG, "Download speed (bytes per second) is: %d", speed);
    }

    /* Upload */
    speed = network_diagnostics_speed_get(NETWORK_PATH_UPLOAD);
    if (speed != -1) {
        ESP_LOGI(TAG, "Upload speed (bytes per second) is: %d", speed);
    }

    return ESP_OK;
}

static esp_err_t network_diagnostics_check_signal_strength()
{
    /* Print in PRINT_INTERVAL_RSSI intervals */
    static uint32_t prev_time = 0;
    uint32_t current_time = 0;
    current_time = esp_timer_get_time();
    if (current_time - prev_time < PRINT_INTERVAL_RSSI) {
        return ESP_OK;
    }
    prev_time = current_time;

    /* Get and print RSSI */
    esp_err_t err = ESP_OK;
    wifi_ap_record_t ap_info;
    memset(&ap_info, 0, sizeof(ap_info));

    if ((err = esp_wifi_sta_get_ap_info(&ap_info)) != ESP_OK) {
        ESP_LOGE(TAG, "Network not found");
        return err;
    }
    ESP_LOGI(TAG, "Network rssi: %d", ap_info.rssi);
    return err;
}

static esp_err_t network_diagnostics_ping_send(struct network_diagnostics_ping *network_diagnostics_ping)
{
    esp_err_t ret = ESP_OK;
    network_diagnostics_ping->packet_hdr->seqno++;
    /* Generate checksum since "seqno" has changed */
    network_diagnostics_ping->packet_hdr->chksum = 0;
    if (network_diagnostics_ping->packet_hdr->type == ICMP_ECHO) {
        network_diagnostics_ping->packet_hdr->chksum = inet_chksum(network_diagnostics_ping->packet_hdr, sizeof(struct icmp_echo_hdr));
    }

    int sent = sendto(network_diagnostics_ping->sock, network_diagnostics_ping->packet_hdr, sizeof(struct icmp_echo_hdr), 0,
                      (struct sockaddr *)&network_diagnostics_ping->target_addr, sizeof(network_diagnostics_ping->target_addr));

    if (sent != sizeof(struct icmp_echo_hdr)) {
        int opt_val;
        socklen_t opt_len = sizeof(opt_val);
        getsockopt(network_diagnostics_ping->sock, SOL_SOCKET, SO_ERROR, &opt_val, &opt_len);
        ESP_LOGE(TAG, "Ping send error=%d", opt_val);
        ret = ESP_FAIL;
    }
    return ret;
}

static int network_diagnostics_ping_receive(struct network_diagnostics_ping *network_diagnostics_ping)
{
    char buf[64]; // 64 bytes are enough to cover IP header and ICMP header
    int len = 0;
    struct sockaddr_storage from;
    int fromlen = sizeof(from);
    uint16_t data_head = 0;

    while ((len = recvfrom(network_diagnostics_ping->sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
        if (from.ss_family == AF_INET) {
            // IPv4
            struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
            inet_addr_to_ip4addr(ip_2_ip4(&network_diagnostics_ping->recv_addr), &from4->sin_addr);
            IP_SET_TYPE_VAL(network_diagnostics_ping->recv_addr, IPADDR_TYPE_V4);
            data_head = (uint16_t)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr));
        } else {
            // IPv6
            struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
            inet6_addr_to_ip6addr(ip_2_ip6(&network_diagnostics_ping->recv_addr), &from6->sin6_addr);
            IP_SET_TYPE_VAL(network_diagnostics_ping->recv_addr, IPADDR_TYPE_V6);
            data_head = (uint16_t)(sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr));
        }
        if (len >= data_head) {
            if (IP_IS_V4_VAL(network_diagnostics_ping->recv_addr)) {              // Currently we process IPv4
                struct ip_hdr *iphdr = (struct ip_hdr *)buf;
                struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
                if ((iecho->id == network_diagnostics_ping->packet_hdr->id) && (iecho->seqno == network_diagnostics_ping->packet_hdr->seqno)) {
                    return len;
                }
            } else if (IP_IS_V6_VAL(network_diagnostics_ping->recv_addr)) {      // Currently we process IPv6
                struct icmp6_echo_hdr *iecho6 = (struct icmp6_echo_hdr *)(buf + sizeof(struct ip6_hdr));
                if ((iecho6->id == network_diagnostics_ping->packet_hdr->id) && (iecho6->seqno == network_diagnostics_ping->packet_hdr->seqno)) {
                    return len;
                }
            }
        }
        fromlen = sizeof(from);
    }
    // If timeout, len will be -1
    return len;
}

static void network_diagnostics_ping_perform()
{
    /* Return if ping is not initialised */
    if(network_diagnostics_ping == NULL || network_diagnostics_ping->sock <= 0) {
        ESP_LOGI(TAG, "Ping is not initialised");
        return;
    }
    uint32_t start_time, end_time;
    int recv_ret;
    network_diagnostics_ping->packet_hdr->seqno = 0;
    network_diagnostics_ping_send(network_diagnostics_ping);
    start_time = esp_timer_get_time();
    recv_ret = network_diagnostics_ping_receive(network_diagnostics_ping);
    end_time = esp_timer_get_time();
    if (recv_ret >= 0) {
        ESP_LOGI(TAG, "Ping success: %d ms", (end_time - start_time) / 1000);
        /* Update ping interval */
        network_diagnostics_ping->print_interval = PRINT_INTERVAL_PING_SUCCESS;
    } else {
        ESP_LOGI(TAG, "Ping timeout: %d ms", (end_time - start_time) / 1000);
        /* Update ping interval */
        network_diagnostics_ping->print_interval = PRINT_INTERVAL_PING_TIMEOUT;
    }
}

static esp_err_t network_diagnostics_check_ping()
{
    /* Return if ping is not initialised */
    if(network_diagnostics_ping == NULL || network_diagnostics_ping->sock <= 0) {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t current_time = 0;
    static uint32_t previous_time = 0;
    current_time = esp_timer_get_time();
    /* Return if set ping interval has not passed */
    if(current_time - previous_time < network_diagnostics_ping->print_interval) {
        return ESP_OK;
    }
    previous_time = current_time;
    /* Do ping after set ping interval has passed */
    network_diagnostics_ping_perform();
    return ESP_OK;
}

static int create_socket()
{
    int sock;
    /* Create socket */
    if (IP_IS_V4(&network_diagnostics_ping->recv_addr)) {
        sock = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    } else {
        sock = socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
    }
    if(sock < 0) {
        ESP_LOGE(TAG, "Error creating ping socket");
        return -1;
    }

    /* Set receive timeout */
    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = PING_TIMEOUT_WAIT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Set tos */
    setsockopt(sock, IPPROTO_IP, IP_TOS, 0, sizeof(uint8_t));

    /* Set socket address */
    if (IP_IS_V4(&network_diagnostics_ping->recv_addr)) {
        struct sockaddr_in *to4 = (struct sockaddr_in *)&network_diagnostics_ping->target_addr;
        to4->sin_family = AF_INET;
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&network_diagnostics_ping->recv_addr));
        network_diagnostics_ping->packet_hdr->type = ICMP_ECHO;
    }
    if (IP_IS_V6(&network_diagnostics_ping->recv_addr)) {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&network_diagnostics_ping->target_addr;
        to6->sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(&network_diagnostics_ping->recv_addr));
        network_diagnostics_ping->packet_hdr->type = ICMP6_TYPE_EREQ;
    }
    return sock;
}

static esp_err_t get_ip_addr(char *hostname, ip_addr_t *target_addr)
{
    struct sockaddr_in6 sock_addr6;
    memset(target_addr, 0, sizeof(*target_addr));
    if (inet_pton(AF_INET6, hostname, &sock_addr6.sin6_addr) == 1) {
        /* Convert ip6 string to ip6 address */
        ipaddr_aton(hostname, target_addr);
    } else {
        struct addrinfo hint;
        struct addrinfo *res = NULL;
        memset(&hint, 0, sizeof(hint));
        /* Return if not a valid IP string or hostname */
        if (getaddrinfo(hostname, NULL, &hint, &res) != 0) {
            return ESP_FAIL;
        }
        /* Convert ip4 string or hostname to ip4 or ip6 address */
        if (res->ai_family == AF_INET) {
            struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
            inet_addr_to_ip4addr(ip_2_ip4(target_addr), &addr4);
        } else {
            struct in6_addr addr6 = ((struct sockaddr_in6 *) (res->ai_addr))->sin6_addr;
            inet6_addr_to_ip6addr(ip_2_ip6(target_addr), &addr6);
        }
        freeaddrinfo(res);
    }
    return ESP_OK;
}

static esp_err_t network_diagnostics_ping_init(char* hostname)
{
    /*
        Note: ping_send(), ping_recieve(), create_socket() implementations are same as the one's in ping_sock.c in esp-idf.
        esp-idf's ping creates a task which takes 2KB. In order to do ping without creating a task,
        only these 3 functions have been taken from the file and the task creation part is excluded to save memory.
        The reason for not wanting a task is because we already have a task for network diagnostics.
    */
    /*
        Note: As alexa does not have support for ipv6, ping does not support ipv6. If support is added for ipv6 in alexa,
        ping will have support for the same, since the code for ipv6 is already in place. This was verified after
        ping to ipv6.google.com was successful after temporary support for ipv6 was added in alexa for testing ping code
    */
    /* Allocate memory for structure */
    network_diagnostics_ping = (struct network_diagnostics_ping*) va_mem_alloc(sizeof(struct network_diagnostics_ping), VA_MEM_EXTERNAL);
    if(network_diagnostics_ping == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for ping");
        goto err;
    }
    network_diagnostics_ping->print_interval = PRINT_INTERVAL_PING_SUCCESS;

    /* Convert URL or IP string to IP address */
    ip_addr_t target_addr;
    esp_err_t err;
    err = get_ip_addr(hostname, &target_addr);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Ping: Not a valid hostname or IP address %s", hostname);
        goto err;
    }

    /* Set receiver address for ping */
    network_diagnostics_ping->recv_addr = target_addr;
    /* Set packet size to size of header + some additional bytes for data */
    network_diagnostics_ping->packet_hdr = va_mem_alloc(sizeof(struct icmp_echo_hdr), VA_MEM_EXTERNAL);
    if(network_diagnostics_ping->packet_hdr == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for ping packet header");
        goto err;
    }

    /* Set ICMP type and code field */
    network_diagnostics_ping->packet_hdr->code = 0;

    /* This will create a new socket connection */
    network_diagnostics_ping->sock = create_socket();
    if(network_diagnostics_ping->sock < 0) {
        return ESP_FAIL;
    }
    return ESP_OK;

err:
    if (network_diagnostics_ping) {
        if (network_diagnostics_ping->sock > 0) {
            close(network_diagnostics_ping->sock);
        }
        if (network_diagnostics_ping->packet_hdr) {
            free(network_diagnostics_ping->packet_hdr);
        }
        free(network_diagnostics_ping);
        network_diagnostics_ping = NULL;
    }
    return ESP_FAIL;
}

void network_diagnostics_task(void *arg)
{
    while (1) {
        network_diagnostics_check_signal_strength();
        network_diagnostics_check_network_speed();
        network_diagnostics_check_ping();

        /* Delay */
        vTaskDelay(PRINT_LOOP_DELAY / portTICK_PERIOD_MS);
    }
}

esp_err_t network_diagnostics_init()
{
    if (init_done) {
        ESP_LOGI(TAG, "Already initialized");
        return ESP_OK;
    }

    /* Network speed */
    for (int path = 0; path < NETWORK_PATH_MAX; path++) {
        vSemaphoreCreateBinary(network_speed[path].data_lock);
        if (network_speed[path].data_lock == NULL) {
            ESP_LOGE(TAG, "Could not create semaphore");
            return ESP_FAIL;
        }
    }

    /* Ping */
    esp_err_t err = network_diagnostics_ping_init(PING_HOSTNAME);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error initialising ping");
    }

    network_diagnostics_register_cli();

    /* Create task */
#define NETWORK_DIAGNOSTICS_STACK_SIZE (4096)
    StackType_t *task_stack = (StackType_t *)va_mem_alloc(NETWORK_DIAGNOSTICS_STACK_SIZE, VA_MEM_EXTERNAL);
    static StaticTask_t task_buf;
    TaskHandle_t handle = xTaskCreateStatic(network_diagnostics_task, "network_diagnostics", (NETWORK_DIAGNOSTICS_STACK_SIZE), NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, task_stack, &task_buf);
    if (handle == NULL) {
        ESP_LOGE(TAG, "Error creating task");
        return ESP_FAIL;
    }
    init_done = true;
    return ESP_OK;
}
