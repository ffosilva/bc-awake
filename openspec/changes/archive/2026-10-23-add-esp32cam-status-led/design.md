## Context

The firmware has a single `loop()` that calls `normalLoop()` in both normal and setup mode. Setup mode is the `g_setupMode` flag in `src/main.cpp`; power state is the `state` variable (`STATE_OFF/BOOTING/ON`). `include/board.h` holds per-target pin constants and polarity (for example `PS_ON_ASSERT`). `BOARD_ESP32CAM` is defined by both `esp32cam` and `esp32cam-carrier`. The ESP32-CAM red LED is on GPIO33, active LOW. Nothing in `src/` or `include/` uses GPIO33.

Existing behavior that already matches the requested setup-mode semantics: a tap is ignored while in setup mode, and an 8 s hold while OFF in setup mode clears `forceSetup` and restarts. This change adds no new button logic.

## Goals / Non-Goals

**Goals:**
- Visual power and setup-mode feedback on all ESP32-CAM builds.
- Non-blocking, with no effect on timing-sensitive code (debounce, sense, wake).

**Non-Goals:**
- Patterns for OTA, boot timeout or errors.
- An external LED or any PCB change.
- LEDs on esp32dev or esp32-c3 (the design only leaves room for them).

## Decisions

- **Constants in `board.h`:** define `STATUS_LED_PIN` and `STATUS_LED_ON` (level that lights the LED) inside the `BOARD_ESP32CAM` block, and `STATUS_LED_PIN = -1` elsewhere. All LED code is skipped when the pin is `< 0`, so there are no `#if` guards in the logic. This mirrors `BUTTON_GND = -1`.
- **Pure pattern function:** a small `statusLedUpdate(now, state, setupMode)` called once per `loop()` iteration, near the top of `normalLoop()`. It derives the desired level from `millis()` (setup mode: 500 ms on and 500 ms off; BOOTING: 125 ms on and 125 ms off; ON: solid; OFF: off), and writes the pin only when the level changes. No timers or tasks, which keeps it consistent with the existing style.
- **Priority:** `g_setupMode` wins over `state`.
- **Initialization:** configure the pin as an output and write the "off" level in `initHardware()`, before the radios start. The brief pre-init level at reset is accepted.
- **Why GPIO33:** it is not a strapping pin, is unused here, and needs no PCB change.
- **Alternative rejected:** GPIO4 with an external LED. It shares the pin with the very bright onboard flash LED and needs PCB changes.

## Risks / Trade-offs

- The LED is under or next to the module and may not be visible in an enclosure → accepted. The change is firmware-only, and an external LED could be added later.
- `loop()` stalls (such as `delay(50)` before restart) can interrupt the blink → harmless, because those paths restart the device.
- The LED may blink slightly unevenly if a loop iteration is long (WiFi or BT work) → acceptable for a status indicator.
- Fast blink during BOOTING lasts at most `BOOT_TIMEOUT_MS` (10 s), after which the state returns to OFF and the LED goes dark.
