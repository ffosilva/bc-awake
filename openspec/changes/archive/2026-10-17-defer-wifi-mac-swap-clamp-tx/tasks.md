# Tasks

## 1. Network Wake Lifecycle & TX Power Clamping

- [x] 1.1 Update `networkWakeOnStateChanged()` in `include/network_wake.h` and `src/network_wake.cpp` to ignore `STATE_BOOTING`, schedule the deferred factory MAC swap only in `STATE_ON` with `MAC_SWAP_DEFER_MS = 5000` ms, cancel pending swaps on `STATE_OFF`, and update callers in `src/main.cpp` to pass the power state.
- [x] 1.2 In `src/network_wake.cpp`, clamp PHY TX power prior to initiating station connections by calling `esp_wifi_set_max_tx_power((int8_t)AP_TX_POWER)` immediately after `WiFi.mode(WIFI_STA)` in `networkWakeBegin()`.

## 2. Verification & Validation

- [x] 2.1 Run `pio run -e esp32dev -e esp32cam -e esp32-c3-devkitm-1` and `python3 tools/bc250-setup/test_setup.py` to verify compilation for all targets and ensure test suites pass without warnings or errors.
- [x] 2.2 Run `openspec validate "defer-wifi-mac-swap-clamp-tx"` to verify all planning artifacts adhere to OpenSpec specifications.

## Workflow follow-up

- Flash target `esp32cam` over `/dev/cu.usbserial-10` and verify on the real BC250 that Bluetooth wake turns on the BC250 without power light flickering or brownout resets.
- Archive the change via `/openspec-archive-change`.
