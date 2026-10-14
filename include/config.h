#pragma once

#include <Arduino.h>
#include <vector>


// Bound wireless gamepad structure.
//   mac  - controller BD_ADDR/MAC in lower-case colon form ("aa:bb:..").
//   name - human-readable friendly name (e.g. "DualSense - Living Room").
struct BoundController {
  String mac;
  String name;
};

const size_t MAX_CONTROLLERS = 8;

// Persistent configuration, stored in NVS via the Preferences library.
//   controllers - array of bound controllers (Classic BT, up to 8).
//   wakeAddr    - primary bound controller BD_ADDR/MAC (mirrored from controllers[0]).
//   hostAddr    - BC250 Bluetooth adapter BD_ADDR (Classic BT only).
//   passHash    - SHA-256 hex of the portal password ("" => not set yet).
//   forceSetup  - request that the next boot enters the WiFi setup portal.
//   flashOnWake - pulse ESP32-CAM flash LED upon controller wake.
//   wolEnabled  - enable Wake-on-LAN listener over Wi-Fi station mode.
//   wifiSsid    - Wi-Fi network SSID to connect to in normal mode.
//   wifiPassword- Wi-Fi network password (empty for open networks).
//   bc250Mac    - BC250 Ethernet MAC address for WOL magic packet filtering.
//   hostname    - device mDNS and DHCP hostname (default: "bc250-switch").
//   gracefulShutdown - optional: mirror the power button to the motherboard
//                      PWR_BTN# line while ON and allow portal-triggered
//                      shutdown pulses (default: disabled).
struct Config {
  std::vector<BoundController> controllers;
  String wakeAddr;
  String hostAddr;
  String passHash;
  bool   forceSetup;
  bool   flashOnWake;
  bool   wolEnabled;
  String wifiSsid;
  String wifiPassword;
  String bc250Mac;
  String hostname;
  bool   gracefulShutdown;
};

extern Config config;

void loadConfig();

void setControllers(const std::vector<BoundController> &controllers);
bool addController(const String &mac, const String &name);
bool removeController(const String &mac);
bool renameController(const String &mac, const String &name);

void setWakeAddr(const String &addr);
void setHostAddr(const String &addr);
void setPassHash(const String &hash);
void setForceSetup(bool force);
void setFlashOnWake(bool enable);
void setWolEnabled(bool enable);
void setGracefulShutdown(bool enable);
void setWifiConfig(const String &ssid, const String &password, const String &bc250Mac, const String &hostname = "");
bool isValidHostname(const String &name);
void setHostname(const String &hostname);

// Fully provisioned: a password and controller(s) are set (plus host adapter if Classic BT).
bool isConfigured();

// Reset all configuration in NVS and in-memory back to defaults, setting forceSetup to true.
void resetConfig();

