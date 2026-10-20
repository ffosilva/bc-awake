# Tasks

## 1. Automated Machine-Readable BOM Export

- [x] 1.1 Update `scripts/generate_carrier_pcb.py` to aggregate components from schematic symbols and generate `hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv`. Verify CSV row count, column structure, and component mappings.

## 2. Centralized Hardware Bill of Materials Documentation

- [x] 2.1 Create `docs/bom.md` providing itemized Bill of Materials tables dedicated specifically to the AI-Thinker ESP32-CAM target (carrier passives, ATX & motherboard wiring harness parts, module & flashing tools) with tolerances, ratings, and sourcing notes. Verify markdown structure and table rendering.
- [x] 2.2 Add cross-references to `docs/bom.md` from `docs/pinout.md` and `docs/esp32cam-carrier-pcb.md`. Verify document cross-links.

## 3. Verification & OpenSpec Validation

- [x] 3.1 Execute `python3 scripts/generate_carrier_pcb.py` and verify `esp32cam-carrier-bom.csv` matches KiCad netlist components and DRC passes with 0 violations.
- [x] 3.2 Run `openspec validate --all` to verify change and specification consistency.
