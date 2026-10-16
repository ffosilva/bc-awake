# Design

## Context

See `proposal.md` for motivation. Currently in `include/board.h`:
```cpp
const char *const AP_SSID = "BC Awake Setup";
```
And in `src/portal.cpp`:
```cpp
bool ok = WiFi.softAP(AP_SSID);
```
All devices running the firmware broadcast this identical name when placed into setup mode.

## Goals / Non-Goals

**Goals:**
- Dynamically generate the SoftAP SSID during `portalBegin()` as `BC250_<SUFFIX>`, where `<SUFFIX>` is the 6-character uppercase hex string of the last 3 bytes of the factory eFuse MAC address (e.g., `BC250_3A9F12`).
- Replace the static `AP_SSID` constant with an `AP_SSID_PREFIX` constant in `include/board.h`.
- Maintain visible open SoftAP broadcast with HTTP password protection.
- Print the generated SSID to Serial when setup mode starts.

**Non-Goals:**
- Allowing user customization of the SSID via NVS or web UI.
- Adding a WPA2 passphrase to the setup SoftAP.
- Changing Station mode or network wake behavior.

## Decisions

### Decision 1: Use factory eFuse MAC address via `esp_read_mac()`
- **Rationale**: `esp_read_mac(mac, ESP_MAC_WIFI_STA)` reads the immutable factory-burned hardware MAC directly from eFuse. This ensures that the generated suffix is permanent, physically unique to each ESP32 module, and unaffected by any Station-mode MAC manipulation used in power-off wake states.
- **Alternatives Considered**:
  - *`WiFi.macAddress()`*: Requires the Wi-Fi subsystem to be active and can reflect software-overridden addresses. `esp_read_mac()` is direct and safe.
  - *ESP32 chip ID / EFUSE MAC CRC*: Less transparent than using the actual MAC bytes that are printed on the physical ESP32 module label.

### Decision 2: Format `BC250_` + 6 uppercase hex characters
- **Rationale**: The resulting 12-character SSID (`BC250_XXXXXX`) is compact, easy to read, fits on mobile Wi-Fi pickers without truncation, and matches the suffix printed on the hardware.
- **Alternatives Considered**:
  - *Full MAC with underscores (`BC250_AA_BB_CC_DD_EE_FF`)*: 23 characters; frequently gets truncated on mobile phone Wi-Fi menus (e.g. `BC250_AA_BB_CC_...`), obscuring the unique trailing bytes.
  - *Underscores between last 3 bytes (`BC250_DD_EE_FF`)*: Longer than necessary; user preferred the compact format `BC250_DDEEFF`.

## Risks / Trade-offs

- **[Risk] Existing documentation referencing `BC Awake Setup` becomes outdated** → **Mitigation**: Update all specification files, and ensure the serial log explicitly outputs `[PORTAL] softAP ret=%d ssid='%s'` on boot.
