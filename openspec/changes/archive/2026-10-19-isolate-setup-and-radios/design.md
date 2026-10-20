# Design

## Context

See [proposal.md](file:///Users/ffosilva/repos/ffosilva/bc-awake/openspec/changes/isolate-setup-and-radios/proposal.md) for problem motivation.

Currently, the ESP32 maintains active wireless stacks across all lifecycle states:
- In `STATE_OFF`: Bluetooth page scan and Wi-Fi station (WOL) run simultaneously, with the web portal server and mDNS active on port 80.
- On boot / in `STATE_ON`: Bluetooth keeps its background controller active, while Wi-Fi executes a deferred 5-second MAC address swap (`applyStateMacChange()`), reconnecting to the access point and running the HTTP server, mDNS, and UDP discovery.
- In Setup Mode: `normalBegin()` runs before `g_setupMode` is set, starting Wi-Fi station mode alongside SoftAP and Bluetooth discovery, while allowing button presses, wake events, or `/api/power/on` calls to energize the host PC.

This design decouples wireless radio lifecycles from host operation and strictly isolates Setup Mode.

## Goals / Non-Goals

**Goals:**
- Completely power down Bluetooth and Wi-Fi radios during `STATE_BOOTING` and `STATE_ON`, restoring them only upon transition back to `STATE_OFF` in normal operation mode.
- Eliminate the 5-second deferred MAC address swap, fallback SoftAP activation while running, and all background discovery services.
- Prevent the host PC from powering on while in Setup Mode across all physical and network triggers, and prevent Wi-Fi station mode from running during Setup Mode.
- Restrict the HTTP web portal and REST API strictly to Setup Mode over SoftAP (`http://192.168.4.1`), removing unreachable remote power action endpoints and UI buttons.
- Completely remove `bc250-sync` and `bc250-setup` tooling and documentation.

**Non-Goals:**
- Remote host monitoring or power control over Wi-Fi while the PC is running or in normal standby.
- Runtime gamepad pairing synchronization from host BlueZ to the ESP32.
- Running onboarding scripts directly from the host BC250 while connected to the ESP32 SoftAP.

## Decisions

### 1. Clean Boot Branching & Setup Mode Isolation
In `src/main.cpp`:
- Determine `g_setupMode = (!isConfigured() || config.forceSetup)` in `setup()` **before** initializing radios or adopting board state.
- When `g_setupMode` is true:
  - Initialize GPIOs with `PS_ON#` released (`STATE_OFF`). Never adopt `STATE_ON` or assert `psuOn()`.
  - Do not call `networkWakeBegin()` or `networkWakeLoop()` (preventing Wi-Fi STA from competing with SoftAP).
  - Call `portalBegin()` to start the SoftAP (`BC250_<SUFFIX>`), HTTP web server, and Bluetooth discovery for controller lookup.
  - Ignore short button presses and wake events in `normalLoop()`; only the 8-second button hold (or `/api/finish` in the portal) exits Setup Mode.
- Block serial `'s'` / `'S'` setup trigger when `state != STATE_OFF` so a running host PC cannot be abruptly cut off by a serial command.

*Rationale*: Separating Setup Mode from Normal Mode at boot ensures Wi-Fi STA and WOL never fight the SoftAP radio, and guarantees the host PC remains unpowered throughout configuration.

### 2. Unified Radio Teardown Hook in `setState()`
When the power controller state machine transitions in Normal Mode:
- **Booting / Running (`STATE_BOOTING`, `STATE_ON`)**:
  - `wakeStop()` is invoked: disables page scan / advertising and tears down the Bluetooth controller (`btEnd()` calling `esp_bt_controller_disable()` + `esp_bt_controller_deinit()` on Classic BT; `NimBLEDevice::deinit(true)` on BLE targets).
  - `networkWakeStop()` is invoked: stops UDP listeners, drops Wi-Fi station connection (`WiFi.disconnect(true)`), and powers off the Wi-Fi driver (`WiFi.mode(WIFI_OFF)`).
  - If the ESP32 boots directly into `STATE_ON` (`boardSenseStable == true` at startup), `startWake()` and `networkWakeBegin()` are not called.
- **Standby (`STATE_OFF`)**:
  - `startWake()` reinitializes Bluetooth wake listening.
  - `networkWakeBegin(true)` reinitializes Wi-Fi station mode with `bc250Mac` (if `wolEnabled` is set) and begins listening for UDP WOL packets on ports 7 and 9.

*Rationale*: Tying radio shutdown directly to `setState()` (and skipping radio startup if already `STATE_ON` at boot) guarantees zero RF current spikes during ATX inrush and eliminates Bluetooth MAC collisions with the host PC's internal adapter.

### 3. Elimination of Post-Boot Deferred MAC Swap
In `src/network_wake.cpp`:
- Remove `MAC_SWAP_DEFER_MS`, `g_pendingMacSwap`, `g_pendingMacSwapAt`, and `applyStateMacChange()`.
- Remove UDP 25025 discovery listener (`BC250_DISCOVER`).
- Remove the 20-second fallback SoftAP activation during running states.

*Rationale*: Because Wi-Fi is completely disabled while the host PC is powered on, the ESP32 is never on the local network at the same time as the host PC. No MAC collision can occur, eliminating the 5-second post-boot MAC swap and the random flashing/brownouts it triggered.

### 4. Web Portal Restricted to SoftAP & Removal of Remote Power Actions
- `portalServerBegin()` is called strictly from `portalBegin()` in Setup Mode. Remove `portalServerBegin()` and `MDNS.begin()` from `networkWakeLoop()`.
- Remove `/api/power/on`, `/api/power/off`, and `/api/shutdown` endpoints from `src/portal.cpp`, along with `requestPowerOn()`, `requestPowerOff()`, `requestGracefulShutdown()`, and the 300 ms API pulse logic in `src/main.cpp`.
- In `app/index.html`:
  - Remove the ATX Power Supply action card (`Power On` / `Force Cut Power`), the Force Power Off confirmation modal, and the `Shut Down OS` button.
  - Retain the **Power** tab for configuring the optional motherboard power button (`gracefulShutdown` checkbox + Save).
  - Update the pairing modal notice (removing "(Case power button remains active)") and the hostname help text (removing `http://<hostname>.local`).

*Rationale*: Since the portal is only active in Setup Mode and the host PC is always locked off in Setup Mode, remote power control buttons in the web UI can never be used. Removing them simplifies both firmware and UI.

### 5. Decommissioning of `bc250-sync` and `bc250-setup`
- Delete `tools/bc250-sync/` and `tools/bc250-setup/`.
- Delete `docs/host-sync.md`.
- Update `README.md` to document configuring the device over Setup SoftAP from a phone or external computer while the host board is off.

*Rationale*: Both tools relied on the BC250 being powered on while communicating with the ESP32 over Wi-Fi (`bc250-sync` in normal mode, `bc250-setup` in Setup Mode). With host power locked off in Setup Mode and radios off in `STATE_ON`, both tools are obsolete.

## Risks / Trade-offs

- **[Risk] Radio re-initialization failure when returning to standby**:
  → *Mitigation*: Standby transition occurs only after board sense confirms the motherboard is fully down (TPMS1 LOW for 1500 ms). Power rails are stable, and `btEnd()` / `NimBLEDevice::deinit(true)` cleanly reset internal state flags before `wakeBegin()` is called again.
- **[Trade-off] No web portal access while the PC is powered on or in normal standby**:
  → *Mitigation*: Setup portal is strictly for configuration and OTA updates. Holding the button 8s while the machine is off re-enters Setup Mode whenever changes are needed.
- **[Trade-off] Initial setup requires a phone, tablet, or secondary computer**:
  → *Mitigation*: The embedded web UI at `http://192.168.4.1` works out-of-the-box on any mobile browser and includes interactive Bluetooth controller discovery.
