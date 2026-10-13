#pragma once

#include <Arduino.h>

// Initializes network wake subsystem:
// Starts Wi-Fi in STA mode and binds the WOL UDP listener if wolEnabled is true and wifiSsid is set.
// Returns true if network wake is enabled and active.
bool networkWakeBegin();

// Polling loop for non-blocking Wi-Fi station management and incoming WOL magic packet processing.
void networkWakeLoop();

// Returns true if a valid WOL magic packet matching config.bc250Mac was detected.
// If lastSeenMs is provided, writes the timestamp of the packet reception.
bool networkWakeSeen(unsigned long *lastSeenMs);

// Helper queries
bool networkWakeIsConnected();
String networkWakeIp();
