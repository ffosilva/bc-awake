# Tasks

## 1. Fix PS_ON_PIN Output Latch Initialization

- [x] 1.1 Include `<driver/gpio.h>` in `src/main.cpp` and update `normalBegin()` to call `gpio_set_level((gpio_num_t)PS_ON_PIN, PS_ON_RELEASE)` before `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` and `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` immediately after `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)`, verifying the initialization sequence in `src/main.cpp`.

## 2. Build & Specification Verification

- [x] 2.1 Run `pio run -e esp32dev -e esp32cam -e esp32-c3-devkitm-1` and `openspec validate "fix-pson-gpio-latch-init"` to verify all firmware targets compile cleanly and the OpenSpec change passes validation with exit code 0.

## Workflow follow-up

- Flash `esp32cam` over `/dev/cu.usbserial-10` and verify in serial monitor that `__digitalWrite(): IO 14 is not set as GPIO` no longer appears and `PS_ON#` remains released (`STATE_OFF`).
- Sync and archive the change via `/openspec-archive-change`.
