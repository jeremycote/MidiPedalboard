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
#define CMD_RECEIVER_FEEDBACK (('R' << 8) | ('S'))

typedef struct {
    uint16_t signature;          // Two bytes of padding as 0xFFFF
    uint16_t command;          // Two ASCII chars
    uint32_t protocol_version; // Should be 2
    uint32_t initiator_token;  // Random number from initiator
    uint32_t ssrc;            // Sync source identifier
    char name[32];            // UTF-8 device name
} exchange_packet_t;

typedef struct {
    uint16_t signature;
    uint16_t command;
    uint32_t ssrc;
    uint8_t count;
    uint8_t unused[3];
    // Timestamp1
    uint32_t timestamps[6];
} timestamp_packet_t;

// TODO: What are these fields?
typedef struct {
    union {
        struct {
            uint8_t cc : 4;
            uint8_t x  : 1;
            uint8_t p  : 1;
            uint8_t v  : 2;
        };
        uint8_t reserved_upper;
    };

    union {
        struct {
            uint8_t pt : 7;
            uint8_t m  : 1;
        };
        uint8_t reserved_lower;
    };

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
} midi_packet_header_t;

typedef struct {
    union {
        struct {
            uint8_t len : 4;
            uint8_t p : 1;
            uint8_t z : 1;
            uint8_t j : 1;
            uint8_t b : 1;
        };
        uint8_t all;
    };
    uint8_t command_list[];
} midi_packet_command_section_t;

typedef struct {
    int control_socket;
    int midi_socket;
    struct sockaddr_in control_addr;
    struct sockaddr_in midi_addr;
    uint32_t ssrc;
    uint32_t remote_ssrc;
    uint32_t initiator_token;
    bool connected;
    uint16_t sequence_number_host;
    uint16_t sequence_number_delta;
} midi_session_t;

#define BUFFER_LEN 1024
static uint8_t buffer[BUFFER_LEN];

static midi_session_t session = {0};
static struct sockaddr_in sender_addr;
static socklen_t sender_len = sizeof(sender_addr);

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

// Helper function to get current timestamp in 100 microsecond units
static uint64_t get_timestamp() {
    absolute_time_t now = get_absolute_time();
    return to_us_since_boot(now) / 100;
}

static void send_invitation_response(int socket, struct sockaddr_in *addr, uint32_t initiator_token, bool accept) {
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

    sendto(socket, &response, 16 + 12, 0,
           (struct sockaddr*)addr, sizeof(*addr));
}

static bool is_exchange_packet(const uint8_t *buffer) {
    return (buffer[0] == 0xFF) && (buffer[1] == 0xFF);
}

static void handle_invitation(int socket, exchange_packet_t* packet, struct sockaddr_in *sender_addr) {
    printf("Received invitation from %s\n", packet->name);

    // Store initiator information
    session.initiator_token = ntohl(packet->initiator_token);
    session.remote_ssrc = ntohl(packet->ssrc);
    session.control_addr = *sender_addr;

    printf("Received invitation with token %lu\n", session.initiator_token);

    // Send acceptance on control port
    send_invitation_response(socket, sender_addr,
                             packet->initiator_token, true);

    session.connected = true;
    printf("Connection established with %s\n", packet->name);
}

static void handle_clock(timestamp_packet_t* packet, struct sockaddr_in *sender_addr) {
    printf("Handling clock\n");

    uint8_t count = packet->count + 1 > 2 ? 2 : packet->count + 1;

    if (count != 1) {
        return;
    }

    // Send back local time
    timestamp_packet_t response = {0};
    response.signature = 0xFFFF;
    response.command = htons(CMD_CLOCK);
    response.ssrc = htonl(session.ssrc);
    response.count = 1;
    response.timestamps[0] = packet->timestamps[0];
    response.timestamps[1] = packet->timestamps[1];
    response.timestamps[2] = htonl(get_timestamp() >> 32);
    response.timestamps[3] = htonl(get_timestamp());

    sendto(session.midi_socket, &response, 36, 0,
           (struct sockaddr*)sender_addr, sizeof(*sender_addr));
}

static void handle_exchange_packet(int socket, exchange_packet_t* packet, struct sockaddr_in *sender_addr) {
    printf("Received exchange packet from %s:%d\n", inet_ntoa(sender_addr->sin_addr), ntohs(sender_addr->sin_port));

    uint16_t command = ntohs(packet->command);
    printf("Received command: %c%c\n", command >> 8, command);

    switch (command) {
        case CMD_INVITATION: {
            handle_invitation(socket, packet, sender_addr);
            return;
        }
        case CMD_CLOCK: {
            handle_clock((timestamp_packet_t*)packet, sender_addr);
            return;
        }
        case CMD_RECEIVER_FEEDBACK: {
            return;
        }
        default: {
            printf("Unknown exchange packet received. Ignoring %c%c.\n", command >> 8, command);
            return;
        }
    }
}

void send_midi(struct sockaddr_in *sender_addr) {
    printf("Sending midi command.\n");
    printf("Current time: %llu\n", get_timestamp());
    uint8_t command[128];

    midi_packet_header_t *header = (midi_packet_header_t*)command;
    midi_packet_command_section_t *command_section = (midi_packet_command_section_t*)(command + 12);

    header->v = 2;
    header->p = 0;
    header->x = 0;
    header->cc = 0;
    header->m = 0;
    header->pt = 0x61;

    header->sequence_number = htons(++session.sequence_number_delta + session.sequence_number_host);
    header->timestamp = htonl(get_timestamp());
    header->ssrc = htonl(session.ssrc);

    command_section->b = 0;
    command_section->j = 0;
    command_section->z = 0;
    command_section->p = 0;

    command_section->len = 3;

    command_section->command_list[0] = 0x90;
    command_section->command_list[1] = 60;
    command_section->command_list[2] = 0x7F;

    sendto(session.midi_socket, command, 12 + 4, 0,
           (struct sockaddr*)sender_addr, sizeof(*sender_addr));
}

void handle_incoming_packets() {
    int received;

    check_control:
        // Check control socket
        received = recvfrom(session.control_socket, buffer, BUFFER_LEN, 0,
                                (struct sockaddr*)&sender_addr, &sender_len);
        if (received > 0) {
            if (!is_exchange_packet(buffer)) {
                printf("Control Socket only accepts exchange packets\n");
                goto check_midi;
            }

            // Interpret the buffer as an exchange packet
            exchange_packet_t* packet = (exchange_packet_t*)buffer;
            handle_exchange_packet(session.control_socket, packet, &sender_addr);
        }

    check_midi:
        // Check MIDI socket for actual MIDI messages
        received = recvfrom(session.midi_socket, buffer, BUFFER_LEN, 0,
                            (struct sockaddr*)&sender_addr, &sender_len);

        if (received > 0 && session.connected) {
            if (is_exchange_packet(buffer)) {
                exchange_packet_t* packet = (exchange_packet_t*)buffer;
                handle_exchange_packet(session.midi_socket, packet, &sender_addr);
                return;
            }

            // Interpret as midi note
            midi_packet_header_t *header = (midi_packet_header_t*)buffer;

            // TODO: Handle wrap around
            int32_t delta = ntohs(header->sequence_number) - session.sequence_number_host;
            session.sequence_number_host = ntohs(header->sequence_number);

            session.sequence_number_delta = delta > session.sequence_number_delta ? 0 : session.sequence_number_delta - delta;

            printf("Updated sequence number from host: %d\n", session.sequence_number_host);
        }

    static uint16_t count = 0;
    if (session.connected) {
        if (count++ > 10000) {
            send_midi(&sender_addr);
            count = 0;
        }
    }
}