#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Protocol constants
#define CONTROL_PORT 5004
#define MIDI_PORT 5005
#define PROTOCOL_VERSION 2

// Packet types
#define CMD_INVITATION "IN"
#define CMD_ACCEPT "OK"
#define CMD_REJECT "NO"
#define CMD_EXIT "BY"

typedef struct {
    uint16_t padding;          // Two bytes of padding as 0xFFFF
    uint16_t command;          // Two ASCII chars
    uint32_t protocol_version; // Should be 2
    uint32_t initiator_token;  // Random number from initiator
    uint32_t ssrc;            // Sync source identifier
    char name[32];            // UTF-8 device name
} exchange_packet_t;

typedef struct {
    int control_socket;
    int midi_socket;
    struct sockaddr_in control_addr;
    struct sockaddr_in midi_addr;
    uint32_t ssrc;
    uint32_t remote_ssrc;
    uint32_t initiator_token;
    bool connected;
    int64_t clock_offset;
} midi_session_t;

static midi_session_t session = {0};

bool setup_wifi() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_CANADA)) {
        printf("Wi-Fi init failed\n");
        return false;
    }

    printf("Wifi initialized\n");
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Wi-Fi connection failed\n");
        return false;
    }

    // Get and print IP address
    ip4_addr_t ip = cyw43_state.netif->ip_addr;
    printf("Connected to Wi-Fi\n");
    printf("Pico W IP Address: %d.%d.%d.%d\n",
           ip4_addr1(&ip), ip4_addr2(&ip),
           ip4_addr3(&ip), ip4_addr4(&ip));

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return true;
}

bool setup_midi_server() {
    session.ssrc = rand();  // Generate random SSRC
    session.connected = false;

    // Create control socket
    session.control_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (session.control_socket < 0) {
        printf("Failed to create control socket\n");
        return false;
    }

    // Create MIDI socket
    session.midi_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (session.midi_socket < 0) {
        printf("Failed to create MIDI socket\n");
        close(session.control_socket);
        return false;
    }

    // Bind control socket
    struct sockaddr_in control_bind = {
            .sin_family = AF_INET,
            .sin_port = htons(CONTROL_PORT),
            .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(session.control_socket, (struct sockaddr*)&control_bind, sizeof(control_bind)) < 0) {
        printf("Failed to bind control socket\n");
        return false;
    }

    // Bind MIDI socket
    struct sockaddr_in midi_bind = {
            .sin_family = AF_INET,
            .sin_port = htons(MIDI_PORT),
            .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(session.midi_socket, (struct sockaddr*)&midi_bind, sizeof(midi_bind)) < 0) {
        printf("Failed to bind MIDI socket\n");
        return false;
    }

    printf("MIDI server listening on ports %d (control) and %d (MIDI)\n",
           CONTROL_PORT, MIDI_PORT);
    return true;
}

void send_invitation_response(int sock, struct sockaddr_in *addr, uint32_t initiator_token, bool accept) {
    printf("Sending invitation response. Accept=%d\n", accept);
    exchange_packet_t response = {0};
    response.padding = 0xFFFF;
    response.command = htons(accept ? ((CMD_ACCEPT[0] << 8) | CMD_ACCEPT[1])
                               : ((CMD_REJECT[0] << 8) | CMD_REJECT[1]));
    response.protocol_version = PROTOCOL_VERSION << 24;
    response.initiator_token = initiator_token;  // Use initiator's token
    response.ssrc = htonl(session.ssrc);

    if (accept) {
        strcpy(response.name, "Pico W MIDI");
    }

    sendto(sock, &response, 16 + 12, 0,
           (struct sockaddr*)addr, sizeof(*addr));
}

void handle_incoming_packets(uint8_t *buffer, uint16_t buffer_len, struct sockaddr_in sender_addr, socklen_t sender_len) {
    // Check control socket
    int received = recvfrom(session.control_socket, buffer, buffer_len, 0,
                            (struct sockaddr*)&sender_addr, &sender_len);

    if (received > 0) {

        buffer[buffer_len] = '\0'; // Null-terminate for printing

        printf("Received exchange packet from %s:%d - %s\n",
               inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);

        exchange_packet_t* packet = (exchange_packet_t*)buffer;

        uint16_t padding = ntohs(packet->padding);
        if (padding != 0xFFFF) {
            printf("Invalid start to exchange packet\n");
            goto end;
        }

        uint16_t command = ntohs(packet->command);

        printf("Received command: %c%c token: %lu\n", command >> 8, command, ntohl(packet->initiator_token));

        // Handle invitation
        if (command == ((CMD_INVITATION[0] << 8) | CMD_INVITATION[1])) {
            printf("Received invitation from %s\n", packet->name);

            // Store initiator information
            session.initiator_token = ntohl(packet->initiator_token);
            session.remote_ssrc = ntohl(packet->ssrc);
            session.control_addr = sender_addr;

            printf("Received invitation with token %lu\n", session.initiator_token);

            // Send acceptance on control port
            send_invitation_response(session.control_socket, &sender_addr,
                                     packet->initiator_token, true);

            session.connected = true;
            printf("Connection established with %s\n", packet->name);
        }
    }

    // Check MIDI socket for actual MIDI messages
//    received = recvfrom(session.midi_socket, buffer, sizeof(buffer), 0,
//                        (struct sockaddr*)&sender_addr, &sender_len);
//
//    if (received > 0 && session.connected) {
//        // First byte of MIDI message is in buffer[12] due to RTP header
//        uint8_t status = buffer[12];
//        uint8_t data1 = buffer[13];
//        uint8_t data2 = buffer[14];
//
//        printf("Received MIDI message: status=0x%02X, data1=0x%02X, data2=0x%02X\n",
//               status, data1, data2);
//    }

    end:
        return;
}