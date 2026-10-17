# Tasks

## 1. Remove ESP32-CAM Flash LED Feature & Configuration

- [x] 1.1 Remove `FLASH_DURATION_MS` from `include/board.h` (retaining `FLASH_LED = 4` documented as clamped LOW on `BOARD_ESP32CAM`) and remove `triggerFlash()`, `triggerFlashBurst()`, `updateFlash()`, and wake flash calls from `src/main.cpp` while keeping `pinMode(FLASH_LED, OUTPUT); digitalWrite(FLASH_LED, LOW);` in `normalBegin()`, verifying no references to `triggerFlash` or `updateFlash` remain in `src/` or `include/`.
- [x] 1.2 Remove `flashOnWake` and `setFlashOnWake()` from `include/config.h` and `src/config.cpp`, verifying NVS load and reset functions no longer read or write `flashOnWake`.
- [x] 1.3 Update `docs/pinout.md` to reflect that `GPIO 4` on `ESP32-CAM` is held permanently `LOW` (flash LED disabled) rather than pulsed on wake, verifying the table matches `include/board.h`.

## 2. Remove Flash LED Portal API & Web UI Controls

- [x] 2.1 Remove `triggerFlashBurst()` from `include/portal.h` and remove `isCam`, `flashOnWake`, and `/api/listener/start` flash burst calls from `src/portal.cpp`, verifying `src/portal.cpp` has no `BOARD_ESP32CAM` or `flashOnWake` conditionals.
- [x] 2.2 Remove the `#cam-options` card (`#flash-on-wake` checkbox), its `onchange` handler, and `isCam` handling from `app/index.html`, then run `python3 scripts/build_html.py` to regenerate `include/portal_html.h` and verify the script exits with code 0.

## 3. Harden Bluetooth Wake & Board Sense Filtering

- [x] 3.1 Update `EVT_CONN_REQUEST` handling in `src/bt_classic.cpp` so `OP_REMOTE_NAME_REQ` is only enqueued when `listenerActive && listenerResult.name[0] == 0`, verifying normal wake connection requests only enqueue `OP_REJECT_CONN_REQ`.
- [x] 3.2 Add `BOARD_ON_DEBOUNCE_MS = 250` to `include/board.h`, update `BOARD_ESP32CAM`'s `readBoardSense()` in `src/main.cpp` to require 16-sample (`SENSE_OVERSAMPLE`) consensus before changing the cached sense level, and use `BOARD_ON_DEBOUNCE_MS` for `STATE_BOOTING` board-sense debouncing in `normalLoop()`, verifying the debounce window and digital filter logic in `src/main.cpp`.

## 4. Build Verification

- [x] 4.1 Run `pio run` across all configured environments (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) and verify all three firmware targets compile cleanly with exit code 0.
