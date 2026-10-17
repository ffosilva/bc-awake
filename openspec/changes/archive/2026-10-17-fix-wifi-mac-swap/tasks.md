# Tasks

## 1. Fix Wi-Fi Station MAC Swap Lifecycle

- [x] 1.1 Refactor `applyStateMacChange()` in `src/network_wake.cpp` to call `networkWakeBegin(isOff)` after stopping listeners and disconnecting with `WiFi.disconnect(true)`, ensuring `WiFi.mode(WIFI_STA)` runs before `esp_wifi_set_mac()`. Verify code eliminates duplication and preserves 500 ms deferral behavior.
- [x] 1.2 Build firmware for all supported targets (`pio run`) and verify compilation succeeds without warnings or errors.
- [x] 1.3 Verify existing Python tests pass with `python3 tools/bc250-setup/test_setup.py`.
