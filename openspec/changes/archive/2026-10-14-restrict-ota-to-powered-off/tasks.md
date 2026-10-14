# Tasks

## 1. Backend Endpoint Guard

- [x] 1.1 In `src/portal.cpp`, add host power state verification (`isBoardPowered()`) to `handleUpdateUpload`: on initial chunk (`index == 0`), abort immediately if the board is powered on without calling `Update.begin()` or pausing Bluetooth discovery; during streaming, abort via `Update.abort()` if power state changes.
- [x] 1.2 In `src/portal.cpp`, add host power state verification (`isBoardPowered()`) to `handleUpdateResponse`: return HTTP 409 Conflict with JSON `{"error":"Firmware update disabled while host machine is powered on"}` if the board is not in `STATE_OFF`.
- [x] 1.3 Verify backend changes compile cleanly by running `pio run -e esp32-c3-devkitm-1`.

## 2. Web Portal User Interface

- [x] 2.1 In `app/index.html`, update `applyBoardStateLocks()` to disable `#ota-file` and `#ota-btn` and inject or reveal an informative lock notice (`#ota-readonly-notice`) within the OTA card when `isBoardOn` is true (`currentBoardState === "ON"` or `BOOTING`).
- [x] 2.2 In `app/index.html`, ensure `applyBoardStateLocks()` removes the lock notice and re-enables `#ota-file` when the board transitions to `STATE_OFF`, keeping `#ota-btn` disabled until a valid `.bin` file is selected.
- [x] 2.3 Run `python3 scripts/build_html.py` to regenerate `include/portal_html.h` from `app/index.html` and verify the generated header builds without errors.

## 3. Verification & Build Integration

- [x] 3.1 Run `pio run` across target environments (`esp32-c3-devkitm-1`, `esp32dev`, `esp32cam`) and verify successful firmware compilation.
- [x] 3.2 Verify that the endpoint contracts and UI lock logic meet the scenarios defined in `specs/setup-portal/spec.md`.
