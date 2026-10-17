# Design

## Context

See proposal.md - Why.
In `src/network_wake.cpp`, station Wi-Fi startup and runtime MAC swapping are handled by `networkWakeBegin(bool isOff)` and `applyStateMacChange(bool isOff)`. During runtime power transitions, `WiFi.disconnect(true)` triggers `STA.end()`, which puts the Wi-Fi mode into `WIFI_MODE_NULL` (0) and runs `esp_wifi_deinit()`. Because `esp_wifi_set_mac()` was called before `WiFi.mode(WIFI_STA)` re-initialized the driver, ESP-IDF failed with `ESP_ERR_WIFI_NOT_INIT` (`12289`).

## Goals / Non-Goals

**Goals:**
- Ensure the ESP32 Wi-Fi station interface is properly initialized before setting a target MAC address.
- Successfully swap to factory MAC during `STATE_BOOTING`/`STATE_ON` and back to the host Wi-Fi dongle MAC (`bc250Mac`) during `STATE_OFF`.
- Eliminate code duplication between initial boot startup and runtime state transitions.

**Non-Goals:**
- Changing SoftAP fallback logic, credentials storage, or setup portal functionality.
- Altering the 500 ms MAC swap deferral window during boot.

## Decisions

### 1. Re-initialize STA mode before calling `esp_wifi_set_mac()`
- **Approach**: Ensure `WiFi.mode(WIFI_STA)` is called before invoking `esp_wifi_set_mac(WIFI_IF_STA, targetMac)`.
- **Rationale**: `WiFi.mode(WIFI_STA)` calls `esp_wifi_init()` and sets up the station network interface without initiating a connection. Setting the MAC address at this stage complies with ESP-IDF's interface state requirements and allows `WiFi.begin()` to connect with the target MAC.
- **Alternatives Considered**:
  - *Directly calling `esp_wifi_stop()` and `esp_wifi_start()`*: Desynchronizes Arduino's internal `_esp_wifi_started` state tracking in `WiFiGeneric.cpp`, leading to missed connection starts.
  - *`WiFi.disconnect(false)`*: Does not fully reset the station interface state in all core versions.

### 2. Reuse `networkWakeBegin(isOff)` inside `applyStateMacChange(isOff)`
- **Approach**: After stopping listeners and disconnecting with `WiFi.disconnect(true)`, call `networkWakeBegin(isOff)` to perform the initialization, MAC assignment, and reconnection.
- **Rationale**: `networkWakeBegin(isOff)` already implements the complete, proven station startup sequence with hostname, MAC adoption, open/WPA connection, and TX power limiting. Delegating to it keeps the implementation DRY and guarantees that boot and runtime transitions never drift.
- **Alternatives Considered**:
  - *Reordering lines within a separate duplicated block*: Leaves ~25 lines of duplicate code in `network_wake.cpp`, which caused the bug originally.

## Risks / Trade-offs

- **[Risk] Brief station disconnection during MAC swap**
  - *Mitigation*: Existing behavior; the 500 ms deferral ensures PSU inrush is finished, and the transition takes only 1.5–3 seconds, well before the host OS completes BIOS POST and initializes its own Wi-Fi dongle.
