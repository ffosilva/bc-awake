# Proposal: Inhibit Power-On During Controller Pairing

## Why

When the BC250 is turned off, attempting to add or pair a wireless controller via the web setup portal ("+ Add controller") frequently triggers an unintended power-on sequence if an already-bound controller pages the impersonated host adapter or if wake triggers fire. When the host powers on, Bluetooth page listening is immediately disabled to prevent radio interference with the PC, and the web portal disables interactive controller pairing.

This change introduces a wake-inhibited pairing mode that prevents controller-based and network-based wake events from turning on the PC while pairing is active, until a 60-second timeout expires, a controller is saved, or the user cancels the interaction.

## What Changes

- **Wake-Inhibited Pairing State**: Entering the controller listener via the setup portal inhibits Bluetooth controller wake and Network Wake (WOL). Incoming Bluetooth connection requests are processed for discovery and naming but do not update the last-seen wake timestamp or assert PS_ON#.
- **Physical Button Exemption**: The momentary case power button remains fully enabled and can power on or control the host at any time.
- **Pairing Timeout (60s)**: Firmware enforces a 60-second timeout on the pairing listener. If no controller is detected within 60 seconds, pairing mode automatically exits and normal wake detection resumes.
- **Explicit Listener Cancellation**: Adds a `POST /api/listener/stop` endpoint so clicking Cancel in the web UI immediately cancels pairing mode and restores normal wake listening.
- **Post-Save Cooldown**: Saving an added controller via `POST /api/controllers/add` arms a 15-second wake cooldown (`WAKE_WAKE_COOLDOWN_MS`) so residual paging packets from the newly bound gamepad do not immediately boot the host.
- **ESP32-CAM Rapid Flash Burst**: On AI-Thinker ESP32-CAM hardware targets with flash indication enabled, entering pairing mode triggers a burst of 3 rapid flashes (60 ms on / 60 ms off) on the onboard white flash LED (GPIO 4), providing immediate physical confirmation that pairing mode is active.
- **Web UI Resilience & Reassurance**:
  - Displays a live countdown ticker (e.g., "Listening... 48s remaining") in the pairing modal.
  - Displays an informational banner reassuring the user that board power-on is paused during pairing and that the case button remains operational.
  - Adds an explicit timeout state with a "Try Again" action if 60 seconds elapse without detection.
  - Displays a 15-second cooldown notice upon saving a controller.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `controller-wake`: Inhibit controller wake triggers while pairing mode is active; arm wake cooldown upon adding a controller.
- `setup-portal`: Support pairing mode lifecycle (60s timeout, `/api/listener/stop` endpoint, UI live countdown and timeout view, post-save cooldown notice, and CAM rapid flash burst).
- `power-control`: Guarantee that momentary button tap power-on remains functional while remote wake sources are inhibited during pairing mode.

## Impact

- **Firmware**: `src/main.cpp`, `src/portal.cpp`, `src/bt_classic.cpp`, `include/portal.h`, `include/bt_classic.h`, and `include/board.h`.
- **Web UI**: `app/index.html` and pre-built header `include/portal_html.h`.
- **Target Hardware**: ESP32-CAM (GPIO 4 flash LED burst), ESP32-WROOM, and ESP32-C3.
