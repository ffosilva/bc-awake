# Design: Inhibit Power-On During Controller Pairing

## Context

The BC250 ESP32 switch runs continuous Classic Bluetooth page listening in `STATE_OFF` (impersonating the PC adapter MAC) while simultaneously serving the persistent setup web portal over Wi-Fi Station mode.

Currently, entering the interactive controller pairing listener via the portal (`POST /api/listener/start`) only sets `listenerActive = true` in `bt_classic.cpp`. In `normalLoop()`, controller wake and network wake logic continue executing uninhibited. If a controller paging during the pairing session matches any bound MAC in `wakeAddrs`, `EVT_CONN_REQUEST` updates `wakeLastSeen`, causing `normalLoop()` to execute `powerOn("controller present")`. The resulting transition to `STATE_BOOTING` stops Bluetooth page scanning, spins up the PC, and locks the web UI into the powered-on state.

## Goals / Non-Goals

**Goals:**
- Provide a robust pairing lifecycle on firmware with an explicit 60-second timeout.
- Inhibit Bluetooth controller wake and WOL wake triggers while pairing mode is active.
- Ensure incoming connection requests from already-bound controllers report as detected in the listener without arming wake or asserting PS_ON#.
- Maintain physical case power button operation as an override at all times.
- Provide physical indicator feedback (3 rapid flashes) on ESP32-CAM targets upon starting pairing.
- Arm a 15-second wake cooldown upon saving an added controller to drain residual paging packets.
- Update the web portal interface with a live countdown, informational reassurance banner, explicit timeout handling, and stop/cancel integration.

**Non-Goals:**
- Modifying BLE scanning behavior on ESP32-C3 targets (ESP32-C3 uses continuous background advertisement scanning and does not implement the page-scan listener modal).
- Changing long-term button hold behaviors (5s force-off or 8s setup mode entry).

## Decisions

### 1. Centralized Pairing Mode State in Firmware
Pairing mode state is tracked with an active flag and expiration timestamp (`g_pairingTimeoutAt = millis() + 60000`) in `portal.cpp` and exposed via `portalIsPairingActive()`.
- **Alternatives considered**:
  - *Tracking only in frontend JS*: Rejected because if the browser tab closes or loses Wi-Fi, the switch would either never time out or lack firmware enforcement.
  - *Extending `wakeInhibitUntil` directly*: Insufficient because `wakeInhibitUntil` is a passive timestamp that does not distinguish whether an interactive listener session is active or should capture incoming MACs.

### 2. Disarming Wake Triggers in HCI Event Handler and Main Loop
In `bt_classic.cpp`, when `EVT_CONN_REQUEST` is processed:
- If `portalIsPairingActive()` (or `listenerActive`) is true: the incoming MAC is recorded for `listenerResult`, but `wakeLastSeen` is NOT updated, even if the MAC matches `wakeAddrs`.
- In `main.cpp`: `normalLoop()` checks `!portalIsPairingActive()` before evaluating `wakePresent` and `wolPresent`.
- If the physical power button is pressed while OFF during pairing: `powerOn()` executes normally, clears pairing mode, and enters `STATE_BOOTING`.

### 3. Dedicated Cancel Endpoint (`/api/listener/stop`)
When the user cancels the modal or closes the session, the web UI sends `POST /api/listener/stop`.
- Endpoint sets `g_pairingTimeoutAt = 0` and clears listener state immediately.
- Returns `{"ok": true}`.

### 4. Non-Blocking Rapid Flash LED Sequence for ESP32-CAM
On ESP32-CAM targets, GPIO 4 is controlled by a state machine in `main.cpp`:
- `triggerFlashBurst(count, onMs, offMs)` schedules 3 pulses (60 ms ON, 60 ms OFF).
- Handled inside `updateFlash(now)` so no blocking `delay()` stalls the main loop or web server.

### 5. Post-Save Cooldown Arming
When `POST /api/controllers/add` succeeds:
- Pairing mode is deactivated.
- `wakeInhibitUntil` is set to `millis() + WAKE_WAKE_COOLDOWN_MS` (15 seconds).
- The web UI displays confirmation with a note that wake resumes in 15 seconds.

## Risks / Trade-offs

- **[Risk] Browser loses connection while modal is open**
  → *Mitigation*: Firmware tracks `millis() >= g_pairingTimeoutAt` autonomously. If 60 seconds elapse without detection, the firmware automatically exits pairing mode and re-enables wake.
- **[Risk] Controller continues paging after user clicks Save Controller**
  → *Mitigation*: The 15-second wake cooldown armed on `POST /api/controllers/add` ensures lingering packets are ignored while the controller goes back to sleep.
- **[Risk] User presses physical button while pairing mode is active**
  → *Mitigation*: The physical button check explicitly bypasses pairing inhibition, asserts PS_ON#, and cancels pairing mode cleanly.
