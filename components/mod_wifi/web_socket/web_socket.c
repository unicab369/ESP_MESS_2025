#include "web_socket.h"
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>


#include "esp_log.h"
#include "esp_err.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#define PORT 3333
#define TAG "WEBSOCKET_SERVER"
#define HANDSHAKE_MAGIC_NUMBER "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define MAX_CLIENTS 10
typedef struct {
    int socket;
    int8_t handshaked;
} socket_client_info_t;

static socket_client_info_t client_infos[MAX_CLIENTS] = {0};
static struct pollfd poll_arr[MAX_CLIENTS + 1];            // +1 for the server socket

static int fds_count;
static int server_sock = -1;
static uint64_t last_log_time;


void web_socket_server_cleanup(void) {
    memset(client_infos, 0, sizeof(client_infos));
    memset(poll_arr, 0, sizeof(poll_arr));
    fds_count = 1;                      // Start with only the server socket

    if (server_sock < 0) return;
    close(server_sock);
    server_sock = -1; // Reset to invalid descriptor
}

void web_socket_setup(void) {
    if (server_sock > 0) return;
    web_socket_server_cleanup();

    //! Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    }

    // Initialize the server socket in the pollfd array
    poll_arr[0].fd = server_sock;
    poll_arr[0].events = POLLIN;        // Monitor for incoming connections

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //! Bind the socket to the port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        web_socket_server_cleanup();
        return;
    }

    //! Listen for incoming connections, max pending connection = 5
    if (listen(server_sock, 5) < 0) {
        ESP_LOGE(TAG, "Failed to listen on socket");
        web_socket_server_cleanup();
        return;
    }

    //! Set the socket to non-blocking mode
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
    ESP_LOGI(TAG, "WebSocket server started on port %d", PORT);
}

static void remove_client_socket(int client_sock, int client_index) {
    close(client_sock);
    client_infos[client_index].socket = -1;
    client_infos[client_index].handshaked = 0;
    printf("Client socket %d removed\n", client_sock);
}

static int handle_recv(int client_sock, int client_index, char *rx_buff, size_t buffer_len) {
    int len = recv(client_sock, rx_buff, buffer_len, 0);
    if (len == 0) {
        // Client disconnected
        ESP_LOGW(TAG, "Client %d disconnected", client_sock);
        remove_client_socket(client_sock, client_index);
        return 0;

    } else if (len < 0) {
        ESP_LOGE(TAG, "recv() failed for client %d. err: %s", client_sock, strerror(errno));
        remove_client_socket(client_sock, client_index);
        return 0;
    }

    ESP_LOGI(TAG, "Received data from socket %d. err: %s", client_sock, rx_buff);
    rx_buff[len] = '\0'; // Null-terminate the received data
    return len;
}

static int handle_send(int client_sock, int client_index, char *tx_buffer, size_t buffer_len) {
    int result = send(client_sock, tx_buffer, buffer_len, 0);
    if (result < 0) {
        ESP_LOGE(TAG, "Client failed: %d. err: %s\n", client_sock, strerror(errno));
        remove_client_socket(client_sock, client_index);
    } else if (result == 0) {
        ESP_LOGI(TAG, "Client closed: %d", client_sock);
    }

    return result;
}

static void perform_handshake(int client_sock, int client_index) {
    //! recv: Receive data from the client. Make sure the buffer is big enough
    char rx_buff[500];
    int len = handle_recv(client_sock, client_index, rx_buff, sizeof(rx_buff) - 1);
    if (len < 1) return;

    if (!client_infos[client_index].handshaked) {
        //! Check for handshaking request
        char *key_start = strstr(rx_buff, "Sec-WebSocket-Key: ");
        if (!key_start) return;

        ESP_LOGI(TAG, "Found key_start");
        key_start += 19; // Move past "Sec-WebSocket-Key: "

        char *key_end = strstr(key_start, "\r\n");
        if (!key_end) return;
        
        ESP_LOGI(TAG, "Found key_end");
        *key_end = 0; // Null-terminate the key

        //! Concatenate client key with WebSocket GUID
        char combined[256];
        snprintf(combined, sizeof(combined), "%s%s", key_start, HANDSHAKE_MAGIC_NUMBER);

        //! Compute SHA-1 hash
        unsigned char sha1[20];
        if (mbedtls_sha1((unsigned char *)combined, strlen(combined), sha1) != 0) {
            ESP_LOGE(TAG, "SHA-1 computation failed");
            remove_client_socket(client_sock, client_index);
            return;
        }

        //! Base64 encode the hash
        size_t base64_len;
        char accept_key[64];
        if (mbedtls_base64_encode((unsigned char *)accept_key, sizeof(accept_key), &base64_len, sha1, 20) != 0) {
            ESP_LOGE(TAG, "Base64 encoding failed");
            remove_client_socket(client_sock, client_index);
            return;
        }

        //! build websocket handshake response
        const char *ws_handshake_response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n";

        char tx_buff[256]; // Ensure buffer is large enough
        snprintf(tx_buff, sizeof(tx_buff), ws_handshake_response, accept_key);
        ESP_LOGI(TAG, "Handshake response:\n%s\n", tx_buff);
        ESP_LOGI(TAG,"client_sock %d\n", client_sock);

        //! send: handshake response
        int result = handle_send(client_sock, client_index, tx_buff, strlen(tx_buff));
        if (result > 0) {
            ESP_LOGI(TAG, "Handshake sent successfully");
            client_infos[client_index].handshaked = 1;
        }

    } else {
        // Example: Echo the data back to the client
        if (send(client_sock, rx_buff, len, 0) < 0) {
            ESP_LOGE(TAG, "Failed to send to client %d: %s\n", client_sock, strerror(errno));
            remove_client_socket(client_sock, client_index);
        }
    }
}

static uint8_t web_socket_server_poll(uint64_t current_time) {
    //! Wait for activity on any socket
    int activity = poll(poll_arr, fds_count, 0); // -1 means no timeout
    if (activity < 0) return 0;

    //! Check for activity on the server socket
    if (poll_arr[0].revents & POLLIN) {
        // Accept a new connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(poll_arr[0].fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock < 0) {
            if (current_time - last_log_time > 1000000) {
                last_log_time = current_time;
                ESP_LOGE(TAG, "Failed to accept connection: %s\n", strerror(errno));
            }

        } else {
            printf("New connection accepted\n");
            if (fds_count < MAX_CLIENTS + 1) {
                poll_arr[fds_count].fd = client_sock;
                poll_arr[fds_count].events = POLLIN; // Monitor for incoming data
                fds_count++;

                //! Add the new client socket to the pollfd array
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_infos[i].socket <= 0) {
                        client_infos[i].socket = client_sock;
                        printf("Client added: %d\n", client_sock);

                        //! Perform handshake
                        perform_handshake(client_sock, i);
                        break;
                    }
                }
            } else {
                printf("Max clients reached, closing connection\n");
                close(client_sock);
            }
        }
    }
    
    return 1;
}


// Send a WebSocket text frame to the client
void send_websocket_message(int client_sock, const char *message) {
    size_t len = strlen(message);
    uint8_t frame[len + 2]; // Frame buffer (header + payload)

    // Construct the frame
    frame[0] = 0x81; // FIN bit set, opcode for text frame
    frame[1] = len;  // Payload length
    memcpy(frame + 2, message, len); // Copy the payload

    // Send the frame
    send(client_sock, frame, len + 2, 0);
}

void web_socket_poll(uint64_t current_time) {
    uint8_t check = web_socket_server_poll(current_time);
    if (!check) return;

    //! Check for activity on client sockets
    for (int i = 1; i < fds_count; i++) {
        struct pollfd target = poll_arr[i];
        int target_client = target.fd;

        if (target.revents & POLLERR) {
            //! Remove the client socket with an error
            printf("Error on client socket %d\n", target_client);
            remove_client_socket(target_client, i-1);
            poll_arr[i].fd = -1; // Mark as invalid in the pollfd array
        }

        if (target.revents & POLLIN) {
            //! Handle client data
            uint8_t frame[128];
            // uint8_t check 
            int len = recv(target_client, frame, sizeof(frame), 0);

            if (len > 0) {
                // Extract the payload length
                uint8_t payload_len = frame[1] & 0x7F; // Mask out the MASK bit
                uint8_t mask_offset = 2; // Start of masking key (if present)
        
                // Check if the payload is masked
                if (frame[1] & 0x80) { // MASK bit is set
                    // Extract the masking key
                    uint8_t masking_key[4];
                    memcpy(masking_key, frame + 2, 4);
                    mask_offset += 4; // Move past the masking key
        
                    // Unmask the payload
                    uint8_t payload[payload_len];
                    for (size_t i = 0; i < payload_len; i++) {
                        payload[i] = frame[mask_offset + i] ^ masking_key[i % 4];
                    }
        
                    // Log the unmasked payload
                    ESP_LOGI(TAG, "Received: %.*s", payload_len, payload);
                } else {
                    // Payload is not masked
                    ESP_LOGI(TAG, "Received: %.*s", payload_len, frame + mask_offset);
                }
        
                // Send a response to the client
                const char *response = "Hello from ESP32!";
                send_websocket_message(target_client, response);
                
            } else if (len == 0) {
                ESP_LOGI(TAG, "Client disconnected");
            }
        }
    }
}
