# Tasks

## 1. Firmware Pairing Mode Lifecycle & Wake Inhibition

- [x] 1.1 Implement pairing mode state tracking and 60-second timeout in `src/portal.cpp` and expose `portalIsPairingActive()` and `portalPairingStop()` in `include/portal.h`. Verify timeout calculation against `millis()`.
- [x] 1.2 Add authenticated `POST /api/listener/stop` endpoint in `src/portal.cpp` and update `POST /api/listener/start` to arm the 60s timeout. Verify endpoint responses.
- [x] 1.3 Update `src/bt_classic.cpp` to prevent incoming connection requests from updating `wakeLastSeen` or arming wake while pairing is active, and update `src/main.cpp` to inhibit controller and WOL wake triggers while pairing mode is active while keeping the physical case button operational. Verify via code inspection.
- [x] 1.4 Update controller addition in `src/portal.cpp` to deactivate pairing mode and arm the 15-second wake cooldown (`WAKE_WAKE_COOLDOWN_MS`). Verify cooldown assignment.

## 2. ESP32-CAM Hardware Feedback

- [x] 2.1 Implement non-blocking flash burst sequence (3 pulses of 60 ms ON / 60 ms OFF) for `FLASH_LED` (GPIO 4) on ESP32-CAM in `src/main.cpp` and trigger upon pairing listener start. Verify timing logic in `updateFlash()`.

## 3. Setup Portal Web Interface

- [x] 3.1 Update `app/index.html` pairing modal to include the informational banner (explaining board power-on is paused and case button remains active) and a live countdown timer decrementing from 60 seconds.
- [x] 3.2 Add pairing timeout view to `app/index.html` with explicit explanation that normal wake has resumed, along with "Try Again" and "Close" actions.
- [x] 3.3 Wire the modal Cancel button in `app/index.html` to invoke `POST /api/listener/stop`.
- [x] 3.4 Display post-save reassurance notice in `app/index.html` informing the user that controller wake will resume after a 15-second cooldown.
- [x] 3.5 Run `scripts/build_html.py` to compile `app/index.html` into `include/portal_html.h` and verify header generation.

## 4. Compilation & Verification

- [x] 4.1 Compile firmware for `esp32cam`, `esp32dev`, and `esp32-c3-devkitm-1` targets using PlatformIO and verify all targets build without errors or warnings.
