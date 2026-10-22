# Tasks

## 1. PlatformIO and Firmware Target Configuration

- [x] 1.1 Add `[env:esp32cam-carrier]` to `platformio.ini` with `-D BOARD_ESP32CAM` and `-D BOARD_ESP32CAM_CARRIER`, and verify configuration syntax with `pio project config`
- [x] 1.2 Update `include/version.h` to define `FW_TARGET "esp32cam-carrier"` when `BOARD_ESP32CAM_CARRIER` is defined, and verify the definition matches build environments
- [x] 1.3 Update `include/board.h` to gate `PS_ON_ASSERT = HIGH` / `PS_ON_RELEASE = LOW` and carrier board hysteresis thresholds (`SENSE_HIGH_MV = 900`, `SENSE_LOW_MV = 500`) under `BOARD_ESP32CAM_CARRIER`, while restoring direct-wire open-drain defaults (`PS_ON_ASSERT = LOW`, `PS_ON_RELEASE = HIGH`) for bare targets
- [x] 1.4 Update `src/main.cpp` to set `pinMode(PS_ON_PIN, OUTPUT)` for `BOARD_ESP32CAM_CARRIER` and `OUTPUT_OPEN_DRAIN` for bare targets, and gate ADC2 oversampled reading exclusively for `BOARD_ESP32CAM_CARRIER` while restoring `digitalRead(BOARD_SENSE)` with `INPUT_PULLDOWN` on bare `BOARD_ESP32CAM`
- [x] 1.5 Verify all targets (`esp32dev`, `esp32cam`, `esp32cam-carrier`, `esp32-c3-devkitm-1`) compile cleanly with `pio run`

## 2. Hardware Standardization & Tooling (BC547 / BC548)

- [x] 2.1 Update `scripts/generate_carrier_pcb.py` to use `Device:Q_NPN_CBE` (BC547 / BC548) for Q1, assign Pin 1 to Collector (`PS_ON#`), Pin 2 to Base (`NET_GATE`), Pin 3 to Emitter (`GND`), update silkscreen flat-edge indicator toward Row 9, and remove all 2N7000 / BSS138 / MOSFET references
- [x] 2.2 Run `scripts/generate_carrier_pcb.py` to regenerate KiCad schematic, PCB, netlist, DRC report (0 violations), SVG, PDF, and BOM CSV
- [x] 2.3 Verify the regenerated KiCad netlist, DRC report, and `esp32cam-carrier-bom.csv` list BC547 / BC548 as standard component

## 3. Documentation Updates

- [x] 3.1 Update `docs/esp32cam-carrier-pcb.md` and `docs/bom.md` to document BC547 / BC548 as the sole standard transistor, update lead orientation instructions (flat face toward Row 9), and remove all 2N7000 / MOSFET references
- [x] 3.2 Update `docs/pinout.md` and `README.md` to document the dedicated `[env:esp32cam-carrier]` target and BC547 / BC548 buffer wiring
- [x] 3.3 Validate the change artifacts with `openspec validate esp32cam-carrier-config` and verify all checks pass
