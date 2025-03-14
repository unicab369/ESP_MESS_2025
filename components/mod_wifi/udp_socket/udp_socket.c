#include "udp_socket.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "regex.h"
#include "esp_nan.h"
#include "esp_log.h"
#include "esp_err.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#define PORT 3333

static const char *TAG = "UDP_STUFF";
static int listening_socket;
static uint8_t retry_count = 5;

struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
socklen_t socklen = sizeof(source_addr);
static uint64_t last_update_time = 0;
struct sockaddr_in6 dest_addr;

udp_status_t current_status;

void udp_close() {
    // Close the socket
    shutdown(listening_socket, 0);
    close(listening_socket);
}

udp_status_t udp_server_socket_setup(uint64_t current_time) {
    if (current_status == UDP_STATUS_SETUP) return current_status;
    if (current_time - last_update_time < 1000000) return current_status;
    last_update_time = current_time;

    printf("IM HERE 333");

    int addr_family = AF_INET;
    int ip_protocol = 0;
    

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    } else if (addr_family == AF_INET6) {
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }

    listening_socket = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (listening_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        current_status = UDP_STATUS_SOCKET_FAILED;
    }
    ESP_LOGI(TAG, "Socket created");

    #if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        int enable = 1;
        lwip_setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
    #endif

    #if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        if (addr_family == AF_INET6) {
            // Note that by default IPV6 binds to both protocols, it is must be disabled
            // if both protocols used at the same time (used in CI)
            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        }
    #endif

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt (listening_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    int err = bind(listening_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        current_status = UDP_STATUS_BINDING_FAILED;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    // #if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
    //         struct iovec iov;
    //         struct msghdr msg;
    //         struct cmsghdr *cmsgtmp;
    //         u8_t cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];

    //         iov.iov_base = rx_buffer;
    //         iov.iov_len = sizeof(rx_buffer);
    //         msg.msg_control = cmsg_buf;
    //         msg.msg_controllen = sizeof(cmsg_buf);
    //         msg.msg_flags = 0;
    //         msg.msg_iov = &iov;
    //         msg.msg_iovlen = 1;
    //         msg.msg_name = (struct sockaddr *)&source_addr;
    //         msg.msg_namelen = socklen;
    // #endif
    
    current_status = UDP_STATUS_SETUP;
    return current_status;
}

static char rx_buffer[128];
static char addr_str[128];

void udp_server_socket_task()
{
    if (current_status != UDP_STATUS_SETUP) return;

    // int err = bind(listening_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    // if (err < 0) {
    //     ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    //     // current_status = UDP_STATUS_BINDING_FAILED;
    // }
    // ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    #if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
            struct iovec iov;
            struct msghdr msg;
            struct cmsghdr *cmsgtmp;
            u8_t cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];

            iov.iov_base = rx_buffer;
            iov.iov_len = sizeof(rx_buffer);
            msg.msg_control = cmsg_buf;
            msg.msg_controllen = sizeof(cmsg_buf);
            msg.msg_flags = 0;
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;
            msg.msg_name = (struct sockaddr *)&source_addr;
            msg.msg_namelen = socklen;
    #endif

    #if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        int len = recvmsg(server_socket, &msg, 0);
    #else
        int len = recvfrom(listening_socket, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
    #endif

    if (len < 0) return;

    // Data received
    else {
        // Get the sender's ip address as string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            #if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
                for ( cmsgtmp = CMSG_FIRSTHDR(&msg); cmsgtmp != NULL; cmsgtmp = CMSG_NXTHDR(&msg, cmsgtmp) ) {
                    if ( cmsgtmp->cmsg_level == IPPROTO_IP && cmsgtmp->cmsg_type == IP_PKTINFO ) {
                        struct in_pktinfo *pktinfo;
                        pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsgtmp);
                        ESP_LOGI(TAG, "dest ip: %s", inet_ntoa(pktinfo->ipi_addr));
                    }
                }
            #endif
        } else if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }

        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
        ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
        ESP_LOGI(TAG, "%s", rx_buffer);

        int err = sendto(listening_socket, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        }
    }
}

#define MESSAGE_UDP "Hello, UDP!"
#define BROADCAST_IP "10.0.0.255" 

uint64_t last_sent_time = 0;

void udp_client_socket_send(uint64_t current_time) {
    if (current_status != UDP_STATUS_SETUP) return;
    if (current_time - last_sent_time < 1000000) return;
    last_sent_time = current_time;

    int sock;
    
    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return;
    }

    struct sockaddr_in dest_addr;

    // Configure destination address
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, BROADCAST_IP, &dest_addr.sin_addr);

    // Send UDP message
    int err = sendto(sock, MESSAGE_UDP, strlen(MESSAGE_UDP), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to send UDP message: errno %d", errno);
    } else {
        ESP_LOGI(TAG, "UDP message sent: %s", MESSAGE_UDP);
    }

    // Close socket
    // close(sock);
}

