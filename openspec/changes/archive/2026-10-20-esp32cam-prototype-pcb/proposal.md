# Proposal

## Why

Hand-wiring the ESP32-CAM resilience circuit (bulk decoupling, input pull-down/filter, and MOSFET buffer) on loose jumper wires is error-prone, fragile, and difficult to troubleshoot. To enable reliable and clean replication on standard perfboard/protoboard with solder pads, a modular KiCad single-layer carrier board design is needed with female socket headers for the ESP32-CAM and clear silkscreen component placement.

## What Changes

- Create a KiCad 8.x schematic (`hardware/kicad/esp32cam-carrier/esp32cam-carrier.kicad_sch`) integrating:
  - 2x 1x8 2.54mm female socket headers for the ESP32-CAM module.
  - Power rail bulk decoupling (470 µF electrolytic + 100 nF ceramic on 5V; optional 47–100 µF + 100 nF on 3.3V).
  - TPMS1 input conditioning (1 kΩ series resistor, 4.7 kΩ pull-down, 100 nF ceramic filter).
  - PS_ON# low-side N-channel MOSFET buffer (2N7000 TO-92, 1 kΩ series gate resistor, 100 kΩ pull-down, 100 nF gate snubber).
  - External connection header/terminal block for ATX +5VSB, GND, PS_ON#, TPMS1 pin 9, power button switch, and optional motherboard PWR_BTN#.
- Create a single-layer KiCad PCB layout (`hardware/kicad/esp32cam-carrier/esp32cam-carrier.kicad_pcb`):
  - Single copper layer (bottom copper `B.Cu` or top copper `F.Cu`) designed on a 2.54mm (0.1 in) grid to allow direct 1:1 replication on standard prototype dot-matrix / perfboards.
  - Component silkscreen placement layer (`F.SilkS`) showing exact component outlines, values, and pin 1 indicators.
- Provide assembly documentation and protoboard wiring diagram in `docs/esp32cam-carrier-pcb.md`.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `hardware-targets`: Add reference single-layer prototype carrier PCB specification for the AI-Thinker ESP32-CAM module.

## Impact

- **Hardware artifacts**: New KiCad project in `hardware/kicad/esp32cam-carrier/`.
- **Documentation**: New wiring and assembly guide in `docs/esp32cam-carrier-pcb.md`, referenced from `docs/pinout.md`.
- **Firmware**: No firmware changes; 100% compatible with existing `BOARD_ESP32CAM` target build.
