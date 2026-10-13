# Proposal

## Why

Today, users must keep their AMD BC250 board running 24/7 because USB Wi-Fi dongles cannot receive Wake-on-LAN (WOL) / Wake-on-Wireless-LAN packets when the host motherboard is unpowered. Furthermore, the setup web portal and REST API are only accessible when the ESP32 is explicitly booted into an offline SoftAP setup mode.

To allow users to power down the BC250 without losing network wake or web management, the ESP32 needs to run its web server and REST API 24/7 over Wi-Fi Station (STA) mode, adopt the Wi-Fi dongle's MAC while the board is off (preserving network presence and receiving WOL packets), and revert to the factory ESP32 MAC when the board powers on to avoid network MAC collisions. Finally, because over-the-air Bluetooth sensing conflicts with the host's active Bluetooth adapter while the board is on, an external Linux sync service will push paired controllers to the ESP32 via its existing REST API.

## What Changes

- **Continuous Web Portal & REST API**: Run the HTTP web server (`AsyncWebServer`) 24/7 over Wi-Fi Station (STA) in normal mode, reserving SoftAP for unconfigured or recovery mode.
- **mDNS Hostname Advertising**: Announce `bc250-switch.local` on the local network so clients and services can reliably reach the ESP32 across IP changes.
- **Dynamic Wi-Fi MAC Address Switching**:
  - While the board is in `STATE_OFF`, set the ESP32 Wi-Fi Station interface to the configured Wi-Fi dongle MAC (`bc250Mac`).
  - When the board transitions to `STATE_BOOTING` or `STATE_ON`, switch the ESP32 Wi-Fi Station interface back to the factory ESP32 MAC.
  - When transitioning back to `STATE_OFF`, restore the Wi-Fi dongle MAC.
- **Power-State Aware Web UI & REST API Protections**:
  - Expose `boardState` (`"OFF"`, `"BOOTING"`, `"ON"`) in `/api/status`.
  - When the board is in `STATE_ON`:
    - Wi-Fi configuration in the UI becomes read-only and `/api/network` rejects modifications with `403 Forbidden`.
    - Bluetooth sensing / interactive listener (`/api/listener/start`, `/api/devices`) is disabled to avoid RF collisions with the host's active Bluetooth adapter (`409 Conflict`).
    - Manual controller enrollment and bound controller list management remain fully allowed.
  - Display a visual status light in the web UI header (green for "POWERED ON", red for "POWERED OFF") reflecting the current board state.
- **Host Device Sync Service (`bc250-sync`)**:
  - A lightweight, standalone Python 3 systemd service for Linux (compatible with Bazzite, SteamOS, Ubuntu, and Arch Linux) that reads local paired gamepads and Bluetooth adapter MAC from BlueZ and synchronizes them to the ESP32 via existing REST API endpoints (`/api/login`, `/api/status`, `/api/host`, `/api/controllers/add`, `/api/controllers/remove`).

## Capabilities

### New Capabilities
- `host-device-sync`: Systemd background service and sync tool for Linux hosts to synchronize local Bluetooth adapter address and paired gamepad controllers to the ESP32 REST API.

### Modified Capabilities
- `network-wake`: Add dynamic Wi-Fi Station MAC address switching based on power state (`STATE_OFF` vs `STATE_ON`).
- `setup-portal`: Run the web portal and REST API continuously over Wi-Fi Station in normal mode, advertise `bc250-switch.local` via mDNS, expose board power state, and enforce read-only Wi-Fi and sensing lockouts while the host board is powered on.

## Impact

- **Firmware (`src/main.cpp`, `src/network_wake.cpp`, `src/portal.cpp`)**: Web server lifecycle moved into normal operation; Wi-Fi MAC switching logic added to power state transitions; mDNS responder integrated.
- **Web Interface (`app/index.html`)**: Dynamic UI disabling/locking for Wi-Fi settings and Bluetooth sensing when board state is `ON`.
- **Host Tooling (`tools/bc250-sync/`)**: New systemd service and script for Linux hosts.
- **Compatibility**: Fully backward compatible with existing hardware wiring and existing NVS storage.
