#include "tcp_socket.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

static const char *TAG = "APP_TCP";

static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.
            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    // Failed to retransmit, giving up
                    return;
                }
                to_write -= written;
            }
        }
    } while (len > 0);
}

#define CONFIG_EXAMPLE_IPV4 true

static int server_socket;
static int current_status = TCP_STATUS_INITIATED;
static uint64_t last_socket_check;

tcp_status_t tcp_server_socket_setup(uint64_t current_time) {
    if (current_status == TCP_STATUS_SETUP) return current_status;
    if (current_time - last_socket_check < 1000000) return current_status;
    last_socket_check = current_time;

    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    server_socket = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        current_status = TCP_STATUS_SOCKET_FAILED;
    }
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(server_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        close(server_socket);
        current_status = TCP_STATUS_BINDING_FAILED;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(server_socket, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(server_socket);
        current_status = TCP_STATUS_LISENNING_FAILED;
    }

    current_status = TCP_STATUS_SETUP;
    return current_status;
}

char addr_str[128];
int keepAlive = 1;
int keepIdle = KEEPALIVE_IDLE;
int keepInterval = KEEPALIVE_INTERVAL;
int keepCount = KEEPALIVE_COUNT;

uint64_t last_socket_task_check;

void tcp_server_socket_task(uint64_t current_time)
{
    // if (current_status != TCP_STATUS_SETUP) return;
    // if (current_time - last_socket_task_check < 1000000) return;
    // last_socket_task_check = current_time;

    //! Set server socket to non-blocking mode
    // int flags = fcntl(server_socket, F_GETFL, 0);
    // fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // while (1) {
        ESP_LOGI(TAG, "Socket listening");
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);

        //! blocked by default, check the flags above for non-blocking mode
        int sock = accept(server_socket, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            return;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
        if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        // shutdown(sock, 0);
        close(sock);
    // }
}


static const char *payload = "Message from ESP32 ";
uint64_t last_check_client_time;

// IPV4 - Client
// TODO: add IPV6 Client
void tcp_client_socket_task(uint64_t current_time)
{
    if (current_time - last_check_client_time < 1000000) return;
    last_check_client_time = current_time;

    char rx_buffer[128];
    char host_ip[] = "10.0.0.64";
    int addr_family = AF_INET;
    int ip_protocol = 0;

    #if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
    #elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_storage dest_addr = { 0 };
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
    #endif

    // create socket
    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

    //! Set socket to non-blocking mode
    // int flags = fcntl(sock, F_GETFL, 0);
    // fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    //! Initiate connection. blocked by default, check the flags above for non-blocking mode
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Successfully connected");

    // while (1) {
        err = send(sock, payload, strlen(payload), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            return;
        }

        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occurred during receiving
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            return;
        }
        // Data received
        else {
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
            ESP_LOGI(TAG, "%s", rx_buffer);
        }
    // }

    // if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        // shutdown(sock, 0);
        close(sock);
    // }
}