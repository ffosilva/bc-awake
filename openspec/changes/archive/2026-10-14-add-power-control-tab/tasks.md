# Tasks

## 1. Firmware Power Control Flags and Loop Handling

- [x] 1.1 In [src/main.cpp](../../../src/main.cpp) and [include/portal.h](../../../include/portal.h), declare and implement `requestPowerOn()` and `requestPowerOff()` using non-blocking volatile flags (`powerOnRequested`, `powerOffRequested`); in `normalLoop()`, service `powerOnRequested` by calling `powerOn("web portal", now)` if in `STATE_OFF`, and service `powerOffRequested` by calling `powerOff("web portal force off", now)` if not in `STATE_OFF`; verify via `pio run -e esp32cam -e esp32dev -e esp32-c3-devkitm-1` that all environments compile.
- [x] 1.2 In `src/main.cpp`, ensure `requestPowerOn()` cancels active controller pairing if pairing is in progress, and ensure `requestPowerOff()` arms the wake cooldown and resets pulse state; verify via code inspection against existing physical button logic.

## 2. Portal REST Endpoints

- [x] 2.1 In [src/portal.cpp](../../../src/portal.cpp), implement authenticated handlers for `POST /api/power/on` and `POST /api/power/off`; return 401 without valid `X-Auth-Token`, return 409 Conflict if `/api/power/on` is called outside `STATE_OFF` or if `/api/power/off` is called while in `STATE_OFF`, and return 200 `{"ok":true}` on success; register routes in `registerRoutes()`.
- [x] 2.2 Verify endpoint responses and state validation using `curl` or automated HTTP request simulation (401 without token, 409 on invalid state, 200 on valid state transition).

## 3. Web UI Power Tab and Confirmation Modal

- [x] 3.1 In [app/index.html](../../../app/index.html), add the Power tab button (`#tab-btn-pwr`) between Network and System in `#tabs-nav`.
- [x] 3.2 In `app/index.html`, add the Power panel container (`#panel-pwr`) with Power Status card, ATX Power Supply card ("Power On" and "Force Cut Power" buttons), and Motherboard Power Button card.
- [x] 3.3 Move the Graceful Shutdown toggle and action button out of `#panel-sys` into `#panel-pwr`, updating the pulse button label to "Shut Down OS".
- [x] 3.4 In `app/index.html`, add the Force Cut confirmation modal overlay (`#modal-force-off`) containing warning messaging, confirmation button ("Yes, Cut Power"), and Cancel button.
- [x] 3.5 In `app/index.html` JavaScript, wire tab switching for the Power tab, attach event handlers for "Power On", "Force Cut Power" modal open/confirm/cancel, and "Shut Down OS", and update `updateBoardState()` to reflect button enable/disable states for `STATE_OFF`, `STATE_BOOTING`, and `STATE_ON`.
- [x] 3.6 Regenerate [include/portal_html.h](../../../include/portal_html.h) via `python3 scripts/build_html.py` and verify all target environments (`esp32cam`, `esp32dev`, `esp32-c3-devkitm-1`) build without errors.

## 4. Documentation and Integration Verification

- [x] 4.1 Update [README.md](../../../README.md) to describe the Web UI Power tab layout, remote power-on, force cut power safeguards, and graceful shutdown controls.
- [x] 4.2 End-to-end verification in a browser: verify tab navigation switches between Bluetooth, Network, Power, and System without state loss; verify "Power On" is active in `STATE_OFF`; verify "Force Cut Power" opens the modal and requires explicit confirmation; verify "Shut Down OS" is enabled only when wired feature is enabled and system is `STATE_ON`.
