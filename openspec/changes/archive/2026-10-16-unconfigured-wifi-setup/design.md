# Technical Design: Unconfigured Wi-Fi Setup

## Context

See `proposal.md` for motivation. Currently, `src/main.cpp` branches into two isolated loops based on `config.forceSetup`: `portalLoop()` and `normalLoop()`. In `portalBegin()`, the PSU is unconditionally released (`digitalWrite(PS_ON_PIN, PS_ON_RELEASE)`), while `normalBegin()` turns Wi-Fi completely off (`WiFi.mode(WIFI_OFF)`) when unconfigured. Furthermore, `/api/network` in `src/portal.cpp` and the UI in `app/index.html` block network settings with HTTP 403 Forbidden whenever `isBoardPowered()` is true.

## Goals / Non-Goals

**Goals:**
- Automatically broadcast SoftAP (`BC250_XXXXXX`) and serve the setup portal when the board is unconfigured (`!isConfigured()`) or `forceSetup` is set.
- Keep the power state machine (`STATE_OFF` -> `STATE_BOOTING` -> `STATE_ON`), button debounce, and TPMS1 sense active during setup mode so the BC250 can boot and run normally.
- Permit network setting updates (`/api/network`) and keep the Network tab editable while in setup mode even when the BC250 is in `STATE_ON`.
- Provide a standalone Linux setup utility in `tools/bc250-setup/` to auto-detect BlueZ and network interfaces and provision the ESP32 in a single command over Wi-Fi.

**Non-Goals:**
- Allowing 8 s button hold while the board is `STATE_ON` (the 5 s hold remains reserved as the emergency power cut backstop).
- Introducing serial console CLI protocols or USB serial setup modes.
- Replacing or modifying the recurring systemd sync role of `bc250-sync.py`.

## Decisions

### 1. Unified Loop and Non-Destructive Setup
*Decision*: Replace the isolated `portalLoop()` by integrating setup portal execution into the main power management loop. In setup mode, the SoftAP and web server start, but `PS_ON_PIN` is not forced released.
*Rationale*: A single loop guarantees that button taps, power-on sequencing, boot watchdogs, and TPMS1 drop-detection continue to operate safely during setup.
*Alternatives Considered*: Duplicating the entire power state machine into `portalLoop()` (rejected: unnecessary code duplication and maintenance hazard).

### 2. Auto-SoftAP Activation Criterion
*Decision*: The ESP32 starts SoftAP and portal web serving at boot whenever `!isConfigured() || config.forceSetup`.
*Rationale*: Out of the box, an unconfigured controller immediately presents its setup AP without requiring the user to hold buttons or connect serial adapters. Once provisioned (`isConfigured() && !config.forceSetup`), it boots directly into normal operation.
*Alternatives Considered*: Requiring an initial 8 s button hold even for brand new boards (rejected: unnecessary friction for first-time builders).

### 3. Setup-Aware Network Setting Permissions
*Decision*: In `src/portal.cpp`, `/api/network` will check `if (!g_portalSetupMode && isBoardPowered())` instead of unconditionally rejecting on `isBoardPowered()`. The web UI in `app/index.html` will similarly check `st.setupMode` before applying read-only locks to network inputs.
*Rationale*: During setup mode (over SoftAP), the user needs to enter the home Wi-Fi credentials and the BC250 MAC while sitting at the BC250 desktop. In normal station mode, the lock remains active to prevent dropping active network streaming sessions.
*Alternatives Considered*: Removing the lock entirely in all modes (rejected: changing STA settings while streaming or remote-controlling could abruptly sever active connections).

### 4. Standalone Setup Wizard (`tools/bc250-setup/`)
*Decision*: Implement `bc250-setup.py` in `tools/bc250-setup/` as a dedicated onboarding tool for Linux hosts.
*Rationale*: It keeps `tools/bc250-sync/bc250-sync.py` small, focused, and non-interactive for its periodic systemd timer duty, while the setup wizard can be interactive, guide the user, discover the `192.168.4.1` gateway, and prompt for Wi-Fi details.

## Risks / Trade-offs

- **[Risk]** Unconfigured board leaves open SoftAP active indefinitely.
  - *Mitigation*: The setup portal enforces setting a mandatory password before committing bindings; once configured and finished, the SoftAP shuts down.
- **[Risk]** Incorrect home Wi-Fi credentials entered during setup could lock the user out.
  - *Mitigation*: When `/api/finish` is called or Wi-Fi credentials are saved, the ESP32 tests association with the home AP. If connection fails or times out, SoftAP remains active on `192.168.4.1` with an error status.
- **[Risk]** Radio contention between Wi-Fi SoftAP and Bluetooth wake.
  - *Mitigation*: While the host is `STATE_ON`, Bluetooth wake is already disabled (`wakeSetConnectable(false)`). When `STATE_OFF`, ESP-IDF supports concurrent Wi-Fi SoftAP and Bluetooth page listening.
