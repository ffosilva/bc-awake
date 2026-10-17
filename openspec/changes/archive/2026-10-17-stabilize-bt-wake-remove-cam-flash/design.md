# Design

## Context

See `proposal.md` for motivation. On the AI-Thinker ESP32-CAM target (`BOARD_ESP32CAM`), the onboard white flash LED (`GPIO 4`) is driven via an NPN transistor from the 3.3V regulator with no series current limiting, drawing ~300 mA when set `HIGH`. Currently:
1. `normalLoop()` calls `triggerFlash()` on Bluetooth controller wake and WOL wake, driving `GPIO 4` `HIGH` for 500 ms at the exact instant `psuOn()` asserts `PS_ON#` (`GPIO 14`) and `setState(STATE_BOOTING)` triggers Bluetooth and Wi-Fi transitions.
2. In `src/bt_classic.cpp`, `onHostRecv()` queues `OP_REMOTE_NAME_REQ` on every incoming `EVT_CONN_REQUEST` before `OP_REJECT_CONN_REQ`, even during normal wake when `listenerActive` is `false`.
3. On `BOARD_ESP32CAM`, `readBoardSense()` performs a single `digitalRead(BOARD_SENSE)` (`GPIO 13`), and `normalLoop()` uses `DEBOUNCE_MS` (`30 ms`) to qualify `BOARD_SENSE` during `STATE_BOOTING`. Any 30 ms noise spike during ATX PSU turn-on transitions the state machine to `STATE_ON` prematurely, which then triggers a shutdown 1.5s later (`BOARD_OFF_DEBOUNCE_MS`) while `TPMS1` is still low.

## Goals / Non-Goals

**Goals:**
- Completely eliminate all runtime activation and user-facing configuration of the ESP32-CAM onboard flash LED (`GPIO 4`), while keeping `GPIO 4` clamped `LOW` at startup so PCB trace leakage from `HS2_DATA1` cannot make the LED glow.
- Prevent unnecessary Bluetooth radio paging (`OP_REMOTE_NAME_REQ`) during normal controller wake events.
- Harden `BOARD_SENSE` qualification on `BOARD_ESP32CAM` and during `STATE_BOOTING` across all targets so PSU turn-on transients cannot cause false `STATE_BOOTING -> STATE_ON -> STATE_OFF` cycles.

**Non-Goals:**
- Adding PWM dimming or alternative LED indicators on `ESP32-CAM`.
- Changing the 15-second `WAKE_WAKE_COOLDOWN_MS` or 10-second `BOOT_TIMEOUT_MS` durations.

## Decisions

### 1. Clamp `GPIO 4` LOW at Boot Instead of Leaving It Unconfigured
- **Decision**: Retain `FLASH_LED = 4` in `include/board.h` and `pinMode(FLASH_LED, OUTPUT); digitalWrite(FLASH_LED, LOW);` in `normalBegin()` under `#if defined(BOARD_ESP32CAM)`, while removing `FLASH_DURATION_MS`, `triggerFlash()`, `triggerFlashBurst()`, `updateFlash()`, and all portal/config hooks.
- **Rationale**: On the AI-Thinker ESP32-CAM schematic, `GPIO 4` is shared with the microSD `HS2_DATA1` line. If left high-impedance, leakage current into the S8050 transistor base can cause the flash LED to glow dimly. Explicitly clamping it `LOW` ensures zero current draw through the LED.
- **Alternatives considered**: Leaving `GPIO 4` untouched as default input (rejected due to SD trace pull-up leakage).

### 2. Gate `OP_REMOTE_NAME_REQ` on `listenerActive` in `EVT_CONN_REQUEST`
- **Decision**: In `src/bt_classic.cpp` (`onHostRecv` -> `EVT_CONN_REQUEST`), only enqueue `OP_REMOTE_NAME_REQ` when `listenerActive && listenerResult.name[0] == 0`.
- **Rationale**: During normal wake in `STATE_OFF`, the controller MAC is already matched against `wakeAddrs` and page scan is immediately disabled (`btSetConnectable(false)`). Requesting the remote name before rejecting the connection wastes HCI command slots and keeps the Bluetooth radio transmitting during PSU power-up.

### 3. Consensus Digital Filtering on `ESP32-CAM` and `BOARD_ON_DEBOUNCE_MS` (`250 ms`)
- **Decision**:
  - In `src/main.cpp` under `#if defined(BOARD_ESP32CAM)`, update `readBoardSense()` to read `digitalRead(BOARD_SENSE)` `SENSE_OVERSAMPLE` (`16`) times consecutively; only flip the cached `senseLevel` when all 16 reads agree (`16` HIGHs -> `true`, `0` HIGHs -> `false`), matching the noise-rejection behavior of the ADC targets.
  - Define `BOARD_ON_DEBOUNCE_MS = 250` in `include/board.h` and use it instead of `DEBOUNCE_MS` (`30 ms`) when debouncing `boardRaw` in `STATE_BOOTING`.
- **Rationale**: 250 ms is short enough to feel instantaneous when `TPMS1` rises genuinely on boot, but long enough (combined with 16-sample consensus filtering) to reject PSU switching transients and EMI spikes.

## Risks / Trade-offs

- **[Removing `flashOnWake` API/UI field]** -> External scripts or cached browsers expecting `flashOnWake` or `isCam` in `/api/status` will no longer see them. Neither `bc250-sync` nor `bc250-setup` uses `flashOnWake` or `isCam`, and `/api/settings` was only used by `#flash-on-wake` in `app/index.html`.
- **[250 ms boot sense debounce]** -> Delays the `STATE_BOOTING -> STATE_ON` transition log by 220 ms compared to 30 ms, which has no adverse effect on the 10,000 ms boot watchdog.
