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

#define CONFIG_EXAMPLE_IPV4 true
#define PORT                        3333
#define MAX_CLIENTS 10
static int client_sockets[MAX_CLIENTS];

static const char *payload = "Message from ESP32 ";
uint64_t last_send_time;
#define SERVER_IP "10.0.0.233"

static const char *TAG = "APP_TCP";
static int server_socket;
static int client_socket;
static int current_status = TCP_STATUS_INITIATED;
static uint64_t last_socket_check;

void tcp_server_socket_setup(uint64_t current_time) {
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
    #elif CONFIG_EXAMPLE_IPV6
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
        return;
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
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(server_socket, MAX_CLIENTS);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(server_socket);
        current_status = TCP_STATUS_LISENNING_FAILED;
        return;
    }

    // Set the server socket to non-blocking mode
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }

    current_status = TCP_STATUS_SETUP;
}

void tcp_server_socket_task(uint64_t current_time) {
    if (current_status != TCP_STATUS_SETUP) return;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    int max_fd = server_socket;

    // Add all client sockets to the fd_set
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) {
            FD_SET(client_sockets[i], &read_fds);
            if (client_sockets[i] > max_fd) {
                max_fd = client_sockets[i];
            }
        }
    }

    // Wait for activity on any socket
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1-second timeout
    timeout.tv_usec = 0;

    int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (activity < 0) {
        ESP_LOGE(TAG, "select failed: errno %d", errno);
        return;
    }

    // Check for new connections
    if (FD_ISSET(server_socket, &read_fds)) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);

        int new_socket = accept(server_socket, (struct sockaddr *)&source_addr, &addr_len);
        if (new_socket < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        } else {
            // Set the new socket to non-blocking mode
            int flags = fcntl(new_socket, F_GETFL, 0);
            fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

            // Add the new socket to the list of client sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = new_socket;
                    ESP_LOGI(TAG, "New client connected, socket fd: %d", new_socket);
                    break;
                }
            }
        }
    }

    // Handle data from clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0 && FD_ISSET(client_sockets[i], &read_fds)) {
            char buffer[1024];

            int len = recv(client_sockets[i], buffer, sizeof(buffer) - 1, 0);
            if (len > 0) {
                buffer[len] = '\0';  // Null-terminate the received data
                ESP_LOGI(TAG, "Received %d bytes from client %d: %s", len, client_sockets[i], buffer);

                // Echo the data back to the client
                send(client_sockets[i], buffer, len, 0);
            } else if (len == 0) {
                // Client disconnected
                ESP_LOGI(TAG, "Client disconnected, socket fd: %d", client_sockets[i]);
                close(client_sockets[i]);
                client_sockets[i] = -1;
            } else {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                close(client_sockets[i]);
                client_sockets[i] = -1;
            }
        }
    }
}


// TODO: add IPV6 Client
int tcp_client_connect() {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    int err = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0) {
        if (errno == EINPROGRESS) {
            // Connection in progress, wait for it to complete
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(client_socket, &write_fds);

            struct timeval timeout;
            timeout.tv_sec = 5;  // 5-second timeout
            timeout.tv_usec = 0;

            int ret = select(client_socket + 1, NULL, &write_fds, NULL, &timeout);
            if (ret > 0) {
                if (FD_ISSET(client_socket, &write_fds)) {
                    // Connection successful
                    int sockerr;
                    socklen_t len = sizeof(sockerr);
                    getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &sockerr, &len);
                    if (sockerr == 0) {
                        ESP_LOGI(TAG, "Connected to server");
                        return 0;
                    } else {
                        ESP_LOGE(TAG, "Connection failed: errno %d", sockerr);
                        return -1;
                    }
                }
            } else {
                ESP_LOGE(TAG, "Connection timeout");
                return -1;
            }
        } else {
            ESP_LOGE(TAG, "Failed to connect to server: errno %d", errno);
            return -1;
        }
    }
    return 0;
}

void tcp_client_socket_task(uint64_t current_time) {
    // Throttle sending to once per second
    if (current_time - last_send_time < 1000000) return;
    last_send_time = current_time;

    // Create a socket if it doesn't exist
    if (client_socket < 0) {
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (client_socket < 0) {
            ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
            return;
        }

        // Set the socket to non-blocking mode
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

        // Connect to the server
        if (tcp_client_connect() < 0) {
            close(client_socket);
            client_socket = -1;
            return;
        }
    }

    // Send data to the server
    const char *message = "Hello, Server!";
    int err = send(client_socket, message, strlen(message), 0);
    if (err < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // The socket is non-blocking, and the send buffer is full
            ESP_LOGW(TAG, "Send buffer full, retrying later");
        } else {
            ESP_LOGE(TAG, "Failed to send data: errno %d", errno);
            close(client_socket);
            client_socket = -1;
            return;
        }
    } else {
        ESP_LOGI(TAG, "Sent data to server: %s", message);
    }

    // Receive data from the server
    char buffer[128];
    int len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';  // Null-terminate the received data
        ESP_LOGI(TAG, "Received data from server: %s", buffer);
    } else if (len == 0) {
        // Server closed the connection
        ESP_LOGI(TAG, "Server closed the connection");
        close(client_socket);
        client_socket = -1;
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, non-blocking socket
            ESP_LOGW(TAG, "No data received from server");
        } else {
            ESP_LOGE(TAG, "Failed to receive data: errno %d", errno);
            close(client_socket);
            client_socket = -1;
        }
    }
}