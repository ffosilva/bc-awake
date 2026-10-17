# Tasks

## 1. Harden Startup Power State Adoption & Board Sense Filtering

- [x] 1.1 Update `normalBegin()` in `src/main.cpp` to call `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` before `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` so the output latch is pre-loaded HIGH before enabling the open-drain driver, verifying `PS_ON_PIN` initialization order in `src/main.cpp`.
- [x] 1.2 Update `BOARD_ESP32CAM`'s `readBoardSense()` in `src/main.cpp` to insert `delayMicroseconds(200)` between consecutive `digitalRead(BOARD_SENSE)` samples so the 16-sample consensus filter spans ~3.2 ms, verifying the inter-sample delay is present in `src/main.cpp`.
- [x] 1.3 Update `normalBegin()` in `src/main.cpp` to check `esp_reset_reason()`: if `ESP_RST_BROWNOUT`, log a warning, wait `500 ms` with `PS_ON#` released for rail capacitors to bleed down, and skip `STATE_ON` adoption; otherwise allow a `10 ms` pin-settling delay and require `readBoardSense()` to remain continuously `HIGH` across the full `BOARD_ON_DEBOUNCE_MS` (`250 ms`) window (sampled every `10 ms`) before adopting `STATE_ON`, calling `psuOn()`, and delaying `200 ms` before starting wireless radios, verifying the startup qualification logic in `src/main.cpp`.

## 2. Make Bluetooth Discovery Idempotent & Block Discovery While Powered On

- [x] 2.1 Update `btBegin()` in `src/bt_classic.cpp` so that if `btUp` is already `true` and `hostMac` matches `currentHostMac`, it returns `true` immediately without re-calling `esp_iface_mac_addr_set()`, `esp_vhci_host_register_callback()`, or `queueInit()` (and delegates to `btRestart(hostMac)` if `hostMac` changed), verifying `btBegin()` idempotency in `src/bt_classic.cpp`.
- [x] 2.2 Update `portalBegin()` in `src/portal.cpp` and `setState()` in `src/main.cpp` so Bluetooth discovery (`wakeSetDiscovery(true)`) and page scan (`wakeSetConnectable(true)`) are only enabled when the board is in `STATE_OFF` (`!isBoardPowered()`) and are explicitly disabled (`wakeSetDiscovery(false)` and `wakeSetConnectable(false)`) when entering `STATE_BOOTING` or `STATE_ON` (and re-enabled on returning to `STATE_OFF` if `isPortalSetupMode()` is true), verifying discovery state transitions across `src/portal.cpp` and `src/main.cpp`.

## 3. Cap Wi-Fi TX Power & Stagger Station MAC Transitions

- [x] 3.1 Update `include/board.h` to set `AP_TX_POWER` to `WIFI_POWER_11dBm` on `BOARD_ESP32CAM` and apply `WiFi.setTxPower(AP_TX_POWER)` after `WiFi.begin(...)` in `networkWakeBegin()` and `networkWakeOnStateChanged()` in `src/network_wake.cpp`, verifying TX power configuration across `include/board.h` and `src/network_wake.cpp`.
- [x] 3.2 Update `networkWakeOnStateChanged()` and `networkWakeLoop()` in `src/network_wake.cpp` so that when transitioning out of `STATE_OFF` (`isOff == false`), the Wi-Fi disconnect, MAC restore, and reconnect sequence is deferred by `500 ms` via a non-blocking timer rather than executing synchronously on the same millisecond as `psuOn()`, verifying the non-blocking deferral logic in `src/network_wake.cpp`.

## 4. Build & Specification Verification

- [x] 4.1 Run `pio run` across all three PlatformIO environments (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) and `openspec validate "fix-state-on-power-flicker-loop"` to verify all firmware targets compile cleanly and the OpenSpec change passes validation with exit code 0.
