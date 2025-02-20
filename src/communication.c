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
#define CMD_INVITATION (('I' << 8) | ('N'))
#define CMD_ACCEPT (('O' << 8) | ('K'))
#define CMD_REJECT (('N' << 8) | ('O'))
#define CMD_EXIT (('B' << 8) | ('Y'))
#define CMD_CLOCK (('C' << 8) | ('K'))

typedef struct {
    uint16_t signature;          // Two bytes of padding as 0xFFFF
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

    int flags = lwip_fcntl(session.control_socket, F_GETFL, 0);
    lwip_fcntl(session.control_socket, F_SETFL, flags | O_NONBLOCK);

    // Create MIDI socket
    session.midi_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (session.midi_socket < 0) {
        printf("Failed to create MIDI socket\n");
        close(session.control_socket);
        return false;
    }

    flags = lwip_fcntl(session.midi_socket, F_GETFL, 0);
    lwip_fcntl(session.midi_socket, F_SETFL, flags | O_NONBLOCK);

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
    response.signature = 0xFFFF;
    response.command = htons(accept ? CMD_ACCEPT : CMD_REJECT);
    response.protocol_version = PROTOCOL_VERSION << 24;
    response.initiator_token = initiator_token;  // Use initiator's token
    response.ssrc = htonl(session.ssrc);

    if (accept) {
        strcpy(response.name, "Pico W MIDI");
    }

    sendto(sock, &response, 16 + 12, 0,
           (struct sockaddr*)addr, sizeof(*addr));
}

static bool is_exchange_packet(const uint8_t *buffer) {
    return (buffer[0] & 0xFF) && (buffer[1] & 0xFF);
}

static void handle_invitation(exchange_packet_t* packet, struct sockaddr_in sender_addr) {
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

static void handle_exchange_packet(exchange_packet_t* packet, struct sockaddr_in sender_addr) {
    printf("Received exchange packet from %s:%d\n", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));

    uint16_t command = ntohs(packet->command);
    printf("Received command: %c%c\n", command >> 8, command);

    switch (command) {
        case CMD_INVITATION: {
            handle_invitation(packet, sender_addr);
            return;
        }
        case CMD_CLOCK: {
            // TODO: Implement CK command
            return;
        }
        default: {
            printf("Unknown exchange packet received. Ignoring %c%c.\n", command >> 8, command);
            return;
        }
    }
}

void handle_incoming_packets(uint8_t *buffer, uint16_t buffer_len, struct sockaddr_in sender_addr, socklen_t sender_len) {
    // Check control socket
    int received = recvfrom(session.control_socket, buffer, buffer_len, 0,
                            (struct sockaddr*)&sender_addr, &sender_len);
    if (received > 0) {
        if (!is_exchange_packet(buffer)) {
            printf("Control Socket only accepts exchange packets\n");
            goto check_midi;
        }

        // Interpret the buffer as an exchange packet
        exchange_packet_t* packet = (exchange_packet_t*)buffer;
        handle_exchange_packet(packet, sender_addr);
    }

    check_midi:
        // Check MIDI socket for actual MIDI messages
        received = recvfrom(session.midi_socket, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&sender_addr, &sender_len);

        if (received > 0 && session.connected) {
            if (is_exchange_packet(buffer)) {
                exchange_packet_t* packet = (exchange_packet_t*)buffer;
                handle_exchange_packet(packet, sender_addr);
                return;
            }
        }
}