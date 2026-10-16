# Design

## Context

The BC250 ESP32 switch stores all persistent settings in non-volatile flash using the ESP32 Arduino `Preferences` library under the namespace `"bc250"`. These settings include:
- `controllers` (JSON array of up to 8 bound Bluetooth MACs and friendly names)
- `wakeAddr` (legacy single-controller MAC mirror)
- `hostAddr` (host Bluetooth adapter BD_ADDR for Classic BT)
- `passHash` (SHA-256 hash of the portal password)
- `forceSetup` (boolean flag to boot into SoftAP setup mode)
- `flashOnWake` (boolean flag for ESP32-CAM flash LED pulse)
- `wolEnabled` (boolean flag for Wake-on-LAN listener)
- `wifiSsid` / `wifiPassword` (Station Wi-Fi credentials)
- `bc250Mac` (BC250 Ethernet MAC address for WOL filtering)
- `hostname` (mDNS / DHCP device hostname)

Currently, the System tab in the web portal (`app/index.html`) contains cards for Firmware update (OTA), Change portal password, and Exit setup mode (visible when running in SoftAP mode). There is no mechanism in the web UI or REST API to clean or reset these settings back to defaults.

See `proposal.md` for motivation and scope boundaries.

## Goals / Non-Goals

**Goals:**
- Provide a clean, robust function `resetConfig()` in `src/config.cpp` that clears the `"bc250"` NVS namespace and restores default in-memory configuration.
- Set `forceSetup` to `true` during reset so the device restarts into SoftAP setup mode (`BC250_XXXXXX`), ensuring the device remains accessible without needing physical access to hold the hardware button.
- Expose an authenticated `POST /api/reset` endpoint requiring a valid `X-Auth-Token`.
- Add a dedicated "Reset configuration" card in the System tab of the portal web interface (`app/index.html`).
- Implement an in-page two-step confirmation (initial button revealing a warning with confirm/cancel actions) avoiding native browser dialogs (`window.confirm`) in accordance with portal resilience requirements.
- Display a clear notification explaining that the device is rebooting into setup mode and that the client must reconnect to the device's SoftAP.

**Non-Goals:**
- Hardware-based factory reset gestures (e.g. holding the physical button for 20+ seconds). Physical button behaviors remain unchanged.
- Clearing low-level ESP-IDF system namespaces (such as Wi-Fi RF calibration data or NVS partition headers). Only the application namespace `"bc250"` is wiped.
- Erasing or rolling back OTA firmware partitions. The active firmware binary is preserved.

## Decisions

### 1. Clear NVS via `Preferences::clear()` followed by `prefs.putBool("forceSetup", true)`
- **Rationale**: `prefs.clear()` removes all keys stored under the `"bc250"` namespace, preventing any leftover keys (e.g., legacy `wakeAddr` or future parameters) from remaining. Immediately writing `forceSetup = true` ensures that when the device reboots, it launches the SoftAP setup portal (`BC250_XXXXXX`) rather than attempting a station connection with an empty Wi-Fi SSID.
- **Alternatives considered**:
  - *Deleting individual keys with `prefs.remove()`*: Fragile if new configuration keys are added in the future.
  - *Leaving `forceSetup = false`*: Would cause the board to reboot into normal mode with no Wi-Fi credentials, leaving the web portal unreachable unless the user physically holds the case button for 8 seconds.

### 2. Runtime State Reset and Session Invalidation
- **Rationale**: When reset is invoked, `resetConfig()` resets the global `config` struct in memory:
  ```cpp
  config = {{}, "", "", "", true, true, false, "", "", "", "bc-awake"};
  ```
  In `src/portal.cpp`, `g_token` is cleared to immediately invalidate the current session token so no further authenticated operations can be processed while waiting for reboot.
- **Alternatives considered**: Leaving `config` in memory until reboot. Harmless due to the 800 ms reboot, but resetting immediately prevents any race conditions if status requests arrive during the reboot window.

### 3. Asynchronous Deferred Reboot (~800 ms)
- **Rationale**: Following the pattern established by `/api/finish` and `/api/update`, `/api/reset` sends a `200 OK` JSON response `{"ok":true}` and schedules `g_restartAt = millis() + 800;`. `portalCheckRestart()` in the main loop executes `ESP.restart()` once the timer expires, giving the web server time to flush TCP packets to the browser.
- **Alternatives considered**: Immediate restart with `delay()` and `ESP.restart()`. This would abruptly sever TCP sockets, causing fetch errors in the browser before the client receives the success response.

### 4. In-Page Two-Step Confirmation in System Tab
- **Rationale**: The setup portal specification mandates that all dialogs and confirmations use in-page DOM elements rather than native browser dialogs (`window.confirm`). Within the new Reset card in `app/index.html`:
  - An initial danger button "Reset all settings to defaults" is displayed.
  - Clicking it expands an inline confirmation container detailing what will be lost (Wi-Fi credentials, paired controllers, host adapter MAC, portal password) and stating that the device will reboot into SoftAP mode.
  - The container provides two actions: a styled danger button "Yes, reset configuration" and a secondary button "Cancel".
  - Clicking Cancel collapses the confirmation container without sending any request.
  - Clicking Confirm disables the buttons, stops any active poll timers, issues `POST /api/reset`, and replaces the card content with a persistent notice instructing the user to reconnect to the `BC250_XXXXXX` Wi-Fi network.
- **Alternatives considered**:
  - *Modal overlay*: Creates unnecessary DOM complexity and visual disruption compared to an inline expandable card section.

## Risks / Trade-offs

- **[Risk] Accidental reset by user**
  → *Mitigation*: The action is placed at the bottom of the System tab with a distinctive danger outline/style, requires an authenticated session, and requires an explicit two-step in-page confirmation before dispatching the API request.

- **[Risk] User loses access because device drops from local Wi-Fi**
  → *Mitigation*: Reset sets `forceSetup = true` so the device boots directly into SoftAP setup mode. The confirmation prompt and post-reset UI notice explicitly notify the user that local Wi-Fi will disconnect and provide the expected SoftAP SSID name pattern.

- **[Risk] Firmware update build toolchain synchronization**
  → *Mitigation*: Modifications to `app/index.html` require running `python3 scripts/build_html.py` to regenerate `include/portal_html.h`. Verified via PlatformIO build.
