# Proposal: Captive-Portal Inline Controller Rename & UI Resilience

## Why

When connecting to the setup portal via captive network environments (such as Apple's Captive Network Assistant or Android's CaptivePortalLogin WebView), browser-native dialogs (`window.prompt`, `window.alert`, `window.confirm`) are suppressed or return `null`. The bound Bluetooth controller "Rename" button currently relies on `window.prompt()`, causing click events to silently fail with zero visual feedback. Formalizing captive-portal UI resilience ensures that setup portal interactions remain completely functional across captive network browsers by requiring in-page DOM interactions instead of native modal dialogs.

## What Changes

- **Inline Controller Renaming**: Replace the `window.prompt()` interaction with an inline row editor in `app/index.html`. Clicking "Rename" swaps the friendly name label for an input field and toggles the action buttons to "Save" and "Cancel".
- **Captive-Portal UI Resilience Requirement**: Formalize in the `setup-portal` specification that the captive web interface SHALL handle all user inputs, edits, and confirmations via in-page DOM elements rather than browser-native modal dialogs.

## Capabilities

### New Capabilities
*(None; builds on existing capabilities)*

### Modified Capabilities
- `setup-portal`: Formalize captive portal web interface interaction requirements to require in-page DOM elements for user input, and update controller rename scenarios to reflect in-page editing without native dialog prompts.

## Impact
- **Web Portal Assets**: `app/index.html` (repacked into SPIFFS filesystem image).
- **Backend APIs**: None. `/api/controllers/rename` continues to be used as-is.
- **Hardware Targets**: All boards hosting the setup portal (`esp32dev`, `esp32cam`, and `esp32c3`).
