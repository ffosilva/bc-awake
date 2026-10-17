# Proposal

## Why

Waking the BC250 with a Bluetooth controller on the AI-Thinker ESP32-CAM target can trigger a power ON/OFF flickering loop because the onboard high-power white flash LED (`GPIO 4`) draws ~300 mA at 100% duty cycle at the exact moment `PS_ON#` turns on the ATX PSU and the Bluetooth/Wi-Fi radios transmit, causing 3.3V brownout resets and transient `GPIO 13` (`BOARD_SENSE`) noise. Removing all ESP32-CAM flash LED behavior (firmware, NVS, API, and UI) while hardening Bluetooth wake HCI handling and `STATE_BOOTING` board-sense filtering eliminates this current spike and prevents false boot/shutdown transitions.

## What Changes

- **BREAKING**: Remove all ESP32-CAM flash LED pulsing (`triggerFlash`, `triggerFlashBurst`, `updateFlash`), NVS configuration (`flashOnWake`), portal REST API fields (`isCam`, `flashOnWake`, `/api/settings` handler), and the web UI toggle (`#cam-options` / `#flash-on-wake`).
- Clamp `GPIO 4` (`FLASH_LED`) permanently `LOW` at startup on `BOARD_ESP32CAM` so the onboard flash transistor never fires or glows from PCB trace leakage.
- Gate `OP_REMOTE_NAME_REQ` in Classic Bluetooth `EVT_CONN_REQUEST` handling so remote name resolution only runs when interactive pairing mode (`listenerActive`) is active, avoiding unnecessary radio transmissions during normal controller wake.
- Harden `BOARD_SENSE` (`GPIO 13`) digital reading on `BOARD_ESP32CAM` with multi-sample consensus filtering (`SENSE_OVERSAMPLE` consecutive samples) and require a dedicated `BOARD_ON_DEBOUNCE_MS` (`250 ms`) stable-HIGH window in `STATE_BOOTING` before transitioning to `STATE_ON`.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `controller-wake`: Remove the `ESP32-CAM Flash LED Wake Pulse` requirement and restrict incoming connection `OP_REMOTE_NAME_REQ` paging to active pairing listener sessions.
- `power-control`: Require multi-sample digital filtering on `ESP32-CAM` `BOARD_SENSE` and a 250 ms stable-HIGH debounce window when transitioning from `STATE_BOOTING` to `STATE_ON`.
- `hardware-targets`: Update `AI-Thinker ESP32-CAM Configuration` to remove flash LED support and clamp `GPIO 4` permanently `LOW`.
- `setup-portal`: Remove the `Rapid flash burst on ESP32-CAM pairing start` scenario, `isCam`/`flashOnWake` API fields, and the ESP32-CAM flash option from the Bluetooth tab UI.
- `configuration-storage`: Remove `flashOnWake` from NVS startup loading and factory reset defaults.

## Impact

- **Firmware**: `include/board.h`, `include/config.h`, `include/portal.h`, `src/main.cpp`, `src/config.cpp`, `src/portal.cpp`, `src/bt_classic.cpp`.
- **Web Portal UI**: `app/index.html` and generated `include/portal_html.h`.
- **Documentation**: `docs/pinout.md`.
