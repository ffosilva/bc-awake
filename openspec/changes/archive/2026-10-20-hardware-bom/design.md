# Design

## Context

See `proposal.md` for motivation. For builders deploying the AI-Thinker ESP32-CAM target and its single-layer carrier board, an exhaustive Bill of Materials (BOM) dedicated strictly to the ESP32-CAM hardware setup is needed in both a rich human-readable markdown guide and an automated machine-readable CSV format.

## Goals / Non-Goals

**Goals:**
- Create an authoritative, comprehensive BOM in `docs/bom.md` dedicated to the ESP32-CAM setup, organized into three distinct sections:
  1. **ESP32-CAM Prototype Carrier Board**: All passives (`C_PWR1`..`C_PWR4`, `R1`..`R4`, `C1`, `C2`), active semiconductors (`Q1` 2N7000), socket headers (`J_CAM_L`, `J_CAM_R`), and terminal block (`J_EXT`).
  2. **Interconnect Harness & Hardware**: ATX 24-pin tap leads (+5VSB, GND, PS_ON#), TPMS1 pin 9 sense lead, momentary front-panel switch, and 20–24 AWG wiring.
  3. **ESP32-CAM Module & Programming Tools**: AI-Thinker ESP32-CAM board, USB-UART serial programmer (CP2102/FT232R) or ESP32-CAM-MB daughterboard, and flashing jumper.
- Provide detailed component specifications: footprint pitch, physical dimensions, voltage/power ratings, tolerances, and vendor search keywords (LCSC, DigiKey, Mouser, AliExpress).
- Automate CSV generation in `scripts/generate_carrier_pcb.py` producing `hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv` directly from schematic definitions.
- Update project documentation (`docs/pinout.md`, `docs/esp32cam-carrier-pcb.md`) with navigation links to the new BOM.

**Non-Goals:**
- Non-ESP32-CAM target BOMs (DevKitC, ESP32-C3 are excluded to maintain dedicated focus on the ESP32-CAM build).
- Exclusive vendor lock-in (provide generic parametric ratings alongside reference part numbers).
- Surface-mount (SMD) component BOMs (focus remains strictly on Through-Hole Technology / THT for protoboard and carrier replication).

## Decisions

### 1. Document Structure and Location
- **Decision**: Place the centralized BOM at `docs/bom.md`.
- **Rationale**: Keeps documentation alongside `docs/pinout.md` and `docs/esp32cam-carrier-pcb.md`, providing a natural progression from wiring and schematic to procurement.
- **Alternatives considered**: Embedding tables directly inside `README.md` (rejected to prevent bloating the top-level repository overview).

### 2. Machine-Readable CSV Schema
- **Decision**: Export `esp32cam-carrier-bom.csv` with standard columns:
  `Item,Reference,Quantity,Value,Footprint,Description,Ratings / Specs,Reference MPN`
- **Rationale**: Compatible with standard spreadsheet software (Excel, LibreOffice) and distributor BOM upload tools (LCSC, JLCPCB, Mouser).
- **Alternatives considered**: Raw KiCad `.net` netlist (not directly consumable by typical BOM uploaders).

### 3. Automated BOM Generation via Generator Script
- **Decision**: Extend `scripts/generate_carrier_pcb.py` to aggregate components from the schematic symbol list and output `esp32cam-carrier-bom.csv`.
- **Rationale**: Ensures that any schematic component or value modification automatically updates the CSV BOM during build/regeneration.

## Risks / Trade-offs

- **[Component sourcing availability / regional stock variations]** → Specify generic parametric tolerances (e.g. 1/4W 1% metal film, 16V 105°C low-ESR electrolytic) so builders can substitute equivalent parts from any local supplier.
- **[TO-92 2N7000 pinout variations]** → Explicitly specify standard D-G-S / S-G-D pinouts and formed 2.54 mm lead spacing in both the table and footnotes.
