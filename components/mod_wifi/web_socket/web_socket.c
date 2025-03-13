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

// WebSocket handshake response
const char *websocket_handshake_response =
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: %s\r\n\r\n";

// Generate Sec-WebSocket-Accept key
static void generate_websocket_accept(const char *client_key, char *accept_key) {
    // Concatenate client key with WebSocket GUID
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", client_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    // Compute SHA-1 hash
    unsigned char sha1[20];
    mbedtls_sha1((unsigned char *)combined, strlen(combined), sha1);

    // Base64 encode the hash
    size_t len;
    mbedtls_base64_encode((unsigned char *)accept_key, 64, &len, sha1, 20);
}

// Send a WebSocket text frame to the client
static void send_websocket_message(int client_sock, const char *message) {
    size_t len = strlen(message);
    uint8_t frame[len + 2]; // Frame buffer (header + payload)

    // Construct the frame
    frame[0] = 0x81; // FIN bit set, opcode for text frame
    frame[1] = len;  // Payload length
    memcpy(frame + 2, message, len); // Copy the payload

    // Send the frame
    send(client_sock, frame, len + 2, 0);
}

// Handle WebSocket frame and send a response
static void handle_websocket_frame(int client_sock, uint8_t *frame, size_t len) {
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
}

int server_sock;
struct sockaddr_in server_addr;
char rx_buffer[512];
web_socket_status_t status = WEBSOCKET_DISCONNECTED;

void web_socket_setup(void) {
    if (status != WEBSOCKET_DISCONNECTED) return;

    // Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    }

    // Bind the socket to the port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(server_sock);
        return;
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        ESP_LOGE(TAG, "Failed to listen on socket");
        close(server_sock);
        return;
    }

    //! Set the socket to non-blocking mode
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);

    ESP_LOGI(TAG, "WebSocket server started on port %d", PORT);
    status = WEBSOCKET_SETUP;
}

int client_sock;
uint64_t last_log_time;

void web_socket_handshake(uint64_t current_time) {
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

    // Read the WebSocket handshake request
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
    char accept_key[64];
    generate_websocket_accept(key_start, accept_key);

    // Send the WebSocket handshake response
    char response[256];
    snprintf(response, sizeof(response), websocket_handshake_response, accept_key);
    send(client_sock, response, strlen(response), 0);
    
    status = WEBSOCKET_HANDSHAKED;
}

void web_socket_task(uint64_t current_time) {
    if (status != WEBSOCKET_HANDSHAKED) return;

    uint8_t frame[128];
    int len = recv(client_sock, frame, sizeof(frame), 0);
    if (len > 0) {
        handle_websocket_frame(client_sock, frame, len);
    } else if (len == 0) {
        ESP_LOGI(TAG, "Client disconnected");
    }
}