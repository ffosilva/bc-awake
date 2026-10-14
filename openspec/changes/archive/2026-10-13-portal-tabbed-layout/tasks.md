# Tasks

## 1. Firmware: finish gating and setup mode reporting

- [x] 1.1 In `src/portal.cpp`, change `handleFinish` to require `authed(req)` and a non-empty `config.passHash` instead of `isConfigured()` (design D1); verify by building (`pio run -e esp32dev`) and, on a device in setup mode with only a password set, `POST /api/finish` with a valid token returns 200 and reboots, and without a token returns 401
- [x] 1.2 Add a portal-local `g_portalSetupMode` flag set at the start of `portalBegin()` and emit `setupMode` in `handleStatus` (design D2); verify `GET /api/status` returns `"setupMode":true` over the SoftAP and `"setupMode":false` over STA in normal mode
- [x] 1.3 Confirm `configured` in `/api/status` and `isConfigured()` are unchanged; verify by inspecting the diff (no edits to `src/config.cpp`) and that `/api/status` still reports `configured:false` on a device with no controllers

## 2. Portal UI: tab shell and navigation

- [x] 2.1 In `app/index.html`, add the tab bar (Bluetooth / Network / System) and three panels, move existing cards into their panels per design D4, and replace post-login `show()` with `showTab()`; verify in a browser that login lands on the Bluetooth tab and switching tabs keeps the session (no 401s in the network log)
- [x] 2.2 Remove `screen-host` and the `needsHost` → `gotoHostScreen()` flow after login; verify on a Classic target with an empty `hostAddr` that login goes straight to the tabbed view
- [x] 2.3 Start BLE `pollDevices` only while the Bluetooth tab is visible and stop it when leaving; verify on the C3 target that `/api/devices` requests stop when switching to Network/System

## 3. Portal UI: Bluetooth tab

- [x] 3.1 Add the adapter card (Classic only, driven by `needsHost`) with MAC input, MAC-validated Save button posting to `/api/host`, help text, and inline message; verify saving a valid MAC shows success and an invalid one keeps Save disabled
- [x] 3.2 Implement `renderWarnings(st)` (design D5): tab badge + adapter notice when `hostAddr` is empty, notice when no controllers are bound; verify the badge and notices appear/disappear after saving the adapter and adding/removing a controller
- [x] 3.3 Disable "Add controller" when no adapter is saved (with an inline hint), alongside the existing board-on and 8-controller conditions, keeping manual MAC add enabled; verify all three disable reasons and that manual add still works with no adapter
- [x] 3.4 Add a "Save selection" button for the BLE picker that posts to `/api/select`; verify on the C3 target that selecting a device and saving persists `wakeAddr` (shown in `/api/status`)

## 4. Portal UI: Network and System tabs

- [x] 4.1 Keep the WoL block in the Network tab, saving only through its own button; update the read-only notice wording to refer to the Network tab; verify that saving works while OFF and the fields are read-only while ON
- [x] 4.2 Move the firmware version and OTA form into the System tab; verify a firmware upload still shows progress and the rebooting message
- [x] 4.3 Add the password change card (new + confirm, ≥6 chars, must match, client-side), posting to `/api/password` and replacing `token` with the returned token (design D6); verify mismatch/short inputs send no request, and after a successful change subsequent API calls (e.g. controller rename) still succeed
- [x] 4.4 Add the exit card, visible only when `setupMode` is true, calling only `/api/finish` (no implicit saves) and showing a rebooting notice; remove the old Finish button and its save-all logic; verify that the card is hidden over STA, and that over the SoftAP exiting with unsaved Network edits does not call `/api/network`

## 5. Docs and integration

- [x] 5.1 Update the "Setup portal" section in `README.md` to describe the tabbed flow (password → Bluetooth / Network / System, exit setup from System, nothing but a password required); verify the steps match the UI
- [x] 5.2 Build all targets (`pio run -e esp32dev -e esp32cam -e esp32-c3-devkitm-1`) and check that the `build_html` gz size in the log is not significantly larger than before
- [x] 5.3 End-to-end on a Classic board after an NVS erase: create the password, land on the tabs, exit setup with nothing else configured, confirm a normal boot with "BT wake disabled" in the serial log and a working button; then run `bc250-sync` once against the device and confirm it still syncs the adapter and controllers
- [x] 5.4 Run `openspec validate portal-tabbed-layout` and confirm it passes
