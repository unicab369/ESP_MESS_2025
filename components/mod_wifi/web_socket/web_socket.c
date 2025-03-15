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


//! Generate Sec-WebSocket-Accept key
static void generate_websocket_accept(const char *client_key, char *accept_key) {
    // Concatenate client key with WebSocket GUID
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", client_key, HANDSHAKE_MAGIC_NUMBER);

    //! Compute SHA-1 hash
    unsigned char sha1[20];
    if (mbedtls_sha1((unsigned char *)combined, strlen(combined), sha1) != 0) {
        printf("SHA-1 computation failed\n");
        return;
    }

    // Base64 encode the hash
    size_t len;
    mbedtls_base64_encode((unsigned char *)accept_key, 64, &len, sha1, 20);
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

#define MAX_CLIENTS 10
int num_fds = 1; // Start with only the server socket
struct pollfd fds[MAX_CLIENTS + 1]; // +1 for the server socket
int client_sock_arr[MAX_CLIENTS] = { -1 }; // Array to store client sockets

int server_sock = -1;
char rx_buffer[512];
web_socket_status_t status = WEBSOCKET_DISCONNECTED;

int client_sock;
uint64_t last_log_time;


void web_socket_server_cleanup(void) {
    if (server_sock < 0) return;
    close(server_sock);
    server_sock = -1; // Reset to invalid descriptor
}

void web_socket_setup(void) {
    if (status != WEBSOCKET_DISCONNECTED) return;
    web_socket_server_cleanup();

    //! Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    }

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
    status = WEBSOCKET_SETUP;

    // Initialize the server socket in the pollfd array
    fds[0].fd = server_sock;
    fds[0].events = POLLIN; // Monitor for incoming connections
    num_fds = 1;
}



void remove_client_socket(int sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sock_arr[i] == sock) {
            client_sock_arr[i] = -1;
            printf("Client socket %d removed\n", sock);
            break;
        }
    }
}


void handle_client_data(int sock) {
    char buffer[255];
    int len;

    // Check if the client has completed the handshake
    static int handshake_complete[MAX_CLIENTS] = {0}; // Tracks handshake status for each client

    // Find the index of the client socket
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sock_arr[i] == sock) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        printf("Invalid client socket\n");
        return;
    }

    //! Receive data from the client
    len = recv(sock, buffer, sizeof(buffer), 0);
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate the received data
        printf("Received data from socket %d: %s\n", sock, buffer);

        if (!handshake_complete[client_index]) {
            // Handle handshaking
            if (strcmp(buffer, "HELLO") == 0) {
                // Client sent the correct handshake message
                printf("Handshake received from client %d\n", sock);

                // Send the "WELCOME" response
                const char *welcome_msg = "WELCOME";
                if (send(sock, welcome_msg, strlen(welcome_msg), 0) < 0) {
                    printf("Failed to send WELCOME message to client %d: %s\n", sock, strerror(errno));
                    close(sock);
                    remove_client_socket(sock);
                } else {
                    printf("Handshake completed with client %d\n", sock);
                    handshake_complete[client_index] = 1; // Mark handshake as complete
                }
            } else {
                // Invalid handshake message
                printf("Invalid handshake message from client %d: %s\n", sock, buffer);
                close(sock);
                remove_client_socket(sock);
            }
        } else {
            // Handshake is complete, handle normal data exchange
            printf("Data from client %d: %s\n", sock, buffer);

            // Example: Echo the data back to the client
            if (send(sock, buffer, len, 0) < 0) {
                printf("Failed to send data to client %d: %s\n", sock, strerror(errno));
                close(sock);
                remove_client_socket(sock);
            }
        }
    } else if (len == 0) {
        //! Client disconnected
        printf("Client socket %d disconnected\n", sock);
        close(sock);
        remove_client_socket(sock);
        handshake_complete[client_index] = 0; // Reset handshake status
    } else {
        //! Error receiving data
        printf("Error receiving data from socket %d: %s\n", sock, strerror(errno));
        close(sock);
        remove_client_socket(sock);
        handshake_complete[client_index] = 0; // Reset handshake status
    }
}


void web_socket_poll(uint64_t current_time) {
    //! Wait for activity on any socket
    int activity = poll(fds, num_fds, -1); // -1 means no timeout
    if (activity < 0) {
        printf("poll() failed: %s\n", strerror(errno));
        return;
    }

    //! Check for activity on the server socket
    if (fds[0].revents & POLLIN) {
        // Accept a new connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock < 0) {
            if (current_time - last_log_time > 1000000) {
                last_log_time = current_time;
                ESP_LOGE(TAG, "Failed to accept connection: %s\n", strerror(errno));
            }

        } else {
            printf("New connection accepted\n");
            if (num_fds < MAX_CLIENTS + 1) {
                fds[num_fds].fd = client_sock;
                fds[num_fds].events = POLLIN; // Monitor for incoming data
                num_fds++;

                //! Add the new client socket to the pollfd array
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sock_arr[i] == -1) {
                        client_sock_arr[i] = client_sock;
                        printf("Client socket %d added\n", client_sock);
                        break;
                    }
                }
            } else {
                printf("Max clients reached, closing connection\n");
                close(client_sock);
            }
        }
    }

    //! Check for activity on client sockets
    for (int i = 1; i < num_fds; i++) {
        struct pollfd target = fds[i];
        int target_fd = target.fd;

        if (target.revents & POLLIN) {
            //! Handle client data
            handle_client_data(target_fd);
        }

        if (target.revents & POLLERR) {
            printf("Error on client socket %d\n", target_fd);
            close(target_fd);
            
            //! Remove the client socket with an error
            remove_client_socket(target_fd);

            //! Remove the socket from the pollfd array
            fds[i].fd = -1; // Mark as invalid
        }
    }
}


void web_socket_accept(uint64_t current_time) {
    if (status != WEBSOCKET_SETUP) return;

    // Accept incoming connection
    struct sockaddr_in client_addr;
    socklen_t socklen = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &socklen);
    if (client_sock < 0) {
        if (current_time - last_log_time > 1000000) {
            last_log_time = current_time;
            ESP_LOGE(TAG, "Failed to accept connection");
        }
        return;
    }
    ESP_LOGI(TAG, "Client connected");

    //! Set the socket to non-blocking mode
    int flags = fcntl(client_sock, F_GETFL, 0);
    fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);

    //! Set a timeout for recv
    struct timeval timeout;
    timeout.tv_sec = 0; // second timeout
    timeout.tv_usec = 1; // microseconds
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    //! Read the WebSocket handshake request
    int len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len <= 0) return;

    rx_buffer[len] = 0; // Null-terminate the received data
    ESP_LOGI(TAG, "Received handshake request: %s", rx_buffer);

    // Extract Sec-WebSocket-Key from the request
    char *key_start = strstr(rx_buffer, "Sec-WebSocket-Key: ");
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
        printf("SHA-1 computation failed\n");
        return;
    }

    //! Base64 encode the hash
    size_t base64_len;
    char accept_key[64];
    mbedtls_base64_encode((unsigned char *)accept_key, sizeof(accept_key), &base64_len, sha1, 20);

    // WebSocket handshake response
    const char *ws_handshake_response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n";

    //! Send the WebSocket handshake response
    char response[256];
    snprintf(response, sizeof(response), ws_handshake_response, accept_key);
    send(client_sock, response, strlen(response), 0);
    
    status = WEBSOCKET_HANDSHAKED;
}

void web_socket_task(uint64_t current_time) {
    if (status != WEBSOCKET_HANDSHAKED) return;

    uint8_t frame[128];
    int len = recv(client_sock, frame, sizeof(frame), 0);

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
        send_websocket_message(client_sock, response);
        
    } else if (len == 0) {
        ESP_LOGI(TAG, "Client disconnected");
    }
}