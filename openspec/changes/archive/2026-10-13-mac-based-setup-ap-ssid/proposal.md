# Proposal

## Why

Currently, the setup SoftAP uses a fixed, hardcoded SSID (`BC Awake Setup`). When multiple devices operate in the same vicinity, this leads to naming collisions and ambiguity regarding which board is in setup mode. Deriving the SoftAP SSID from the device's factory MAC address guarantees that every device broadcasts an unambiguous, unique network name without requiring extra configuration fields, UI complexity, or persistent NVS storage.

## What Changes

- Dynamically generate the setup mode SoftAP SSID at startup using the prefix `BC250_` concatenated with the 6-character uppercase hexadecimal string of the last 3 bytes of the board's factory eFuse MAC address (e.g. `BC250_3A9F12`).
- Update `portalBegin()` in `src/portal.cpp` to read the factory eFuse MAC address and configure the SoftAP with the dynamic SSID string.
- Update `include/board.h` constants to define the AP SSID prefix rather than a fixed static network name.
- Update the `setup-portal` specification to document the dynamic MAC-based SoftAP naming convention.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `setup-portal`: Update the SoftAP network and web serving requirement so that the broadcasted SSID dynamically incorporates the last 3 bytes of the factory MAC address as `BC250_<SUFFIX>`.

## Impact

- **Firmware code**: `src/portal.cpp` (SSID formatting and `WiFi.softAP()` invocation), `include/board.h` (`AP_SSID_PREFIX`).
- **User experience**: When entering setup mode, the device broadcasts `BC250_<SUFFIX>` (where `<SUFFIX>` matches the last 6 hex characters of the MAC address printed on the ESP32 module), preventing confusion across multiple installations.
- **Compatibility**: The network remains an open Wi-Fi SoftAP protected by HTTP password authentication; no changes to REST APIs or Station mode.
