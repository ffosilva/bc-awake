# Tasks: Unconfigured Wi-Fi Setup

## 1. Firmware Power & Setup Lifecycle Integration

- [x] 1.1 Update `setup()` and main initialization in `src/main.cpp` to launch the SoftAP and portal web server when `!isConfigured() || config.forceSetup`, while preserving the active power state machine. Verify build compiles on `esp32dev`, `esp32cam`, and `esp32-c3-devkitm-1`.
- [x] 1.2 Remove unconditional `PS_ON_RELEASE` from `portalBegin()` in `src/portal.cpp` so the ATX power supply is not forced off when entering setup mode. Verify `PS_ON#` state remains unchanged during setup initialization.
- [x] 1.3 Integrate setup portal servicing into the unified power loop so push-button controls (tap to power on while off, hold 5 s while on to force off) and `TPMS1` board-sense debounce remain active while SoftAP is running. Verify button tap initiates `STATE_BOOTING` without interrupting SoftAP.

## 2. API Permissions & Web UI Unlock

- [x] 2.1 Modify `/api/network` in `src/portal.cpp` to allow updating Wi-Fi credentials, hostname, and BC250 MAC while `isBoardPowered()` is true during setup mode (`g_portalSetupMode`). Verify endpoint returns HTTP 200 OK when called with the board powered on in setup mode.
- [x] 2.2 Update `app/index.html` to keep the Network tab inputs and save action editable when `setupMode` is true, even if `boardState` is `ON` or `BOOTING`. Verify inputs are enabled in the embedded portal interface.
- [x] 2.3 Implement Wi-Fi association testing and fallback retention in `src/network_wake.cpp` and `src/portal.cpp` so that SoftAP remains available if home Wi-Fi connection fails upon finishing setup. Verify fallback behavior via serial logs.

## 3. Standalone Setup Wizard & Documentation

- [x] 3.1 Create `tools/bc250-setup/bc250-setup.py` to auto-discover BlueZ controller MACs, paired gamepads, and local Ethernet/Wi-Fi interface MAC addresses from the host system. Verify tool runs and detects local interfaces on Linux.
- [x] 3.2 Implement REST API provisioning client in `tools/bc250-setup/bc250-setup.py` targeting `http://192.168.4.1` with interactive prompts for password and home Wi-Fi credentials. Verify end-to-end setup script execution against mocked or live endpoints.
- [x] 3.3 Update `README.md` and `docs/` to document the direct host setup process via browser copy & paste and via the standalone `bc250-setup.py` script. Verify markdown links and documentation consistency.
