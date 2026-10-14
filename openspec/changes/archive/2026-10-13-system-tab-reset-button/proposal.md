# Proposal

## Why

Currently, if a user wants to erase all customized settings (such as Wi-Fi credentials, portal password, host adapter MAC, or bound controllers) and restore the device to its default factory state, there is no mechanism in the web portal or REST API. Reverting settings requires re-flashing empty NVS binaries over serial USB or manually modifying individual fields. Providing a configuration reset action directly in the System tab allows users to cleanly wipe all customizations and return the device to its default out-of-the-box state in a single action.

## What Changes

- Add a dedicated **Reset configuration** card to the System tab in the setup web portal (`app/index.html`).
- Implement an in-page confirmation step within the Reset card (explaining that Wi-Fi credentials, portal password, adapter MAC, and bound controllers will be erased) without using native browser dialogs (`window.confirm`).
- Add an authenticated HTTP POST endpoint at `/api/reset` requiring a valid `X-Auth-Token`.
- Implement a configuration wipe routine (`resetConfig()`) in `src/config.cpp` and `include/config.h` that clears all keys in the `bc250` NVS namespace, resets in-memory configuration variables to defaults, sets `forceSetup` to `true` in NVS, and triggers a delayed restart (~800 ms).
- Re-launch the device in SoftAP setup mode after reset so the user can connect and re-provision the device from a clean slate.
- Display a rebooting and restoration notice in the web UI upon confirming the reset.

## Capabilities

### New Capabilities
<!-- none -->

### Modified Capabilities
- `configuration-storage`: add requirements for resetting all stored parameters under the `bc250` NVS namespace to factory defaults and setting `forceSetup` for the subsequent boot.
- `setup-portal`: add requirements for the System tab reset action with in-page confirmation, and the authenticated `/api/reset` endpoint that invokes configuration reset and reboot.

## Impact

- `include/config.h` and `src/config.cpp`: new `resetConfig()` function to wipe NVS and reset runtime state.
- `src/portal.cpp`: route handler for `POST /api/reset`.
- `app/index.html`: new Reset configuration card in System tab with in-page confirmation flow, styles, and script handlers.
- `include/portal_html.h`: regenerated gzipped HTML header via `scripts/build_html.py`.
- Documentation: update README/docs setup portal descriptions.
