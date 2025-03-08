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

/**
 * Register MIDI Session control port with Bonjour
 * Creates a SRV and PTR DNS record.
 * TODO: This function will crash if already called
 * @return true if session was created successfully
 */
bool setup_bonjour();

void handle_incoming_packets();

#endif //MIDICONTROLLER_COMMUNICATION_H
