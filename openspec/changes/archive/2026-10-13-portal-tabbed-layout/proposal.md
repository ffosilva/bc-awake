# Proposal

## Why

The portal is a single linear wizard: after login, Classic BT users are always forced through the Bluetooth adapter MAC screen (even when it is already saved), and everything else is packed into one long card whose "Finish setup" button is blocked until a controller is bound. This made sense for a first-boot SoftAP wizard, but the portal now also runs persistently over Wi-Fi in normal mode and Wi-Fi/WoL-only setups are legitimate — the firmware already degrades gracefully (BT wake disabled) when Bluetooth settings are missing. The UI and `/api/finish` are stricter than the device actually needs.

## What Changes

- Reorganize the authenticated portal UI into three tabs: **Bluetooth**, **Network**, **System**.
- The password screen (create with confirmation, or login) remains the only blocking gate; after it the user lands on the tabbed view directly.
- **Remove** the forced adapter MAC screen. The adapter MAC becomes an editable field inside the Bluetooth tab (Classic targets only).
- Interactive "Add controller" is disabled with an inline hint until an adapter MAC is saved (the listener cannot detect anything without it). Manual MAC entry remains available.
- Missing settings are surfaced as non-blocking warnings (tab indicator / inline notice) instead of gates.
- Each tab saves its own settings independently. Exiting setup no longer implicitly saves network or flash settings.
- System tab hosts: firmware version + OTA upload, change password (new + confirmation, checked client-side), and "Exit setup mode & reboot".
- `/api/status` gains a `setupMode` boolean; the Exit button is shown only when `setupMode` is true.
- **BREAKING (API behavior)**: `/api/finish` requires only an authenticated session with a password set; it no longer requires a bound controller or adapter MAC.
- The `configured` field in `/api/status` and `isConfigured()` keep their current meaning (no change for `bc250-sync` or other consumers).

## Capabilities

### New Capabilities
<!-- none -->

### Modified Capabilities
- `setup-portal`: completing setup only requires a password; `/api/status` reports `setupMode`; portal UI is organized into tabs with password as the only gate; adapter MAC editing and controller pairing move into the Bluetooth tab with a pairing dependency on the adapter MAC; password change with confirmation; read-only network settings scenario refers to the Network tab instead of a card.

## Impact

- `app/index.html` (and generated `include/portal_html.h`): major restructure of screens, navigation, and save flows.
- `src/portal.cpp`: `handleFinish` gating, `handleStatus` adds `setupMode`, portal-local setup-mode flag set in `portalBegin()`.
- `README.md`: setup portal walkthrough updated to the tabbed flow.
- No NVS schema changes. `tools/bc250-sync` unaffected (uses `/api/host`, `/api/controllers/*`, `hostAddr`).
