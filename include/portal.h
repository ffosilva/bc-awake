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

// Request power on from the web portal. Safe to call from the web server task;
// returns false if the board is not in STATE_OFF.
bool requestPowerOn();

// Request force power off from the web portal. Safe to call from the web server task;
// returns false if the board is already in STATE_OFF.
bool requestPowerOff();

// Request a graceful shutdown (300 ms PWR_BTN# pulse). Safe to call from the web
// server task; returns false if the feature is disabled or the board is not ON.
bool requestGracefulShutdown();

// Interactive pairing mode state
bool portalIsPairingActive();
void portalPairingStop();

// Wake cooldown arming implemented by main.cpp
void armWakeCooldown(unsigned long durationMs = WAKE_WAKE_COOLDOWN_MS);

#if defined(BOARD_ESP32CAM)
// Flash burst feedback on ESP32-CAM targets implemented by main.cpp
void triggerFlashBurst(int count = 3, unsigned long onMs = 60, unsigned long offMs = 60);
#endif
