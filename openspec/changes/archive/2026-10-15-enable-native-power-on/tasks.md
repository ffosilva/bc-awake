# Tasks

## 1. Pin Mode and Lifecycle Management

- [x] 1.1 In `src/main.cpp`, update `PWR_BTN_PIN` mode configuration to dynamically set `INPUT_PULLUP` when entering `STATE_OFF` with `config.gracefulShutdown` enabled, and high-impedance `INPUT` when entering `STATE_BOOTING`, `STATE_ON`, or when `config.gracefulShutdown` is disabled; verify compilation on all environments with `pio run`.
- [x] 1.2 In `src/main.cpp`, ensure `normalBegin()` and state transitions (`powerOn()`, `powerOff()`, `setState()`) apply the correct `PWR_BTN_PIN` mode on boot and state changes; verify via serial logging on state transitions.

## 2. Button Input Processing and Debouncing

- [x] 2.1 In `src/main.cpp`, unify raw button input sampling in `normalLoop()` to evaluate both `BUTTON_SENSE` and `PWR_BTN_PIN` (when `config.gracefulShutdown` is enabled) into `buttonRaw`; verify that pressing either input triggers the unified debounce window.
- [x] 2.2 In `src/main.cpp`, verify that a short tap on `PWR_BTN_PIN` while in `STATE_OFF` triggers `powerOn()`, holding `PWR_BTN_PIN` ≥ 8 s while in `STATE_OFF` triggers `enterSetupMode()`, and holding `PWR_BTN_PIN` ≥ 5 s while in `STATE_ON` triggers `powerOff()`.
- [x] 2.3 In `src/main.cpp`, update `updatePwrBtn()` so `PWR_BTN_PIN` is only actively driven LOW for external switch presses (`BUTTON_SENSE`) or programmatic shutdown pulses, keeping `PWR_BTN_PIN` as `INPUT` during native button presses.

## 3. UI and Documentation Updates

- [x] 3.1 In `app/index.html`, update the description for the motherboard power button toggle to explain that enabling the feature supports both native power-on when off and clean OS shutdown when on; run `python3 scripts/build_html.py` to regenerate `include/portal_html.h` and verify clean build.
- [x] 3.2 In `docs/pinout.md` and `README.md`, update descriptions and tables for `PWR_BTN_PIN` to document bidirectional power button functionality and `INPUT_PULLUP` behavior during `STATE_OFF`.

## 4. Verification and Multi-Target Build

- [x] 4.1 Compile the firmware across all targets (`pio run -e esp32dev -e esp32cam -e esp32-c3-devkitm-1`) and verify zero compiler warnings or errors.
- [x] 4.2 Validate the change against OpenSpec standards using `openspec validate enable-native-power-on --strict`.
