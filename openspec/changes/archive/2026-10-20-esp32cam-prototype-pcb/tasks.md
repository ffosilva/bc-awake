# Tasks

## 1. KiCad Schematic Design

- [x] 1.1 Create the KiCad project structure in `hardware/kicad/esp32cam-carrier/` (`esp32cam-carrier.kicad_pro` and `esp32cam-carrier.kicad_sch`). Verify KiCad project files parse without syntax errors.
- [x] 1.2 Complete the schematic wiring for ESP32-CAM female sockets (`J_CAM_L`, `J_CAM_R`), dual-rail power decoupling, TPMS1 conditioning filter, 2N7000 MOSFET buffer, and external terminal block `J_EXT`. Verify netlist and pin mappings.

## 2. KiCad Single-Layer PCB Layout

- [x] 2.1 Place all THT footprints on a strict 2.54 mm (100 mil) grid matching standard protoboard pitch with 22.86 mm (0.9 in) header row spacing. Verify footprint pinout and package clearances.
- [x] 2.2 Route all traces on a single copper layer (`B.Cu`) with ≥ 0.8 mm signal width and 1.2 mm power traces, and format the silkscreen layer (`F.SilkS`) showing component outlines, values, and reference designators. Verify design rule checks (DRC).

## 3. Documentation & Verification

- [x] 3.1 Create `docs/esp32cam-carrier-pcb.md` with protoboard pad diagrams, soldering steps, and cross-reference from `docs/pinout.md`. Verify documentation links and markdown rendering.
- [x] 3.2 Run `openspec validate` across all change artifacts to ensure full specification and task consistency.
