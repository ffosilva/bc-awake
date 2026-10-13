# Tasks

## 1. Configuration Storage

- [x] 1.1 Add `wolEnabled`, `wifiSsid`, `wifiPassword`, and `bc250Mac` fields and setter functions to `include/config.h` and verify header compiles
- [x] 1.2 Implement NVS persistence and loading for network wake keys in `src/config.cpp` and verify `loadConfig()` initializes default values correctly

## 2. Network Wake Core Driver

- [x] 2.1 Define network wake interface in `include/network_wake.h` declaring initialization, loop polling, and magic packet detection functions
- [x] 2.2 Implement `src/network_wake.cpp` with non-blocking Wi-Fi station connectivity, open/passwordless network support, and auto-reconnect handling
- [x] 2.3 Implement UDP listener on port 9 and port 7 with 102-byte magic packet parser matching `config.bc250Mac`
- [x] 2.4 Verify network wake module compilation across all build environments (`esp32dev`, `esp32cam`, and `esp32-c3-devkitm-1`)

## 3. Setup Portal API & Web Interface

- [x] 3.1 Update `/api/status` endpoint in `src/portal.cpp` to expose `wolEnabled`, `wifiSsid`, `hasWifiPassword`, and `bc250Mac`
- [x] 3.2 Implement authenticated `/api/network` POST endpoint in `src/portal.cpp` with MAC format validation and persistence
- [x] 3.3 Add Wake-on-LAN configuration section with enable checkbox, SSID, optional password, and target MAC inputs to `app/index.html`
- [x] 3.4 Wire portal JavaScript to load network status from `/api/status` and save updates to `/api/network` with in-page feedback

## 4. Main Loop Integration & Multi-Target Build Verification

- [x] 4.1 Integrate `networkWakeBegin()` and `networkWakeLoop()` into `src/main.cpp` and trigger `powerOn` upon valid magic packet while in `STATE_OFF`
- [x] 4.2 Add Wi-Fi and network wake status to the periodic serial heartbeat log in `src/main.cpp`
- [x] 4.3 Update `README.md` to document the new Wake-on-LAN feature and portal configuration instructions
- [x] 4.4 Run full PlatformIO compilation for all targets (`pio run -e esp32dev`, `pio run -e esp32cam`, `pio run -e esp32-c3-devkitm-1`) and verify zero build errors

## Workflow follow-up

- Archive the change after implementation and testing are complete.
- Verify the archived capability specifications in `openspec/specs/`.
