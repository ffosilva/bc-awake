# Design

## Context

See `proposal.md - Why`.

In normal operation mode (`normalBegin()`), `networkWakeBegin()` initializes Wi-Fi Station if `config.wolEnabled` and `config.wifiSsid` are present. If Network Wake is disabled (the default state on unconfigured devices), it logs that Wi-Fi station is off and calls `WiFi.mode(WIFI_OFF)`.

Currently, `normalBegin()` calls `portalServerBegin()` immediately after `networkWakeBegin()`. Because `server.begin()` in AsyncTCP relies on LwIP's `tcpip_api_call()`, attempting to bind a socket when the TCP/IP stack has not been initialized attempts to acquire a NULL FreeRTOS mutex (`lock_tcpip_core`), triggering `assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))`.

## Goals / Non-Goals

**Goals:**
- Eliminate the assertion failure and bootloop when starting in normal mode with Wi-Fi disabled or unconfigured.
- Start the persistent web server on port 80 only after Wi-Fi Station has successfully connected and obtained an IP address.
- Retain existing setup mode (`portalBegin()`) behavior where the web server starts immediately on the SoftAP interface.
- Add defensive guards to `portalServerBegin()` to prevent socket operations when Wi-Fi is off.

**Non-Goals:**
- Altering the SoftAP configuration or setup mode workflow.
- Changing REST endpoints, authentication tokens, or web UI assets.

## Decisions

### 1. Start Web Server in `networkWakeLoop()` Upon Wi-Fi Station Connection

- **Approach**: Remove `portalServerBegin()` from `normalBegin()` in `src/main.cpp`. Instead, invoke `portalServerBegin()` inside `networkWakeLoop()` in `src/network_wake.cpp` upon the connection transition `if (connected && !g_wasConnected)` alongside `MDNS.begin()` and `startUdpListeners()`.
- **Rationale**: The web server cannot service requests in normal mode without an active Station IP. Starting the server upon connection aligns with the original specification and ensures LwIP is fully active and assigned an IP.
- **Alternatives Considered**:
  - *Keep `portalServerBegin()` in `normalBegin()` and initialize Wi-Fi in dummy mode*: Rejected because initializing the network stack when the user explicitly disabled Wi-Fi wastes memory and radio state.

### 2. Defensive Guard Against `WIFI_OFF` in `portalServerBegin()`

- **Approach**: In `src/portal.cpp`, check `WiFi.getMode() == WIFI_OFF` at the entry of `portalServerBegin()` and return early without calling `server.begin()`.
- **Rationale**: Provides defense in depth to guarantee that even if called inadvertently before network initialization, `server.begin()` will never trigger a kernel panic. In setup mode (`portalBegin()`), `WiFi.softAP()` sets the mode to `WIFI_AP`, which safely bypasses the guard.

## Risks / Trade-offs

- **[Risk] Web portal inaccessible if Wi-Fi credentials fail in normal mode** → **Mitigation**: This is expected behavior; without network connectivity, no Station IP exists. Hardware power control, button sensing, and Bluetooth wake continue to operate uninterrupted. If reconfiguration is needed, the user can hold the button for 8 seconds to enter the SoftAP setup portal.
