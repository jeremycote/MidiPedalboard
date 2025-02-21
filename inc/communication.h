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

void handle_incoming_packets();

#endif //MIDICONTROLLER_COMMUNICATION_H
