# Proposal

## Why

Today the only way to turn the BC250 off from the ESP32 is to cut PSU power via `PS_ON#`, which is a hard power-off. The board's front-panel power button (`PWR_BTN#`) can be shorted to ground by software, which makes the ESP32 behave like a real power button: a short press triggers an ACPI shutdown and a long press triggers the motherboard's own override. The existing TPMS1 board-sense logic already releases `PS_ON#` once the board is down, so the missing piece is the "press the button" output.

## What Changes

- Add an optional `PWR_BTN_PIN` output wired directly to the motherboard `PWR_BTN#` (idles ~3.4 V, pulled up by the board). The ESP32 never drives it high: it idles as a high-impedance input and pulls LOW only to emulate a button press.
- Pin per target: ESP32-CAM GPIO2, ESP32-DevKitC GPIO27, ESP32-C3 GPIO7.
- Add a persisted, default-off "graceful shutdown" setting. When disabled, the pin is never configured as an output and behavior is unchanged.
- When enabled:
  - While the system is ON, the physical button is **mirrored** to `PWR_BTN_PIN`: LOW while pressed, released when the button is released. A short press is an ACPI event; a long press lets the motherboard apply its own override. The ESP32 does not interpret the press itself.
  - Presses that begin while OFF or BOOTING are not mirrored; power-on and the 8 s setup hold are unchanged.
  - A portal/API "Shut down" action sends a 300 ms pulse on `PWR_BTN_PIN`.
  - The existing 5 s hold-to-cut-PSU remains as a backstop: it releases `PS_ON#` immediately, whether or not the motherboard override worked.
  - Completion relies on the existing TPMS1-LOW detection; there is no automatic hard-cut timeout if the OS hangs.
- Add the enable toggle and a "Shut down" action to the portal System tab, shown as an optional feature.

## Capabilities

### New Capabilities

_None._

### Modified Capabilities

- `power-control`: adds power-button mirroring while ON, the 300 ms programmatic shutdown pulse, and completion via existing board-following detection, with the 5 s force-off kept as backstop.
- `hardware-targets`: defines `PWR_BTN_PIN` for the ESP32-CAM (GPIO2), DevKitC (GPIO27), and ESP32-C3 (GPIO7).
- `configuration-storage`: persists the new graceful shutdown enable flag, loaded at boot and reset to disabled on factory reset.
- `setup-portal`: adds the optional graceful shutdown toggle and "Shut down" action in the System tab, plus the supporting authenticated endpoints and status reporting.

## Impact

- Code: `include/board.h` (new pin constant per target), `include/config.h` and `src/config.cpp` (new flag), `src/main.cpp` (mirroring, pulse), `src/portal.cpp` and `app/index.html` / `include/portal_html.h` (toggle, shutdown action, status field).
- Docs: `docs/pinout.md` and `README.md` (new pin rows, wiring diagram, ESP32-CAM GPIO2 UART-flashing caveat).
- Hardware: optional single wire from `PWR_BTN#` to the new pin. Voltage verified ~3.4 V on the target board; no extra components. Whether this board has a hardware long-press override is unverified and should be tested.
- Out of scope: automatic hard-cut timeout, powering on via `PWR_BTN#`, MOSFET buffering, and changing the 5 s force-off or 8 s setup-hold thresholds.
