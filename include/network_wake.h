#pragma once

#include <Arduino.h>
#include "board.h"

// Initializes network wake subsystem:
// Starts Wi-Fi in STA mode and binds the WOL UDP listener if wolEnabled is true and wifiSsid is set.
// Parameter isOff indicates whether the host is currently in STATE_OFF.
bool networkWakeBegin(bool isOff = true);

// Completely stops UDP listeners, disconnects Wi-Fi, and disables the Wi-Fi radio.
void networkWakeStop();

// Polling loop for non-blocking Wi-Fi station management and incoming WOL magic packet processing.
void networkWakeLoop();

// Returns true if a valid WOL magic packet matching config.bc250Mac was detected.
// If lastSeenMs is provided, writes the timestamp of the packet reception.
bool networkWakeSeen(unsigned long *lastSeenMs);

// Helper queries
bool networkWakeIsConnected();
String networkWakeIp();
