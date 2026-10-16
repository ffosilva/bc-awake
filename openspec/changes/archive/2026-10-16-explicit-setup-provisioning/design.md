# Technical Design: Explicit Setup Provisioning

## Context

See `proposal.md` for motivation. Currently, `src/main.cpp` executes `portalBegin()` on startup whenever `!isConfigured() || config.forceSetup`. Because `isConfigured()` requires `passHash`, `controllers` (gamepads), and `hostAddr` (BlueZ adapter), devices that are set up solely for Wi-Fi/WOL or configured incrementally re-enter setup mode on every reboot.

## Goals / Non-Goals

**Goals:**
- Provide an explicit persistent `provisioned` boolean flag in NVS (`bc250` namespace, key `"provisioned"`).
- Mark `provisioned = true` whenever setup is formally finished via `/api/finish` or button-hold exit from setup mode.
- Clear `provisioned` to `false` when factory reset is triggered (`resetConfig()` / `/api/reset`).
- Update `isConfigured()` to return `true` if `provisioned` is true, retaining backward-compatibility fallback for existing configured boards.

**Non-Goals:**
- Changing Bluetooth pairing requirements or BlueZ controller binding APIs.
- Removing or altering the 8s button hold while OFF to manually re-enter setup mode.

## Decisions

### 1. Dedicated NVS Key for Provisioning Status
*Decision*: Store an explicit boolean key `"provisioned"` in the `"bc250"` Preferences namespace (11 characters, well within the 15-character limit).
*Rationale*: Separates the lifecycle state ("has this board completed initial onboarding?") from domain-specific features (Bluetooth gamepad vs. Wi-Fi WOL).
*Alternatives Considered*:
- Checking `wifiSsid.length() > 0 || !controllers.empty()` directly inside `isConfigured()` (rejected: does not protect users who want to set password first without Wi-Fi, and does not clearly convey whether the user actually finished onboarding).

### 2. Dual-Criteria `isConfigured()` Evaluation
*Decision*:
```cpp
bool isConfigured() {
  if (config.provisioned) {
    return true;
  }
#if defined(WAKE_BLE)
  return config.passHash.length() > 0 && config.wakeAddr.length() > 0;
#else
  return config.passHash.length() > 0 && !config.controllers.empty() &&
         config.hostAddr.length() > 0;
#endif
}
```
*Rationale*: Ensures boards already in the field that update firmware seamlessly continue working without forcing users to re-run setup, while new or reset boards benefit from the explicit `provisioned` lifecycle.

### 3. Automatic Provisioning Persistence on Normal Startup
*Decision*: In `loadConfig()`, if the legacy criteria evaluate to `true` but `config.provisioned` was `false`, automatically commit `setProvisioned(true)` to NVS.
*Rationale*: Migrates legacy boards forward immediately on first boot under the new firmware.

## Risks / Trade-offs

- **[Risk]** User calls `/api/finish` without configuring any wake method (neither Wi-Fi nor Bluetooth).
  - *Mitigation*: The device still honors button power toggles and force shutdown. If the user later wants to configure a wake method, holding the button for 8s while OFF or sending `'s'` via serial re-enters setup portal at any time.
