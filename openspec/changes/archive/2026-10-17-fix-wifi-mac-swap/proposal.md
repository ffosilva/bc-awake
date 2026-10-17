# Proposal

## Why

During system power-state transitions (`STATE_OFF` to `STATE_BOOTING`/`STATE_ON` and back to `STATE_OFF`), the ESP32 fails to swap its Wi-Fi Station MAC address with error `Failed to set MAC: 12289` (`ESP_ERR_WIFI_NOT_INIT`). This happens because `WiFi.disconnect(true)` completely de-initializes the ESP-IDF Wi-Fi driver, and `esp_wifi_set_mac()` is called before `WiFi.mode(WIFI_STA)` re-initializes it. Consequently, the MAC address is never restored or adopted on power transitions, breaking Wake-on-LAN and adapter spoofing behavior.

## What Changes

- Reorder the Wi-Fi reconnection lifecycle in `applyStateMacChange()` to ensure `WiFi.mode(WIFI_STA)` re-initializes the Wi-Fi driver before `esp_wifi_set_mac()` is invoked.
- Unify the Wi-Fi station setup logic between `networkWakeBegin()` and `applyStateMacChange()` to eliminate duplicate connection sequences and avoid lifecycle divergence.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
None. This is a firmware bugfix to properly fulfill existing requirements in `network-wake`.

## Impact

- Affects `src/network_wake.cpp`.
- Restores proper MAC swapping during boot/power-off transitions without changing external APIs or configurations.
