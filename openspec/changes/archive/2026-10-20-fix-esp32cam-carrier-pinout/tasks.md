# Tasks

## 1. Pinout & Carrier PCB Generator Realignment

- [x] 1.1 Correct the AI-Thinker ESP32-CAM ASCII pinout diagram and pin reference tables in `docs/pinout.md` to match `docs/images/esp32cam-pinout.png`. Verify markdown rendering and pin rows.
- [x] 1.2 Update `scripts/generate_carrier_pcb.py` header pad assignments and net mappings for `J_CAM_L` and `J_CAM_R` according to physical pinout, leaving `IO16` floating. Verify netlist output.
- [x] 1.3 Re-route single-layer bottom copper (`B.Cu`) traces in `scripts/generate_carrier_pcb.py` to connect power and signals without crossing tracks. Verify 0 DRC violations.

## 2. Documentation & Deliverables Verification

- [x] 2.1 Update `docs/esp32cam-carrier-pcb.md` with the corrected grid layout coordinate map, pad legend, and routing lanes. Verify text matches schematic.
- [x] 2.2 Regenerate all KiCad deliverables (schematics, PCB, netlist, SVGs, PDF, BOM CSV) and verify 0 DRC violations with KiCad CLI.
- [x] 2.3 Run `openspec validate --all` to verify specification and change consistency.
