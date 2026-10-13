#pragma once

#include <Arduino.h>
#include "board.h"

// Setup mode (SoftAP + discovery)
void portalBegin();
void portalLoop();

// Normal mode persistent web server
void portalServerBegin();
void portalCheckRestart();

// Board state provider implemented by main.cpp
const char *getBoardStateName();

// Interactive pairing mode state
bool portalIsPairingActive();
void portalPairingStop();

// Wake cooldown arming implemented by main.cpp
void armWakeCooldown(unsigned long durationMs = WAKE_WAKE_COOLDOWN_MS);

#if defined(BOARD_ESP32CAM)
// Flash burst feedback on ESP32-CAM targets implemented by main.cpp
void triggerFlashBurst(int count = 3, unsigned long onMs = 60, unsigned long offMs = 60);
#endif
