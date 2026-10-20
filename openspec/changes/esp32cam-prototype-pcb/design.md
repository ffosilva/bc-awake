# Design

## Context

See `proposal.md` for motivation. To build the ESP32-CAM resilience circuit on dot-matrix perfboard / protoboard (standard 2.54 mm hole pitch), a reference KiCad 8 project will be created providing a single-layer PCB layout (`B.Cu`) and component placement silkscreen (`F.SilkS`).

The AI-Thinker ESP32-CAM module has two 8-pin rows:
- Left header: `3V3`, `GND`, `IO0`, `U0TX`, `U0RX`, `IO16`, `3V3/VCC`, `5V`
- Right header: `IO12`, `IO13`, `IO15`, `IO14`, `IO2`, `IO4`, `GND`, `5V`
- Header spacing: 15.24 mm (0.6 in / 6 grid units of 2.54 mm).

## Goals / Non-Goals

**Goals:**
- Provide complete KiCad 8 project files (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`).
- Use two 1x8 2.54 mm female socket headers (`J_CAM_L` and `J_CAM_R`) to plug in the ESP32-CAM securely.
- Include all circuit passives using standard Through-Hole Technology (THT) footprints:
  - `C_PWR1`: 470 µF / 16V radial electrolytic capacitor (5.0 mm lead pitch, 8–10 mm diameter).
  - `C_PWR2`: 100 nF 25V ceramic capacitor (2.54 mm / 5.0 mm lead pitch).
  - `C_PWR3`: 47–100 µF / 16V radial electrolytic/tantalum capacitor (2.54 mm lead pitch).
  - `C_PWR4`: 100 nF 25V ceramic capacitor.
  - `R1`: 1 kΩ 1/4W axial resistor.
  - `R2`: 4.7 kΩ 1/4W axial resistor.
  - `C1`: 100 nF ceramic capacitor.
  - `Q1`: 2N7000 N-channel MOSFET (TO-92 package).
  - `R3`: 1 kΩ 1/4W axial resistor.
  - `R4`: 100 kΩ 1/4W axial resistor.
  - `C2`: 100 nF ceramic capacitor.
  - `J_EXT`: 1x6 2.54 mm header or screw terminal block for external wiring (`+5VSB`, `GND`, `PS_ON#`, `TPMS1`, `BTN_A`, `PWR_BTN#`).
- Ensure all tracks are on a single copper layer (`B.Cu`) with pad centers locked to a 2.54 mm grid for effortless protoboard replication.
- Provide a clean component silkscreen layer (`F.SilkS`) showing component shapes, reference designators, and values.

**Non-Goals:**
- Surface mount (SMD) components.
- Multi-layer routing (strictly single-layer with at most 1 standard top jumper wire if crossing is required).
- Custom PCB manufacturing order (the design is targeted at manual protoboard soldering, though Gerber generation will be supported).

## Decisions

### 1. Grid Alignment and Single-Layer Routing
- **Decision**: All component pads and track segments are snapped to a 2.54 mm (100 mil) grid. Traces are routed exclusively on `B.Cu` with minimum 0.8 mm (32 mil) width, and power traces (`+5VSB`, `GND`) routed at 1.2–1.5 mm (47–59 mil).
- **Rationale**: Protoboard / stripboard consists of pads spaced at 2.54 mm. Placing all traces on `B.Cu` mimics the bottom soldered tracks of a hand-wired prototype board.
- **Alternatives considered**: Freeform multi-angle routing (rejected because it cannot be directly copied onto protoboard hole grids).

### 2. Component Layout Around ESP32-CAM Socket
- **Decision**:
  - Place `J_CAM_L` and `J_CAM_R` in the center of the board.
  - Place power decoupling capacitors (`C_PWR1`, `C_PWR2`) immediately adjacent to the `5V` and `GND` pins on `J_CAM_L` (bottom pins).
  - Place TPMS1 conditioning components (`R1`, `R2`, `C1`) adjacent to `IO13` on `J_CAM_R`.
  - Place the MOSFET buffer (`Q1`, `R3`, `R4`, `C2`) adjacent to `IO14` on `J_CAM_R`.
  - Place external connector `J_EXT` along the board edge.
- **Rationale**: Keeps trace lengths minimal, ensures low-inductance power decoupling, and prevents routing crossings.

### 3. KiCad 8 File Formats
- **Decision**: Generate standard S-expression formatted `.kicad_sch` and `.kicad_pcb` files directly compatible with KiCad 8.x.
- **Rationale**: S-expression format is text-based, version-control friendly, and directly readable by modern KiCad installations.

## Risks / Trade-offs

- **[Protoboard point-to-point wiring differs slightly from copper etched board]** -> Include an ASCII wiring and pad diagram in `docs/esp32cam-carrier-pcb.md` alongside the KiCad project files to allow visual cross-referencing while soldering.
- **[2N7000 TO-92 pinout variations]** -> Standard 2N7000 TO-92 pinout is Pin 1 = Source, Pin 2 = Gate, Pin 3 = Drain (looking at flat face). Explicitly label S, G, D on the silkscreen and schematic.
