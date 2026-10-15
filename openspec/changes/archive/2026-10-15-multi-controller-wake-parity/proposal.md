# Proposal: Multi-Controller Wake Parity and Dynamic Runtime Updates

## Why

Currently, when multiple controllers (up to 8) are enrolled via the setup portal or `bc250-sync`:
1. On BLE targets (`BOARD_ESP32C3`), `src/wake_ble.cpp` only matches incoming advertisements against `config.wakeAddr` (which strictly tracks the first enrolled controller), ignoring all other enrolled controllers.
2. On Classic Bluetooth targets, adding or removing controllers via the REST API (`/api/controllers/add`, `/api/controllers/remove`) updates NVS and in-memory config, but never refreshes the active `wakeAddrs` table in `bt_classic.cpp`. Because the ESP32 is powered continuously by the ATX power supply's 5V standby rail (`5VSB`), it never reboots on PC shutdown, leaving the active Bluetooth wake engine with a stale controller list containing only the first controller.

This change ensures feature parity across both Bluetooth Classic and BLE targets and enables immediate runtime synchronization of bound controllers so that any enrolled gamepad can power on the machine without requiring a physical ESP32 power cycle.

## What Changes

- **BLE Multi-Controller Wake**: Update `src/wake_ble.cpp` so that `WakeScanCallbacks::onResult` checks incoming advertised addresses against all bound controllers in `config.controllers` (case-insensitively).
- **Unified Dynamic Wake Update Interface**: Add `void wakeUpdateControllers()` to `include/wake.h`, implemented by both `src/wake_classic.cpp` and `src/wake_ble.cpp`.
- **Classic BT Runtime Wake Refresh**: In `src/wake_classic.cpp`, implement `wakeUpdateControllers()` to refresh `btSetWakeAddrs()` with the current `config.controllers` list.
- **Portal Runtime Sync**: Call `wakeUpdateControllers()` in `src/portal.cpp` upon controller addition (`handleControllersAdd`) and removal (`handleControllersRemove`).
- **BLE Diagnostic & Status Parity**: Update BLE startup and status logging to reflect the count and addresses of all bound controllers being scanned.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `controller-wake`: Extend BLE scanning requirement to monitor all bound controllers (up to 8) and require dynamic runtime synchronization when the bound controllers list changes without requiring a system reboot.

## Impact

- **Affected Code**: `include/wake.h`, `src/wake_classic.cpp`, `src/wake_ble.cpp`, `src/portal.cpp`.
- **APIs / Endpoints**: `POST /api/controllers/add` and `POST /api/controllers/remove` will immediately refresh the active wake engine in addition to updating NVS.
- **Hardware Targets**: Both Classic BT (`esp32dev`, `esp32cam`) and BLE (`esp32-c3-devkitm-1`).
- **Dependencies & Storage**: No new libraries or NVS schema changes; remains backward-compatible with existing NVS storage.
