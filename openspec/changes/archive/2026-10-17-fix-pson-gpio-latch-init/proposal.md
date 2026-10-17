# Proposal

## Why

In `v1.13.3`, `normalBegin()` was updated to call `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` before `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` without calling `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` afterward. On Arduino-ESP32 v3.x (`esp32-hal-gpio.c`), calling `digitalWrite()` before `pinMode()` is rejected by the peripheral manager (`__digitalWrite(): IO 14 is not set as GPIO`) and returns early without writing the hardware output latch, leaving `PS_ON_PIN` at its reset-default `LOW` (`PS_ON_ASSERT`) state once `pinMode(..., OUTPUT_OPEN_DRAIN)` is enabled and causing the ATX PSU to immediately turn the BC250 board ON when AC power is connected.

## What Changes

- Pre-load the hardware GPIO output register HIGH before enabling the open-drain driver using ESP-IDF's `gpio_set_level((gpio_num_t)PS_ON_PIN, PS_ON_RELEASE)` (`driver/gpio.h`), which writes the hardware latch directly without requiring prior Arduino peripheral manager registration.
- Call `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` immediately after `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` so the Arduino-ESP32 GPIO HAL state is also synchronized to `PS_ON_RELEASE` (`HIGH` / high-Z) without logging a peripheral-manager error.

## Capabilities

### New Capabilities

### Modified Capabilities
- `power-control`: Refine `Power State Management` startup `PS_ON_PIN` initialization requirements so the hardware output latch is pre-loaded HIGH before enabling open-drain mode and synchronized via `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` after `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)`, ensuring `PS_ON#` remains released (high-Z) in `STATE_OFF`.

## Impact

- `src/main.cpp`: Include `<driver/gpio.h>` and update `PS_ON_PIN` initialization in `normalBegin()`.
- `openspec/specs/power-control/spec.md`: Clarify startup `PS_ON_PIN` initialization behavior.
