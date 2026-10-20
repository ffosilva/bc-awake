# Proposal

## Why

Simultaneous operation of Wi-Fi, Bluetooth, and ATX power supply state transitions on the ESP32 causes high current draw, RF contention, brownout resets (`ESP_RST_BROWNOUT`), and erratic hardware behavior—notably flash LED glitching and radio resets caused by a 5-second post-boot MAC swap. Furthermore, the setup portal currently allows the host PC to power on during configuration, and the web server remains exposed during normal station operation.

Completely disabling both radios while the host PC is powered on, locking out host power-up while in Setup Mode, restricting the web portal server strictly to Setup SoftAP mode, and removing obsolete host-side scripts and remote power endpoints drastically simplifies the system architecture, eliminates post-boot radio churn, and prevents brownout resets.

## What Changes

- **Radio Shutdown in Running States**: When the host machine transitions to `STATE_BOOTING` or `STATE_ON` (or boots directly into `STATE_ON`), both Bluetooth and Wi-Fi station radios are completely disabled (`WiFi.mode(WIFI_OFF)` and Bluetooth controller teardown). Both are initialized or re-initialized only in `STATE_OFF` (standby).
- **Elimination of Post-Boot 5s MAC Swap**: Remove the 5-second deferred MAC address swap (`MAC_SWAP_DEFER_MS`), fallback SoftAP timer while powered on, and all station reconnection churn during running states.
- **Isolated Setup Mode (Host Power Lockout)**: Determine `g_setupMode` before starting normal mode radios. In Setup Mode, the host PC is strictly prevented from powering up (`PS_ON#` remains released, button taps are ignored while 8-second hold exits setup mode, wake triggers are disabled, and Wi-Fi STA / WOL does not run). Serial `'s'` command to enter setup mode is ignored unless in `STATE_OFF`. Bluetooth discovery remains active in Setup Mode exclusively for the controller lookup UI.
- **Web Portal Restricted to Setup Mode & UI Cleanup**: **BREAKING** The HTTP web server and REST API run exclusively during Setup Mode over SoftAP (`http://192.168.4.1`). Normal mode Wi-Fi station connectivity operates strictly as a headless UDP listener for WOL magic packets (ports 7 and 9). Remove unreachable remote power action endpoints (`/api/power/on`, `/api/power/off`, `/api/shutdown`) and their UI buttons (`Power On`, `Force Cut Power`, `Shut Down OS`), keeping the optional motherboard power button (`gracefulShutdown`) configuration toggle.
- **Removal of `bc250-sync` and `bc250-setup`**: **BREAKING** Remove the `bc250-sync` Linux background service, `bc250-setup` host CLI wizard, `docs/host-sync.md`, and the UDP port 25025 discovery responder (`BC250_DISCOVER`). Provisioning and configuration are performed via the web portal (`http://192.168.4.1`) from a phone or external computer while the host board is off.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `power-control`: Prevent host motherboard power-up during Setup Mode, completely disable all wireless radios during `STATE_BOOTING` and `STATE_ON` (restoring wake listeners only in `STATE_OFF`), and remove programmatic power-on, force power-off, and shutdown pulse requirements.
- `controller-wake`: Completely tear down the Bluetooth controller when leaving `STATE_OFF` into `STATE_BOOTING` or `STATE_ON`, and inhibit controller wake during Setup Mode.
- `network-wake`: Restrict Wi-Fi station mode strictly to headless WOL packet listening in `STATE_OFF` (outside Setup Mode), eliminate the 5-second post-boot MAC swap and UDP discovery broadcast responder, and shut down Wi-Fi immediately upon boot.
- `setup-portal`: Run the HTTP web server exclusively in Setup Mode over SoftAP, enforce host power lockout while in Setup Mode, and remove remote power action endpoints (`/api/power/on`, `/api/power/off`, `/api/shutdown`) and UI action buttons while preserving the `gracefulShutdown` setting.
- `host-device-sync`: Decommission and remove the Linux host device synchronization service and background REST API controller sync requirements.

## Impact

- **Firmware (`src/main.cpp`, `src/network_wake.cpp`, `src/portal.cpp`, `src/wake_classic.cpp`, `src/bt_classic.cpp`, `src/wake_ble.cpp`)**:
  - `setup()` branches cleanly between Setup Mode (`portalBegin()` only) and Normal Mode (`normalBegin()`).
  - `setState()` tears down Bluetooth (`wakeStop()`) and Wi-Fi (`networkWakeStop()`) on `STATE_BOOTING`/`STATE_ON`, and reinitializes them on `STATE_OFF`.
  - Remove `requestPowerOn()`, `requestPowerOff()`, `requestGracefulShutdown()`, `/api/power/on`, `/api/power/off`, and `/api/shutdown`.
  - `networkWakeLoop()` deletes MAC swap timer, mDNS server, UDP discovery listener on 25025, fallback SoftAP, and station portal server activation.
- **Web Portal (`app/index.html`)**:
  - Remove ATX power supply action card, Force Cut modal, `Shut Down OS` button, and normal-mode read-only lock notices; update pairing modal and hostname help text.
- **Tooling & Docs**:
  - `tools/bc250-sync/` and `tools/bc250-setup/` directories deleted.
  - `docs/host-sync.md` deleted; `README.md` updated.
