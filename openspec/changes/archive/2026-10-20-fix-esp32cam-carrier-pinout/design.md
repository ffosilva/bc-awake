# Design

## Context

See `proposal.md` for motivation. The AI-Thinker ESP32-CAM module has a specific physical pinout where:
- Left header (antenna at top): 1: 5V, 2: GND, 3: IO12, 4: IO13, 5: IO15, 6: IO14, 7: IO2, 8: IO4.
- Right header (antenna at top): 1: 3V3, 2: IO16, 3: IO0, 4: GND, 5: VCC, 6: U0RX, 7: U0TX, 8: GND.

The previous carrier PCB generator and documentation mapped these pins incorrectly based on an inverted ASCII chart. This design realigns the carrier board layout, schematic nets, and documentation.

## Goals / Non-Goals

**Goals:**
- Correct the socket pad-to-net mapping in `scripts/generate_carrier_pcb.py` to match the physical AI-Thinker ESP32-CAM module pinout.
- Maintain a strict single bottom copper layer (`B.Cu`) routing with 0 crossing tracks and 0 top jumpers on standard 2.54 mm perfboard grid.
- Keep `IO16` (PSRAM), `IO12` (MTDI strap), `IO0` (GPIO0 strap), `IO4` (flashlight), `VCC`, and UART lines floating / ungrounded.
- Verify with KiCad CLI that the board passes DRC with 0 errors and netlist exports cleanly.
- Synchronize all ASCII diagrams and pin tables in `docs/pinout.md` and `docs/esp32cam-carrier-pcb.md`.

**Non-Goals:**
- Changing firmware GPIO configurations (firmware in `include/board.h` already matches the ESP32 GPIO numbers).
- Multi-layer routing (the board must remain 100% single copper layer for perfboard hand-soldering).

## Decisions

### 1. Carrier Socket Pin Assignments

```
J_CAM_L (Left Socket, Col 13):
  Row 2: Pin 1 -> +5VSB (Power inlet)
  Row 3: Pin 2 -> GND (Common ground)
  Row 4: Pin 3 -> IO12 (NC)
  Row 5: Pin 4 -> NET_IO13 (BOARD_SENSE, from TPMS1 divider)
  Row 6: Pin 5 -> BTN_A (BUTTON_SENSE, from front-panel switch)
  Row 7: Pin 6 -> NET_IO14 (PS_ON_PIN, to transistor gate series resistor R3)
  Row 8: Pin 7 -> PWR_BTN# (PWR_BTN_PIN, optional shutdown mirror)
  Row 9: Pin 8 -> IO4 (Flash LED - NC)

J_CAM_R (Right Socket, Col 22):
  Row 2: Pin 1 -> +3V3 (To 3.3V decoupling C_PWR3 / C_PWR4)
  Row 3: Pin 2 -> IO16 (NC - PSRAM bus, isolated)
  Row 4: Pin 3 -> IO0 (NC - Boot strap)
  Row 5: Pin 4 -> GND (Ground reference)
  Row 6: Pin 5 -> VCC (NC - 3.3V/5V selector)
  Row 7: Pin 6 -> U0RX (NC - Serial receive)
  Row 8: Pin 7 -> U0TX (NC - Serial transmit)
  Row 9: Pin 8 -> GND (Ground reference)
```

### 2. Single-Layer Routing Architecture

To keep all traces on the single bottom copper layer (`B.Cu`) with zero crossings:
- `+5VSB` power track runs from `J_EXT` pin 6 (Row 9) along the bottom edge, through `C_PWR1` and `C_PWR2`, then up Col 11 to `J_CAM_L` pin 1 (Row 2).
- `GND` runs along Row 1 (top bus) and Row 8 (bottom bus), tying `J_EXT` pin 5, `Q1` source/emitter, passive ground pads, `J_CAM_L` pin 2, and `J_CAM_R` pin 4 / pin 8.
- `TPMS1` conditioning (`R1`, `R2`, `C1`) outputs `NET_IO13` which routes to `J_CAM_L` pin 4 (Row 5).
- `BTN_A` routes straight from `J_EXT` pin 2 to `J_CAM_L` pin 5 (Row 6).
- `Q1` buffer gate network connects `NET_IO14` from `J_CAM_L` pin 6 (Row 7).
- `PWR_BTN#` routes from `J_EXT` pin 3 to `J_CAM_L` pin 7 (Row 8).

## Risks / Trade-offs

- **[Track Clearance on Single Layer]** → Maintain at least 0.5 mm clearance between tracks and pads on standard 2.54 mm perfboard pitch to prevent solder bridges.
