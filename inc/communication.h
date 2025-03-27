//
// Created by Jeremy Cote on 2025-02-13.
//

#ifndef MIDICONTROLLER_COMMUNICATION_H
#define MIDICONTROLLER_COMMUNICATION_H

#include <stdbool.h>

/**
 * Setup Wi-Fi.
 * @return true if setup
 */
bool setup_wifi();

/**
 * Connect to Wi-Fi network
 * @return
 */
bool connect_wifi();

/**
 * Open UDP Socket for RTP-MIDI communication
 * @return true if socket was created successfully
 */
bool setup_midi_server();

/**
 * Register MIDI Session control port with Bonjour
 * Creates a SRV and PTR DNS record.
 * TODO: This function will crash if already called
 * @return true if session was created successfully
 */
bool setup_bonjour();

void send_midi_control_change(uint8_t control, uint8_t value);

void send_midi_resync();

void handle_incoming_packets();

bool is_connected();

#endif //MIDICONTROLLER_COMMUNICATION_H
