# Design

## Context

See `proposal.md` - Why. In `framework-arduinoespressif32 @ 3.1.3` (ESP-IDF 5.3), `esp32-hal-gpio.c`'s `__digitalWrite(uint8_t pin, uint8_t val)` checks whether `pin` has already been assigned to the `ESP32_BUS_TYPE_GPIO` bus in the peripheral manager (`perimanGetPinBus`). Because `pinMode(PS_ON_PIN, ...)` has not yet run on startup, calling `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` before `pinMode()` logs `[E][esp32-hal-gpio.c:176] __digitalWrite(): IO 14 is not set as GPIO.` and returns without writing `GPIO.out_w1ts`. When `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` then enables the open-drain output driver, the hardware output latch is still at its power-on reset state (`0` = `LOW` = `PS_ON_ASSERT`), holding `PS_ON#` LOW in `STATE_OFF`.

## Goals / Non-Goals

**Goals:**
- Pre-load the hardware GPIO output latch HIGH (`PS_ON_RELEASE`) before enabling the open-drain driver so `PS_ON#` never glitches LOW during `pinMode()`.
- Keep Arduino-ESP32's peripheral manager and GPIO HAL state consistent without emitting `[E][esp32-hal-gpio.c:176] __digitalWrite(): IO 14 is not set as GPIO.`.
- Ensure `PS_ON#` stays released (high-Z) whenever the controller boots into `STATE_OFF`.

**Non-Goals:**
- Changing `PS_ON_PIN` assignments or external wiring on any board target.

## Decisions

### 1. Pre-load Hardware Latch via `gpio_set_level()` Before `pinMode()` and Call `digitalWrite()` After `pinMode()`
- **Decision**: Include `<driver/gpio.h>` in `src/main.cpp` and initialize `PS_ON_PIN` in `normalBegin()` using:
  ```cpp
  gpio_set_level((gpio_num_t)PS_ON_PIN, PS_ON_RELEASE);
  pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(PS_ON_PIN, PS_ON_RELEASE);
  ```
- **Rationale**:
  1. `gpio_set_level()` is an ESP-IDF HAL call that writes the hardware `GPIO.out_w1ts` register directly without checking Arduino's `periman` bus state, ensuring the output latch is already `1` (`HIGH` / open-drain high-Z) before `pinMode()` enables the pad driver.
  2. Calling `digitalWrite(PS_ON_PIN, PS_ON_RELEASE)` immediately after `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` succeeds without error because `pinMode()` registers the pin with `periman`, guaranteeing that both the ESP-IDF hardware latch and Arduino HAL state agree on `PS_ON_RELEASE`.
- **Alternatives Considered**:
  - *Only calling `digitalWrite()` after `pinMode()`*: Leaves a microsecond window between `pinMode()` enabling the open-drain driver and `digitalWrite()` setting the latch HIGH where the default `0` latch could briefly pull `PS_ON#` LOW. Combining `gpio_set_level()` before `pinMode()` with `digitalWrite()` after `pinMode()` eliminates both the microsecond glitch and the Arduino v3.x `periman` rejection.

## Risks / Trade-offs

- **[Risk] Compatibility across ESP32 architectures (`ESP32` and `ESP32-C3`)** → `driver/gpio.h` and `gpio_set_level((gpio_num_t)pin, level)` are standard ESP-IDF APIs supported identically on both Xtensa ESP32 (`esp32dev`, `esp32cam`) and RISC-V ESP32-C3 (`esp32-c3-devkitm-1`).
