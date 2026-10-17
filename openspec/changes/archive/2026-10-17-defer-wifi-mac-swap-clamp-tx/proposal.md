# Proposal

## Why

On targets with linear voltage regulators (such as the AI-Thinker ESP32-CAM powered from ATX +5VSB), waking the AMD BC250 via Bluetooth or button press causes the BC250's power light to flash on for ~500 ms and immediately shut off. This occurs because the firmware currently schedules a Wi-Fi disconnect and MAC address restore 500 ms after `PS_ON#` is asserted—right in the middle of `STATE_BOOTING` when ATX PSU 12V inrush current and motherboard VRMs peak, creating electrical noise and voltage sag on +5VSB. Furthermore, `WiFi.setTxPower()` is called after `WiFi.begin()`, resulting in initial connection and scan bursts at full 19.5 dBm (~400–500 mA spike) that trip the ESP32 brownout detector, float `PS_ON#` to high-Z, and kill PSU power.

## What Changes

- **Defer Wi-Fi MAC Restore to `STATE_ON`**: Prevent any Wi-Fi disconnection, MAC swap, or reconnection while the host machine is in `STATE_BOOTING`. Maintain the existing Wi-Fi station connection untouched during boot so no RF transients collide with ATX PSU turn-on inrush.
- **Settle Before Restoring Factory MAC**: In `STATE_ON` (after motherboard `TPMS1` has been qualified and confirmed HIGH), defer restoring the factory default MAC address by a 5000 ms stabilization window, ensuring ATX rails and motherboard power states are completely quiet before Wi-Fi reconnects.
- **Avoid Unnecessary Swaps on Failed Boots**: If a boot attempt times out or aborts before reaching `STATE_ON`, discard the pending MAC restore so Wi-Fi does not cycle unnecessarily.
- **Pre-Connection RF Power Clamping**: Call `esp_wifi_set_max_tx_power((int8_t)AP_TX_POWER)` immediately after `WiFi.mode(WIFI_STA)` (before `WiFi.begin()`) in addition to `WiFi.setTxPower(AP_TX_POWER)`, ensuring the Wi-Fi PHY layer never emits RF calibration or channel probe bursts at default 19.5 dBm.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `network-wake`: Update `Wi-Fi Station Management for Network Wake` to prohibit MAC swapping during `STATE_BOOTING`, require MAC restoration only after `STATE_ON` with an extended stabilization delay, and clamp RF TX power prior to initiating station connections.

## Impact

- **Firmware**: `src/network_wake.cpp` and `include/board.h`.
- **Hardware Targets**: Eliminates brownout resets on `BOARD_ESP32CAM` and prevents power-cycle glitching across all targets during Bluetooth and button wake.
