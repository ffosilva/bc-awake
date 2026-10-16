# Tasks

## 1. Web Portal & mDNS in Normal Mode

- [x] 1.1 Integrate mDNS responder (`ESPmDNS.h`) in `src/network_wake.cpp` to advertise `bc-awake.local` upon Wi-Fi Station connection and verify mDNS startup in build and logs.
- [x] 1.2 Refactor web server initialization so `portal.cpp` endpoints run persistently over Wi-Fi Station mode in `normalBegin()`, verifying HTTP responses on port 80.
- [x] 1.3 Add `boardState` string field (`"OFF"`, `"BOOTING"`, `"ON"`) to `/api/status` in `src/portal.cpp` and verify payload matches active power controller state.

## 2. Dynamic Wi-Fi MAC Address Switching

- [x] 2.1 Implement dynamic Wi-Fi Station MAC switching in `src/network_wake.cpp`: apply configured `bc250Mac` in `STATE_OFF` and restore factory default ESP32 MAC in `STATE_BOOTING` and `STATE_ON`. Verify MAC switching calls in build.
- [x] 2.2 Wire power state transition hooks in `src/main.cpp` to trigger Wi-Fi disconnect, MAC address swap, and Station reconnection on `STATE_OFF` <-> `STATE_BOOTING`/`STATE_ON` transitions. Verify non-blocking reconnection behavior.

## 3. Power-State Web UI & API Guardrails

- [x] 3.1 Update `/api/listener/start` and `/api/devices` in `src/portal.cpp` to reject sensing requests with HTTP 409 Conflict when host power state is `STATE_BOOTING` or `STATE_ON`. Verify endpoint status codes.
- [x] 3.2 Update `/api/network` in `src/portal.cpp` to reject configuration updates with HTTP 403 Forbidden when host power state is `STATE_BOOTING` or `STATE_ON`. Verify endpoint status codes.
- [x] 3.3 Update `app/index.html` UI logic: add a green/red board power status light indicator in the header ("POWERED ON" / "POWERED OFF"), disable Wi-Fi configuration form fields and hide the save button when `boardState` is `ON`, disable the interactive pairing listener button when `boardState` is `ON`, and run `python scripts/build_html.py` to regenerate `include/portal_html.h`.

## 4. Linux Host Device Sync Tooling

- [x] 4.1 Create `tools/bc250-sync/bc250-sync.py` using Python 3 standard library to discover local BlueZ host adapter MAC and paired gamepads, authenticate with `/api/login`, and synchronize controllers via `/api/host`, `/api/controllers/add`, and `/api/controllers/remove`.
- [x] 4.2 Create systemd service and timer unit templates (`tools/bc250-sync/bc250-sync.service`, `tools/bc250-sync/bc250-sync.timer`) and sample configuration file (`tools/bc250-sync/bc250-sync.conf`).
- [x] 4.3 Add documentation in `docs/host-sync.md` explaining installation and operation on Bazzite, SteamOS, Ubuntu, and Arch Linux.

## 5. Integration Verification

- [x] 5.1 Run PlatformIO build across all hardware targets (`esp32`, `esp32cam`, `esp32c3`) to verify zero compilation errors or flash overflow.
- [x] 5.2 Validate OpenSpec change integrity with `openspec validate --change persistent-portal-and-device-sync`.
