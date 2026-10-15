# Tasks

## 1. Unified Wake Interface & Classic Bluetooth Synchronization

- [x] 1.1 Add `void wakeUpdateControllers();` declaration to `include/wake.h`
- [x] 1.2 Implement `wakeUpdateControllers()` in `src/wake_classic.cpp` to extract MAC addresses from `config.controllers` and invoke `btSetWakeAddrs()`, and refactor `wakeBegin()` to use it
- [x] 1.3 Call `wakeUpdateControllers()` in `src/portal.cpp` upon controller addition in `handleControllersAdd()` and controller removal in `handleControllersRemove()`

## 2. BLE Multi-Controller Wake Implementation

- [x] 2.1 Add in-memory `bleWakeAddrs` storage and spinlock protection in `src/wake_ble.cpp` to hold up to 8 bound controller addresses
- [x] 2.2 Implement `wakeUpdateControllers()` in `src/wake_ble.cpp` to refresh `bleWakeAddrs` from `config.controllers`, and update `wakeBegin()` to populate this buffer and log all scanned controllers
- [x] 2.3 Update `WakeScanCallbacks::onResult` in `src/wake_ble.cpp` to iterate across all stored controller addresses and record wake sighting on match

## 3. Build & Spec Verification

- [x] 3.1 Compile all PlatformIO targets (`esp32dev`, `esp32cam`, and `esp32-c3-devkitm-1`) with `pio run` to verify zero compilation or link errors
- [x] 3.2 Validate OpenSpec change integrity and artifacts with `openspec validate multi-controller-wake-parity`
