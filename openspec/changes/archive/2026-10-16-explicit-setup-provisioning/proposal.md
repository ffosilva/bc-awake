# Proposal: Explicit Provisioning State on Setup Completion

## Why

Currently, `isConfigured()` requires a portal password, at least one paired Bluetooth gamepad, and a host Bluetooth adapter address. In v1.13.0, the firmware automatically launches the setup portal SoftAP on boot whenever `!isConfigured() || config.forceSetup`. 

When a user only configures Wi-Fi credentials or Wake-on-LAN without binding a Bluetooth controller, or when onboarding the board incrementally, calling `/api/finish` or exiting setup reboots the ESP32, but `!isConfigured()` evaluates to `true` again on reboot. This traps the board in an endless setup portal loop, preventing Wi-Fi-only operation and making it appear as if the board cannot coexist as both AP and client.

## What Changes

- **Explicit `provisioned` State in NVS**: Add a persistent boolean `provisioned` flag in NVS (`bc250` namespace, key `"provisioned"`).
- **Mark Provisioned on Setup Completion**: When `/api/finish` is invoked (or when leaving setup mode via an 8s button hold while OFF), set `provisioned` to `true` in NVS and in memory alongside clearing `forceSetup`.
- **Reset Clears Provisioning**: In `resetConfig()` (`/api/reset`), reset `provisioned` back to `false` so a factory-reset board will auto-launch the setup portal again.
- **Updated `isConfigured()` Check**: `isConfigured()` evaluates to `true` if `config.provisioned` is `true`, with a backward-compatibility fallback for already-configured boards (matching existing `passHash` and Bluetooth pairings).

## Capabilities

### New Capabilities
<!-- None: uses existing capability boundaries -->

### Modified Capabilities
- `configuration-storage`: Add `provisioned` parameter to persistent storage, update `isConfigured()` evaluation criteria, and update factory reset behavior.
- `setup-portal`: Update setup completion and setup mode exit scenarios to record `provisioned = true` in NVS upon leaving setup mode.

## Impact

- **Firmware (`include/config.h`, `src/config.cpp`)**: Add `provisioned` field to `Config`, setter `setProvisioned(bool)`, load/save from NVS, and update `isConfigured()`.
- **Firmware (`src/portal.cpp`, `src/main.cpp`)**: Invoke `setProvisioned(true)` in `/api/finish` and when button-hold exits setup mode.
- **Specifications (`openspec/specs/configuration-storage/`, `openspec/specs/setup-portal/`)**: Update provisioning evaluation and setup completion criteria.
