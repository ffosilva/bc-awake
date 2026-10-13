# Proposal: Over-the-Air (OTA) Firmware Updates and Firmware Versioning

## Why

Updating the firmware currently requires connecting the ESP32 board to a computer via USB/UART and flashing over a serial line. Because the switch is installed inside a desktop PC case powering the BC250 motherboard from ATX standby, physical access for re-flashing is cumbersome. Adding Over-The-Air (OTA) update capability in the setup portal enables wireless updates directly from a phone or browser. Additionally, introducing explicit firmware versioning and target hardware reporting allows users to verify what version is installed and confirm successful updates.

## What Changes

- **Dual-Bank Flash Partitioning**: Reconfigure `partitions.csv` to replace the single application partition with two 1.81 MB OTA banks (`app0` / `ota_0` and `app1` / `ota_1`) and reduce SPIFFS to 256 KB.
- **OTA Upload Endpoint**: Add an authenticated `POST /api/update` endpoint to `ESPAsyncWebServer` using Arduino's `Update.h` to stream, verify, and write firmware to the inactive OTA partition.
- **Firmware Versioning & Target Reporting**:
  - Define semantic firmware version (`FW_VERSION`) and target platform strings (`esp32dev`, `esp32cam`, `esp32-c3`).
  - Report `version` and `target` in `/api/status` JSON responses.
  - Display the version badge and target in the setup portal UI.
  - Print version and target in serial initialization banner.
- **Setup Portal Update UI**: Add a dedicated "Firmware Update" card in `app/index.html` with file selection, upload progress tracking, error handling, and post-update reboot notification.
- **Resource Protection During Flashing**: Temporarily suspend background Bluetooth scanning/discovery when an OTA write starts to prevent heap exhaustion and cache stalls.

## Capabilities

### New Capabilities
*(None; builds on existing setup portal and hardware targets capabilities)*

### Modified Capabilities
- `setup-portal`: Add requirements for firmware version/target reporting in the status API, the authenticated OTA firmware update endpoint (`/api/update`), and in-page firmware update controls in the captive portal UI.
- `hardware-targets`: Add requirements for compile-time target hardware identifier definitions and dual-bank OTA flash partition support across all supported boards.

## Impact

- **Flash Layout & First-Time Flashing**: Modifying `partitions.csv` alters partition offsets. The transition to this dual-bank layout requires a one-time wired flash via USB/UART before OTA updates become operational.
- **API**: Extends `GET /api/status` with `version` and `target` fields. Adds `POST /api/update`.
- **UI**: Expands `app/index.html` with firmware update upload controls and version badge.
- **Dependencies**: Uses Arduino's built-in `Update.h` (already part of `framework-arduinoespressif32`).
