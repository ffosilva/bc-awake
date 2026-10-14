# Proposal

## Why

Updating the ESP32 firmware via OTA and rebooting the microcontroller while the BC250 host machine is powered on can destabilize the running system, disrupt power control signals (such as PS_ON# or PWR_BTN#), or cause unexpected state desynchronization. Restricting OTA firmware updates to when the host board is turned off (STATE_OFF) prevents accidental interruption or corruption of an active host session.

## What Changes

- Reject firmware upload requests at `/api/update` with HTTP 409 Conflict when the host machine is powered on (`STATE_ON` or `STATE_BOOTING`), aborting the update before modifying flash memory.
- In the setup portal web interface (System tab), disable firmware binary file selection and the upload action when the host machine is powered on.
- Display an informative notice in the OTA firmware update section indicating that firmware updates are only permitted while the host machine is turned off.
- Automatically re-enable firmware update controls when the board transitions to `STATE_OFF`.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `setup-portal`: Restrict the `/api/update` endpoint and UI controls to when the host machine is in `STATE_OFF`.

## Impact

- **Backend API**: `/api/update` will validate host power state (`isBoardPowered()`) and reject with HTTP 409 Conflict if the board is powered on or booting.
- **Web Portal UI**: `app/index.html` (and compiled `include/portal_html.h`) will disable the file input (`#ota-file`) and update button (`#ota-btn`) and display an informational lock message when `currentBoardState !== "OFF"`.
- **Dependencies**: No external library or hardware dependency changes.
