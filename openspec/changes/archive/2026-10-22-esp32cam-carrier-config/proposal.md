# Proposal

## Why

The AI-Thinker ESP32-CAM prototype carrier board uses a low-side transistor switch on `PS_ON#` (which inverts the signal and requires push-pull `HIGH` assertion) and an RC + `4.7 kΩ` voltage divider on `TPMS1` (which divides the high-impedance board-sense line to ~1.2 V when ON and ~0.3 V when OFF, requiring `ADC2` analog hysteresis rather than `digitalRead`). Currently, applying these changes globally breaks direct-wire deployments, while listing both `2N7000` (N-MOSFET, `S-G-D`) and `BC547` (NPN BJT, `C-B-E`) in documentation and schematics creates an error-prone pinout conflict due to their mirrored TO-92 lead assignments.

## What Changes

- Add a dedicated PlatformIO build environment `[env:esp32cam-carrier]` that defines `-D BOARD_ESP32CAM` and `-D BOARD_ESP32CAM_CARRIER` (reporting target identifier `"esp32cam-carrier"`), isolating carrier-board behavior from bare-module direct-wire targets (`esp32dev`, `esp32cam`, and `esp32-c3-devkitm-1`).
- Gate the push-pull active-`HIGH` `PS_ON#` output driver (`OUTPUT`, `PS_ON_ASSERT = HIGH`, `PS_ON_RELEASE = LOW`) and the `ADC2` `TPMS1` analog hysteresis sensing (`SENSE_LOW_MV = 500`, `SENSE_HIGH_MV = 900` sampled while wireless radios are off) under `BOARD_ESP32CAM_CARRIER`, restoring direct-wire open-drain `PS_ON#` and digital `INPUT_PULLDOWN` sensing for bare `BOARD_ESP32CAM`.
- Standardize the `PS_ON#` low-side transistor switch (`Q1`) exclusively on the **BC547 / BC548** NPN BJT (`TO-92`, Pin 1 = Collector, Pin 2 = Base, Pin 3 = Emitter) and remove all `2N7000` / `BSS138` / MOSFET alternatives across KiCad schematics, PCB footprints, BOM generator, and documentation so there is no ambiguity in component selection or orientation.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `hardware-targets`: Add the `esp32cam-carrier` (`BOARD_ESP32CAM_CARRIER`) target configuration and target identification metadata, and update the ESP32-CAM resilience circuit, carrier PCB, and BOM requirements to exclusively specify the `BC547 / BC548` NPN BJT buffer and `ADC2` board-sense conditioning.
- `power-control`: Specify `PS_ON#` drive mode and polarity for both direct-wire open-drain targets and carrier-board BJT-buffered targets, and add `ADC2` oversampled hysteresis board-sense conditioning (`500 mV` / `900 mV` with radio detachment in `STATE_OFF`) for `BOARD_ESP32CAM_CARRIER`.

## Impact

- **Build configuration**: `platformio.ini` gains `[env:esp32cam-carrier]`.
- **Firmware**: `include/version.h`, `include/board.h`, and `src/main.cpp` separate `BOARD_ESP32CAM_CARRIER` from bare `BOARD_ESP32CAM` and other direct-wire targets.
- **Hardware & Documentation**: `scripts/generate_carrier_pcb.py`, `hardware/kicad/esp32cam-carrier/*`, `docs/bom.md`, `docs/esp32cam-carrier-pcb.md`, `docs/pinout.md`, and `README.md` are updated to use `BC547 / BC548` exclusively and document `[env:esp32cam-carrier]`.
