//
// Created by Jeremy Cote on 2025-02-13.
//

#ifndef MIDICONTROLLER_COMMUNICATION_H
#define MIDICONTROLLER_COMMUNICATION_H

#include <stdbool.h>

/**
 * Setup Wi-Fi and connect to network.
 * @return true if connected to network.
 */
bool setup_wifi();

/**
 * Open UDP Socket for RTP-MIDI communication
 * @return true if socket was created successfully
 */
bool setup_midi_server();

void send_invitation_response(int sock, struct sockaddr_in *addr, uint32_t initiator_token, bool accept);

void handle_incoming_packets(uint8_t *buffer, uint16_t buffer_len, struct sockaddr_in sender_addr, socklen_t sender_len);

#endif //MIDICONTROLLER_COMMUNICATION_H
