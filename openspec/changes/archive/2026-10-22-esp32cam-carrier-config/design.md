# Design

## Context

See `proposal.md` for background and motivation. The repository currently supports three PlatformIO target environments (`esp32dev`, `esp32cam`, and `esp32-c3-devkitm-1`). The recent squashed commit applied push-pull active-`HIGH` `PS_ON#` logic and `ADC2` `TPMS1` sensing globally to `include/board.h` and `src/main.cpp`. This design isolates that carrier-board behavior to a dedicated environment, restores direct-wire open-drain defaults to bare targets, and standardizes the hardware footprint and documentation exclusively on the `BC547 / BC548` NPN BJT.

## Goals / Non-Goals

**Goals:**
- Provide a dedicated PlatformIO target `[env:esp32cam-carrier]` that compiles with `-D BOARD_ESP32CAM -D BOARD_ESP32CAM_CARRIER`.
- Report `"esp32cam-carrier"` via `FW_TARGET` for runtime transparency in the setup portal and serial boot logs.
- Gate the active-`HIGH` push-pull `PS_ON#` driver and `ADC2` `500 mV / 900 mV` hysteresis board-sensing under `BOARD_ESP32CAM_CARRIER`.
- Restore direct-wire targets (`esp32dev`, bare `esp32cam`, and `esp32-c3-devkitm-1`) to `OUTPUT_OPEN_DRAIN` (active `LOW`), and restore bare `esp32cam` to `digitalRead(13)` with `INPUT_PULLDOWN`.
- Standardize the carrier board PCB schematic, footprint, BOM, and assembly docs exclusively on `BC547 / BC548` (TO-92, `C-B-E`), removing `2N7000`, `BSS138`, and general MOSFET references.

**Non-Goals:**
- Modifying the copper track routing of the carrier PCB: the physical track positions at `Col 5, Row 7` (Collector), `Col 6, Row 7` (Base), and `Col 7, Row 9` (Emitter) remain completely intact and 100% planar.
- Changing `ESP32-DevKitC` or `ESP32-C3` pinouts or sense behavior.

## Decisions

### 1. Dedicated Target Environment vs. Build Flag Toggle
- **Decision**: Add `[env:esp32cam-carrier]` to `platformio.ini` with `-D BOARD_ESP32CAM_CARRIER` and define `FW_TARGET "esp32cam-carrier"` in `include/version.h`.
- **Rationale**: Keeps flashing and CI deterministic (`pio run -e esp32cam-carrier -t upload`). Users cannot accidentally flash bare-wire open-drain firmware to a carrier board or vice-versa.
- **Alternatives Considered**: Making `esp32cam` default to carrier board while adding `esp32cam-bare`. Rejected because existing documentation and deployments associate `esp32cam` with the base dev board.

### 2. Gating Firmware Drivers in `include/board.h` and `src/main.cpp`
- **Decision**:
  - `PS_ON_ASSERT` / `PS_ON_RELEASE`:
    ```cpp
    #if defined(BOARD_ESP32CAM_CARRIER)
    const int PS_ON_ASSERT = HIGH;  // Transistor on -> PS_ON# pulled LOW -> PSU on
    const int PS_ON_RELEASE = LOW;  // Transistor off -> PSU pull-up wins -> PSU off
    #else
    const int PS_ON_ASSERT = LOW;   // PSU on (open-drain pulls low)
    const int PS_ON_RELEASE = HIGH; // PSU off (open-drain released)
    #endif
    ```
  - `PS_ON_PIN` mode in `initHardware()`:
    ```cpp
    #if defined(BOARD_ESP32CAM_CARRIER)
      pinMode(PS_ON_PIN, OUTPUT);
    #else
      pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN);
    #endif
    ```
  - `BOARD_SENSE` hysteresis and ADC2 reading:
    Only `BOARD_ESP32CAM_CARRIER` uses `SENSE_HIGH_MV = 900` / `SENSE_LOW_MV = 500` and oversampled `analogRead(BOARD_SENSE)` on ADC2 while wireless radios are off. Bare `BOARD_ESP32CAM` retains `digitalRead(BOARD_SENSE)` with `INPUT_PULLDOWN`.

### 3. Exclusively Standardizing on BC547 / BC548
- **Decision**: Update KiCad schematic symbol to `Device:Q_NPN_CBE` (BC547 / BC548), update silkscreen outline to indicate flat face pointing toward Row 9 (bottom), and update documentation/BOM to list BC547 / BC548 as the standard part.
- **Rationale**: Eliminates the risk of users inserting a 2N7000 (which has reversed Source/Drain pinout relative to BC547's Collector/Emitter), preventing dead-short or floating gate states.
- **Alternatives Considered**: Keeping 2N7000 as secondary note. Rejected per requirement to eliminate room for mistakes.

## Risks / Trade-offs

- **[Risk] User builds carrier board but flashes `esp32cam` instead of `esp32cam-carrier`** → Mitigation: `FW_TARGET` is printed in serial boot banner and shown in the setup portal System tab so target mismatch is immediately obvious.
- **[Risk] ADC2 read attempt during SoftAP setup or Bluetooth discovery** → Mitigation: `readBoardSense()` checks `(radiosActive || g_setupMode)` and returns `false` / `0 mV` without touching ADC2 when radios are active.
