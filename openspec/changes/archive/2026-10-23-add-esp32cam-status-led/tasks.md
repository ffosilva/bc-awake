## 1. Board configuration

- [x] 1.1 In `include/board.h`, add `STATUS_LED_PIN = 33` and `STATUS_LED_ON = LOW` to the `BOARD_ESP32CAM` block (shared by esp32cam and esp32cam-carrier), with a comment about the onboard red LED
- [x] 1.2 Add `STATUS_LED_PIN = -1` defaults for the esp32dev and esp32-c3 blocks

## 2. Firmware

- [x] 2.1 In `src/main.cpp` `initHardware()`, configure `STATUS_LED_PIN` as an output driven to the off level when the pin is `>= 0`
- [x] 2.2 Add the pattern function (setup mode 1 Hz, BOOTING about 4 Hz, ON solid, OFF off), `millis()`-based and writing only on level change; setup mode takes priority over state
- [x] 2.3 Call it every iteration of `normalLoop()`; verify the tap-ignored and 8 s exit behaviors in setup mode are unchanged

## 3. Docs

- [x] 3.1 Add the GPIO33 status LED entry and the LED pattern table to `docs/pinout.md`; mention the patterns in the README if setup mode is described there

## 4. Verification

- [x] 4.1 Build `esp32cam`, `esp32cam-carrier`, `esp32dev` and `esp32-c3-devkitm-1`
- [ ] 4.2 On hardware (esp32cam and carrier): LED off in OFF, fast blink while BOOTING, solid in ON, 1 Hz blink in setup mode, tap in setup mode does not power on, 8 s hold returns to normal with LED off
