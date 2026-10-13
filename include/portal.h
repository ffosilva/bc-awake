#pragma once

// Setup mode (SoftAP + captive portal DNS + discovery)
void portalBegin();
void portalLoop();

// Normal mode persistent web server
void portalServerBegin();
void portalCheckRestart();

// Board state provider implemented by main.cpp
const char *getBoardStateName();
