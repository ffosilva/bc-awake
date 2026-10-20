# Proposal

## Why

The carrier PCB generator and documentation previously used an inverted ASCII pinout diagram where `5V` and `GND` were incorrectly located at the bottom of the left header, while on the right header `GND` was connected to pins 2 and 7 (which on the physical AI-Thinker ESP32-CAM module are `IO16` / PSRAM and `U0TX`). Grounding `IO16` causes PSRAM failure and boot failure. This change corrects the carrier board schematic, single-layer copper routing, and documentation to strictly match the physical AI-Thinker ESP32-CAM pinout as illustrated in `docs/images/esp32cam-pinout.png`.

## What Changes

- Correct the AI-Thinker ESP32-CAM ASCII diagram and pin reference tables in `docs/pinout.md` to reflect the physical header layout (Left: `5V`, `GND`, `IO12`..`IO4`; Right: `3V3`, `IO16`, `IO0`, `GND`, `VCC`, `U0RX`, `U0TX`, `GND`).
- Update `scripts/generate_carrier_pcb.py` header pad assignments (`J_CAM_L` and `J_CAM_R`) to align with physical pins, leaving `IO16`, `IO12`, `IO4`, `IO0`, `VCC`, and UART lines completely disconnected/floating.
- Re-route single-layer bottom copper (`B.Cu`) tracks in `scripts/generate_carrier_pcb.py` to route `+5VSB` and `GND` to top header pins, conditioning inputs to `IO13` and `IO14`, with 0 track crossings and 0 DRC violations.
- Update `docs/esp32cam-carrier-pcb.md` with the corrected grid layout coordinate map, silkscreen legend, and routing lanes.
- Regenerate all KiCad deliverables: schematics (`.kicad_sch`), PCB layout (`.kicad_pcb`), netlist (`.net`), vector plots (`.svg`, `.pdf`), and BOM CSV (`.csv`).

## Capabilities

### Modified Capabilities

- `hardware-targets`: Update `AI-Thinker ESP32-CAM Prototype Carrier Board` requirement to specify accurate physical socket pin mapping corresponding to the AI-Thinker ESP32-CAM module and leaving `IO16` (PSRAM) isolated.

## Impact

- Affects `docs/pinout.md`, `docs/esp32cam-carrier-pcb.md`, `scripts/generate_carrier_pcb.py`, and `hardware/kicad/esp32cam-carrier/` files.
- Zero impact on firmware logic or PlatformIO source code (`include/board.h` already used correct GPIO definitions: `BOARD_SENSE` = GPIO13, `PS_ON_PIN` = GPIO14, `BUTTON_SENSE` = GPIO15, `PWR_BTN_PIN` = GPIO2).
