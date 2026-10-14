# Design

## Context

- The portal is a single vanilla HTML/JS file (`app/index.html`) gzipped into `include/portal_html.h` by `scripts/build_html.py` at build time. No framework, no build step beyond gzip; size matters on flash.
- Current flow: `screen-pass` → `screen-host` (always on Classic, since `wakeNeedsHostAddr()` returns true unconditionally) → `screen-dev` (controllers, BLE picker, CAM flag, WoL, Finish) + `card-ota`.
- `/api/finish` gates on `isConfigured()` (password + hostAddr + ≥1 controller on Classic; password + wakeAddr on BLE). `isConfigured()` is also exposed as `configured` in `/api/status`.
- The portal routes are served both in SoftAP setup mode (`portalBegin()`) and in normal mode over STA (`portalServerBegin()` only). `g_setupMode` lives as a `static` in `main.cpp` and is not visible to `portal.cpp`.
- The classic listener depends on `hostAddr`: `wakeSetDiscovery(true)` does nothing when it is empty, so `/api/listener/start` would arm a 60 s session that can never detect a controller.
- `/api/password` already supports authenticated password change and rotates the session token (old token becomes invalid).

## Goals / Non-Goals

**Goals:**
- Tabbed UI with password as the sole gate (see specs).
- Minimal firmware change: `/api/finish` gating and `setupMode` reporting only.
- Keep the single-file, dependency-free portal and keep gzip size roughly flat.

**Non-Goals:**
- No change to `isConfigured()` semantics or the `configured` status field.
- No current-password verification for password change (session auth is sufficient; confirmation is UI-only).
- No server-side enforcement that pairing requires an adapter address (UI-only guard).
- No URL routing / deep links to tabs, no persistence of the selected tab.
- No visual redesign beyond what tabs require (existing colors, card, button styles reused).

## Decisions

### D1. `/api/finish` checks `passHash` directly instead of `isConfigured()`
`handleFinish` will require `authed(req)` and `config.passHash.length() > 0`. Since `authed()` already implies a password was set (a token can only be issued via `/api/password` or `/api/login`), the extra check is defensive.
- *Alternative:* loosen `isConfigured()` itself. Rejected — it changes `configured` in `/api/status` and the `configuration-storage` spec, with no benefit.

### D2. `setupMode` via a portal-local flag set in `portalBegin()`
Add `static bool g_portalSetupMode = false;` in `portal.cpp`, set to `true` at the start of `portalBegin()`, and emit `doc["setupMode"] = g_portalSetupMode` in `handleStatus`. `portalBegin()` is only called when `config.forceSetup` is true, so this mirrors `g_setupMode` exactly.
- *Alternative:* expose `g_setupMode` from `main.cpp` via an accessor in a header. Works too, but adds a cross-module dependency for a value the portal already knows from its own entry point.
- *Alternative:* derive from `config.forceSetup`. Rejected — `forceSetup` is cleared by `/api/finish` before the reboot, so it would briefly lie.

### D3. Tabs implemented as plain buttons toggling `hidden` on three panels
A tab bar of three `<button>`s with an `active` class; each panel is a `<div>` wrapping existing cards. A `showTab(name)` function replaces the current `show(screen)` for post-login navigation; `show("pass")` remains for the password screen.
- *Alternative:* CSS-only radio-button tabs. Rejected — we need JS hooks anyway (warning badges, BLE polling start/stop).
- BLE device polling (`pollDevices`) runs only while the Bluetooth tab is visible, to avoid needless requests.

### D4. Layout and element ownership

```
header: title | power badge | fw badge
[ Bluetooth (!) ] [ Network ] [ System ]

Bluetooth panel
  - adapter card (Classic only): MAC input + [Save] + help + msg
  - controllers card (Classic) | BLE picker card (BLE, + [Save selection])
  - CAM flash option (isCam)   -- saves on change (existing behavior)
Network panel
  - existing WoL block, unchanged fields + [Save Network Settings]
System panel
  - firmware card (version + existing OTA form)
  - password card: new + confirm + [Change password] + msg
  - exit card (setupMode only): [Exit setup mode & reboot] + msg
```

- BLE selection previously relied on the Finish button to call `/api/select`. With per-tab saving it gets its own "Save selection" button in the Bluetooth tab.
- Existing element ids (`host`, `ctrl-list`, `wol-*`, `ota-*`, modal ids) are kept where possible to limit JS churn. `screen-host` and its `needsHost` flow are removed; the `needsHost` status field is still used to decide whether to render the adapter card.

### D5. Warnings are computed from `/api/status` in one place
A `renderWarnings(st)` function sets the Bluetooth tab badge and inline notices: Classic with empty `hostAddr` → adapter notice + badge, add-controller disabled with hint; no controllers → notice. It runs after login, after saving the adapter, and after controller changes. `applyBoardStateLocks()` keeps its role and is combined with the adapter check when deciding `btn-add-ctrl.disabled` (disabled if board on OR no adapter OR 8 controllers).

### D6. Password change reuses `/api/password`
Validate client-side (length ≥ 6, both fields equal), POST `{password}`, then set `token = data.token`. Any in-flight intervals (power poll, listener poll) read the global `token`, so swapping it is sufficient.

## Risks / Trade-offs

- [User exits setup with nothing configured on Classic] → Device boots normally with BT wake disabled (existing firmware behavior; button still works). The inline warnings make this visible before exiting.
- [Token rotation on password change breaks concurrently open tabs/devices] → Expected single-session model; other sessions get 401 and must log in again. Acceptable.
- [Server still allows `/api/listener/start` without hostAddr] → UI guard only; a direct API call yields a session that just times out. Harmless, out of scope.
- [Losing the implicit "save everything on Finish"] → Users may edit Network fields and exit without saving. Mitigated by per-tab save feedback; optional dirty-state hint is a nice-to-have, not required.
- [HTML size growth] → Mostly restructuring; removing `screen-host` offsets the tab bar. Verify gz size in the build log.

## Migration Plan

- Firmware and UI ship together in one image (UI is embedded), so there is no version skew between them.
- `bc250-sync` keeps working: it uses `/api/status` (`hostAddr`), `/api/host`, and `/api/controllers/*`, none of which change.
- Rollback: flash the previous firmware via OTA or serial; NVS layout is unchanged.
