# Proposal

## Why

The device has no on-device feedback about its state (off, booting, on, or setup mode). The only setup indicator is the `BC250_` SoftAP, which needs a phone to check. Every AI-Thinker ESP32-CAM module has a red status LED on GPIO33 (active LOW). It is free in firmware and needs no PCB or wiring change, so it can be a power/status indicator on all ESP32-CAM configurations, with and without the carrier board.

## What Changes

- Add a firmware-driven status LED on the ESP32-CAM's onboard red LED (GPIO33, active LOW), non-blocking and `millis()`-based.
- Patterns:
  - `STATE_ON`: solid on.
  - `STATE_BOOTING`: fast blink (about 4 Hz).
  - `STATE_OFF`: completely off.
  - Setup mode: slow blink at 1 Hz.
- Setup mode overrides the power-state pattern. It keeps its existing behavior of blocking power-on, and an 8 s hold exits it back to normal operation (no change to that behavior).
- Applies to `BOARD_ESP32CAM`, so both `esp32cam` and `esp32cam-carrier` get it. Other targets compile the LED logic out.
- No other LED patterns (OTA, boot failure and similar) in this change.

## Capabilities

### New Capabilities
- `status-led`: LED patterns that reflect the power state and setup mode.

### Modified Capabilities
- `hardware-targets`: the ESP32-CAM target gains a status LED pin (GPIO33, active LOW). Targets without a status LED have none.

## Impact

- Code: [board.h](../../../include/board.h) (pin and polarity constants, default of none for other targets), `src/main.cpp` (LED init and update in `setup()`/`loop()`), possibly a small new LED module.
- Docs: [pinout.md](../../../docs/pinout.md) (GPIO33 entry).
- No hardware, partition, NVS or portal changes. No new dependencies.
