# Tasks

## 1. Pin definitions and configuration flag

- [x] 1.1 Add `PWR_BTN_PIN` to [include/board.h](../../../include/board.h) for each target (CAM GPIO2, DevKitC GPIO27, C3 GPIO7) with wiring comments; verify `pio run -e esp32cam -e esp32dev -e esp32-c3-devkitm-1` still compiles
- [x] 1.2 Add `gracefulShutdown` to `Config` in [include/config.h](../../../include/config.h) with `setGracefulShutdown(bool)`; load it in `loadConfig()` (default false when the key is absent) and persist it immediately in the setter in [src/config.cpp](../../../src/config.cpp); verify via serial log that the value survives a reboot after being set
- [x] 1.3 Reset `gracefulShutdown` to false in `resetConfig()`; verify via serial log (or `/api/status`) that it is false after `/api/reset`

## 2. Power button mirroring and shutdown pulse

- [x] 2.1 In [src/main.cpp](../../../src/main.cpp), keep `PWR_BTN_PIN` a high-impedance `INPUT` at boot and with the feature disabled; add helpers to drive it LOW (`OUTPUT` + `LOW`) and release it back to `INPUT`; verify with the feature disabled that the pin never leaves `INPUT` (multimeter or serial log)
- [x] 2.2 Mirror the debounced physical button to `PWR_BTN_PIN` while enabled, in `STATE_ON`, and only for presses that began while ON (`!pressStartedOff`); release the pin on button release and on any transition out of `STATE_ON` (including `powerOff()`); verify on hardware with the feature on that a short press makes the BC250 start a clean OS shutdown and `PWR_BTN#` returns to ~3.4 V on release
- [x] 2.3 Confirm presses beginning in `STATE_OFF` (power-on tap, 8 s setup hold) or `STATE_BOOTING` never drive the pin, and that the 5 s hold still releases `PS_ON#` and the pin; verify by serial log on each case
- [x] 2.4 Add a non-blocking 300 ms shutdown pulse (pulse-until timestamp serviced from `normalLoop()`) exposed as a function the portal can call; it returns failure when the feature is disabled or the state is not `STATE_ON`, and ignores a request while a pulse is active; verify via serial log that the pulse lasts ~300 ms and other loop logging continues during it
- [x] 2.5 Add serial log lines for mirror press, release, pulse start and pulse end, and the `[HB ]` heartbeat state; verify they appear while exercising 2.2 and 2.4
- [ ] 2.6 Test on the BC250 whether holding the button ~4 s makes the board power off through the motherboard override before the 5 s `PS_ON#` cut, and record the observed behavior in the pinout doc (task 4.2); verify TPMS1 drops and the system lands in `STATE_OFF` with `PS_ON#` released

## 3. Portal API and UI

- [x] 3.1 In [src/portal.cpp](../../../src/portal.cpp), add `gracefulShutdown` to the `/api/status` JSON; add authenticated `POST /api/power` that validates a boolean `gracefulShutdown` and saves it; verify with curl: 401 without `X-Auth-Token`, `{"ok":true}` with a valid token, and the value reported by `/api/status`
- [x] 3.2 Add authenticated `POST /api/shutdown` calling the pulse function from 2.4: 200 `{"ok":true}` on success, 409 when disabled or not ON, 401 without a token; verify each case with curl
- [x] 3.3 In [app/index.html](../../../app/index.html) System tab, add an "optional" graceful shutdown toggle with a wiring note and Save, a Shut down button enabled only when the feature is on and `boardState` is ON, and inline result messages; wire them to `/api/power` and `/api/shutdown`, and load the toggle from `/api/status`; verify in a browser against the device (toggle persists across page reload, button state tracks power state)
- [x] 3.4 Regenerate [include/portal_html.h](../../../include/portal_html.h) via `scripts/build_html.py` (runs automatically in the PlatformIO build) and verify the firmware builds for all three environments and the portal served by the device shows the new System tab controls

## 4. Documentation

- [x] 4.1 Update [docs/pinout.md](../../../docs/pinout.md): add a `PWR_BTN_PIN` row to the GPIO table, per-board pin rationale, the optional wiring to the motherboard `PWR_BTN#` pin (~3.4 V, direct wire, ESP32 never drives HIGH), and header diagrams for CAM/DevKitC/C3; verify every pin number matches `board.h`
- [ ] 4.2 In `docs/pinout.md`, document the ESP32-CAM GPIO2 caveat (unplug the `PWR_BTN#` wire to flash over UART; OTA unaffected), the 5 s `PS_ON#` backstop, and the result of the hardware override test from 2.6; verify the text is consistent with the design risks
- [x] 4.3 Update [README.md](../../../README.md) with the optional graceful shutdown feature (what it does, how to enable it in the portal System tab, short press vs. long press behavior); verify the described behavior matches the implemented specs

## 5. Integration check

- [ ] 5.1 End-to-end on hardware with the feature enabled: short press while ON gives a clean OS shutdown and the system follows to `STATE_OFF`; portal Shut down does the same; 5 s hold still hard-cuts; power-on tap and 8 s setup hold unchanged; verify using the serial log and observed machine behavior
- [ ] 5.2 With the feature disabled, confirm behavior is identical to before (no pin activity, Shut down rejected, existing hold/tap behavior unchanged); verify via serial log and `curl` on `/api/shutdown`
