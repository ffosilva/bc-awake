# Proposal

## Why

While the BC250 is running in `STATE_ON` on the AI-Thinker ESP32-CAM target, a random RF current spike (from uncapped `19.5 dBm` Wi-Fi station transmissions coexisting with Classic Bluetooth, or the 20-second Wi-Fi fallback re-initializing an already-enabled Bluetooth stack via `portalBegin()`) triggers a 3.3V brownout or reset. During the ~250 ms reset, `PS_ON#` floats high-Z and cuts ATX PSU power, but `normalBegin()` immediately samples `BOARD_SENSE` (`GPIO 13`) over just 3.2 microseconds with zero time debounce, reads residual capacitor voltage (held near the logic threshold by the ESP32-CAM's onboard 47 kOhm SD-card pull-up fighting the internal ~45 kOhm pull-down) as `HIGH`, slams `PS_ON#` LOW (`psuOn()`), and 0 ms later fires both Bluetooth and Wi-Fi RF calibration—locking the board into an infinite, high-speed ON/OFF flicker loop until unplugged.

## What Changes

- Pre-load `PS_ON_PIN` output level (`PS_ON_RELEASE`) before configuring `OUTPUT_OPEN_DRAIN` in `normalBegin()` so `PS_ON#` never glitches `LOW` on boot, and hold `PS_ON_PIN` state across software/watchdog resets when in `STATE_ON`.
- Space out the 16 consecutive `digitalRead(BOARD_SENSE)` samples on `BOARD_ESP32CAM` with a 200 us inter-sample delay (~3.2 ms total window instead of 3.2 us), and require `BOARD_SENSE` in `normalBegin()` to remain continuously `HIGH` across the full `BOARD_ON_DEBOUNCE_MS` (`250 ms`) window (after allowing residual rail capacitance to bleed down on brownout resets) before adopting `STATE_ON` and asserting `PS_ON#`.
- Stagger radio startup and power-state transitions (`psuOn()` vs. Bluetooth and Wi-Fi initialization/MAC swap) so ATX PSU turn-on inrush never coincides at `t = 0 ms` with dual-radio RF calibration.
- Make `btBegin()` and `wakeSetDiscovery()` idempotent when the Classic Bluetooth controller is already running, and prevent `portalBegin()` (including the 20-second Wi-Fi station fallback) from enabling Bluetooth discovery or page scan while the host is in `STATE_BOOTING` or `STATE_ON`.
- Cap Wi-Fi TX power on `BOARD_ESP32CAM` (`WIFI_POWER_11dBm`) and apply configured Wi-Fi TX power limits in both SoftAP (`portalBegin()`) and Station (`networkWakeBegin()` / `networkWakeOnStateChanged()`) modes to prevent AMS1117-3.3 brownout resets.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `power-control`: Require latch-first `PS_ON_PIN` initialization, 200 us inter-sample spacing on `ESP32-CAM` `BOARD_SENSE`, a continuous `250 ms` boot-sense qualification window in `normalBegin()` with brownout recovery settling, and staggered radio transitions after `psuOn()`.
- `controller-wake`: Make Classic Bluetooth initialization idempotent when already enabled and prohibit enabling Bluetooth discovery or page scan while the host is in `STATE_BOOTING` or `STATE_ON`.
- `network-wake`: Apply target Wi-Fi TX power limits in Station mode and prevent the 20-second Wi-Fi fallback SoftAP from starting Bluetooth discovery while the host machine is powered on.
- `hardware-targets`: Define reduced Wi-Fi TX power (`11 dBm`) for `BOARD_ESP32CAM` across both SoftAP and Station operation.

## Impact

- **Firmware**: `include/board.h`, `src/main.cpp`, `src/bt_classic.cpp`, `src/wake_classic.cpp`, `src/network_wake.cpp`, `src/portal.cpp`.
- **Specifications**: `power-control`, `controller-wake`, `network-wake`, `hardware-targets`.
