# Proposal

## Why

On a freshly flashed or reset ESP32 (or whenever Wake-on-LAN is disabled / Wi-Fi is unconfigured), `normalBegin()` executes `portalServerBegin()` immediately after `networkWakeBegin()` turns Wi-Fi off (`WiFi.mode(WIFI_OFF)`). This causes `AsyncServer::begin()` to invoke LwIP socket binding functions (`tcpip_api_call`) while the TCP/IP stack is uninitialized, triggering a FreeRTOS kernel panic (`assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))`). Because this failure occurs in `setup()`, the device resets every ~100 ms in an inescapable bootloop, preventing even the 8-second button hold to enter setup mode.

## What Changes

- Defer starting the HTTP web portal in normal operation until Wi-Fi Station has successfully connected and obtained an IP address.
- Add a defensive guard inside `portalServerBegin()` to prevent starting the HTTP server whenever Wi-Fi is in `WIFI_OFF` mode.
- Ensure devices with Network Wake disabled (`wolEnabled = false`) or unconfigured Wi-Fi boot cleanly into normal operation without crashing.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `setup-portal`: Explicitly require that in normal operation mode the HTTP web server only activates after Wi-Fi Station connectivity is established, and remains offline/deferred when Wi-Fi is disabled or not connected.

## Impact

- `src/main.cpp`: Remove the premature call to `portalServerBegin()` from `normalBegin()`.
- `src/network_wake.cpp`: Call `portalServerBegin()` when Wi-Fi Station connection is established.
- `src/portal.cpp`: Add a safety check in `portalServerBegin()` against `WiFi.getMode() == WIFI_OFF`.
- No breaking API changes or persistent configuration schema changes.
