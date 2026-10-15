# Proposal

## Why

Currently, wiring the BC250's `PWR_BTN#` line to the ESP32 (`PWR_BTN_PIN`) is only used while the machine is `STATE_ON` (to mirror button presses for graceful ACPI shutdown or send a portal shutdown pulse). When the machine is `STATE_OFF`, the `PWR_BTN_PIN` line remains a high-impedance floating input, and pressing the BC250's native power button does nothing because the BC250 board has no standby power and cannot turn on the ATX power supply on its own.

Because the BC250 motherboard's `PWR_BTN#` input presents an open circuit (> 2 MΩ) to ground when unpowered, the ESP32 can safely apply an internal pull-up (`INPUT_PULLUP`) on `PWR_BTN_PIN` during `STATE_OFF` without back-powering the board. This allows users to power on the machine by pressing the native power button on the BC250 (or chassis button wired to it), providing a complete, native button experience without extra hardware or wiring.

## What Changes

- When the motherboard power button feature is enabled:
  - In `STATE_OFF`, configure `PWR_BTN_PIN` with an internal pull-up (`INPUT_PULLUP`), pulling the line to 3.3 V.
  - Tapping the native power button while in `STATE_OFF` shorts `PWR_BTN#` to GND; upon release (debounced < 8 s), the ESP32 asserts `PS_ON#` LOW to power on the machine.
  - Holding the native power button for ≥ 8 s while in `STATE_OFF` enters the WiFi setup portal.
  - In `STATE_BOOTING`, `PWR_BTN_PIN` switches back to high-impedance `INPUT` to avoid interfering with the motherboard's power sequencing.
  - In `STATE_ON`, `PWR_BTN_PIN` remains in `INPUT` (switched to `OUTPUT` + `LOW` only for portal shutdown pulses or external button mirroring). Holding the native power button for ≥ 5 s in `STATE_ON` triggers the emergency 5 s force power-off backstop.
  - The native button and any external switch connected to `BUTTON_SENSE` share unified button debouncing and hold logic.
- When the motherboard power button feature is disabled (default):
  - `PWR_BTN_PIN` remains in high-impedance `INPUT` (no pull-up) across all states, preserving 100% backward compatibility.
- In the setup portal UI, update the card description for the motherboard power button toggle to reflect that enabling the feature enables both native power-on when off and clean OS shutdown when on.

## Capabilities

### New Capabilities

_None._

### Modified Capabilities

- `power-control`: adds native power button monitoring via `PWR_BTN_PIN` with internal pull-up during `STATE_OFF` (tap to power on, hold ≥ 8 s for setup portal) and long-press force-off monitoring during `STATE_ON` when the feature is enabled.
- `setup-portal`: updates the description of the motherboard power button feature in the web portal to document bidirectional control (power-on and graceful shutdown).

## Impact

- Code: `src/main.cpp` (unified button debounce, `INPUT_PULLUP` mode management for `PWR_BTN_PIN` in `STATE_OFF`, release to `INPUT` in `STATE_BOOTING` and `STATE_ON`), `app/index.html` (portal descriptive copy update), `include/portal_html.h` (regenerated web assets).
- Hardware / Wiring: No wiring changes; uses the existing single wire between `PWR_BTN#` and `PWR_BTN_PIN` (DevKitC: GPIO27, ESP32-C3: GPIO7, ESP32-CAM: GPIO2).
- Configuration: Reuses existing `gracefulShutdown` config toggle (stored as `graceful` in NVS, respecting 15-char limit) as a unified motherboard power button toggle.
- Compatibility: 100% backward compatible; when disabled, behavior is identical to previous releases.
