# Proposal: Unconfigured Wi-Fi Setup Access

## Why

When the ESP32 board is unconfigured, there is no communication path between the BC250 host computer and the controller. Current firmware forces the PSU off (`PS_ON_RELEASE`) during setup mode, shuts down the Wi-Fi stack in normal mode when unconfigured, and blocks network settings with HTTP 403 Forbidden whenever the host is in `STATE_ON`. This forces users to configure the controller via an external smartphone or laptop, manually transcribing Bluetooth and network MAC addresses from the BC250 screen.

Enabling direct Wi-Fi setup from the host computer allows users to power on the BC250, connect directly to the ESP32's setup SoftAP, and copy-paste or script the configuration with zero external devices.

## What Changes

- **Automatic SoftAP on First Boot / Unconfigured**: When the firmware boots and is not provisioned (`!isConfigured()`), it automatically starts the `BC250_XXXXXX` SoftAP and HTTP web portal while keeping the power state machine and push-button controls fully functional.
- **Power Coexistence**: Tapping the physical power button while unconfigured powers on the BC250 into `STATE_ON` without tearing down the SoftAP or stopping the web server.
- **Relax Network Setting Restrictions**: Remove the artificial `isBoardPowered()` 403 Forbidden check from `/api/network` and unlock the Network tab in the web UI when running in setup/SoftAP mode so Wi-Fi credentials and BC250 MACs can be configured while the host is running.
- **Preserve Clean Button Semantics**: Retain the 8 s setup hold strictly for when the machine is in `STATE_OFF`, ensuring no conflict with the 5 s hold-to-shutdown safety backstop while `STATE_ON`.
- **SoftAP-to-Station Handover with Fallback**: When valid home Wi-Fi credentials are saved and setup is completed, the ESP32 transitions to Station mode; if connection to the home router fails, it falls back to keeping the SoftAP active to prevent lockout.
- **Standalone Setup Wizard**: Provide a dedicated setup utility (`tools/bc250-setup/`) that detects the SoftAP gateway (`192.168.4.1`), auto-discovers local BlueZ controller and network interface MACs, and provisions the board over Wi-Fi.

## Capabilities

### New Capabilities
<!-- None: uses existing capability boundaries -->

### Modified Capabilities
- `setup-portal`: Update setup portal activation to broadcast SoftAP automatically when unconfigured while keeping the power controller active, and allow updating network wake settings while in setup mode even if the host machine is in `STATE_ON`.

## Impact

- **Firmware (`src/main.cpp`, `src/portal.cpp`)**: Unified power controller loop and setup portal lifecycle; relaxed network endpoint checks for setup mode.
- **Web UI (`app/index.html`)**: Remove read-only lock from Network tab inputs when accessed via SoftAP.
- **Tooling (`tools/bc250-setup/`)**: New standalone Python/CLI setup wizard for Linux hosts.
- **Documentation (`README.md`, `docs/`)**: Updated setup guide reflecting that users can configure directly from the BC250 over Wi-Fi.
