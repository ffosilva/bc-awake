# Tasks

## 1. Decommission Host Tooling and Documentation

- [x] 1.1 Remove `tools/bc250-sync/` and `tools/bc250-setup/` directories and verify all contained files are deleted
- [x] 1.2 Remove `docs/host-sync.md` and update `README.md` to remove `bc250-sync`, `bc250-setup`, remote power action references, and normal-mode `http://bc-awake.local` portal references, verifying no broken links remain

## 2. Simplify Network Wake & Eliminate 5s MAC Swap

- [x] 2.1 Add `networkWakeStop()` in `include/network_wake.h` and `src/network_wake.cpp` to stop UDP listeners, disconnect Wi-Fi, and set `WiFi.mode(WIFI_OFF)`
- [x] 2.2 Delete the 5-second deferred MAC swap timer (`MAC_SWAP_DEFER_MS`, `g_pendingMacSwap`, `applyStateMacChange()`), fallback SoftAP timer, and UDP 25025 discovery responder (`BC250_DISCOVER`) from `src/network_wake.cpp`
- [x] 2.3 Remove `portalServerBegin()` and `MDNS.begin()` from `networkWakeLoop()` in `src/network_wake.cpp`, restricting station mode exclusively to UDP ports 7 and 9 in `STATE_OFF`

## 3. Clean Up Portal Endpoints and Web UI

- [x] 3.1 Remove `/api/power/on`, `/api/power/off`, and `/api/shutdown` endpoints from `src/portal.cpp` and remove `requestPowerOn()`, `requestPowerOff()`, and `requestGracefulShutdown()` from `include/portal.h` and `src/main.cpp`
- [x] 3.2 Update `app/index.html` to remove the ATX Power Supply action card (`Power On` / `Force Cut Power`), the Force Power Off modal, and the `Shut Down OS` button while keeping the `gracefulShutdown` checkbox and Save button in the Power tab
- [x] 3.3 Update `app/index.html` pairing modal text (removing `(Case power button remains active)`) and hostname help text (removing `http://<hostname>.local`), and verify `scripts/build_html.py` regenerates `include/portal_html.h`

## 4. Isolate Setup Mode and Teardown Radios in Running States

- [x] 4.1 Implement `btEnd()` in `include/bt_classic.h` and `src/bt_classic.cpp` and `wakeStop()` in `include/wake.h`, `src/wake_classic.cpp`, and `src/wake_ble.cpp` to completely disable and de-initialize the active Bluetooth controller/stack
- [x] 4.2 Update `setup()` and `normalBegin()` in `src/main.cpp` to evaluate `g_setupMode` first: in Setup Mode, keep `PS_ON#` released in `STATE_OFF`, skip `networkWakeBegin()`, and launch `portalBegin()`; in Normal Mode, only start `startWake()` and `networkWakeBegin(true)` when booting into `STATE_OFF`
- [x] 4.3 Update `setState()` in `src/main.cpp` to call `wakeStop()` and `networkWakeStop()` on transitions to `STATE_BOOTING` or `STATE_ON`, and re-initialize `startWake()` and `networkWakeBegin(true)` on transition back to `STATE_OFF`
- [x] 4.4 Guard `powerOn()`, short button presses, serial `'s'` command (`state == STATE_OFF` only), controller wake, and WOL checks in `src/main.cpp` so the host board can never power on during Setup Mode

## 5. Build and Validation

- [x] 5.1 Build all firmware environments (`pio run -e esp32dev`, `pio run -e esp32cam`, `pio run -e esp32-c3-devkitm-1`) and verify zero compilation errors
- [x] 5.2 Validate OpenSpec change artifacts with `openspec validate isolate-setup-and-radios` and verify clean validation
