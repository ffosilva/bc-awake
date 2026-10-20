# Tasks

## 1. Documentation & Circuit Schematics

- [x] 1.1 Update `docs/pinout.md` with dedicated ESP32-CAM resilience schematics covering dual-rail decoupling (5V and 3.3V), GPIO 13 pull-down/RC low-pass filter, and GPIO 14 MOSFET isolation buffer. Verify ASCII schematics render correctly.
- [x] 1.2 Document the Bill of Materials (BOM) and component rating recommendations in `docs/pinout.md`. Verify component values match `design.md`.
- [x] 1.3 Update `README.md` hardware disclaimer and wiring sections to cross-reference the ESP32-CAM resilience circuit in `docs/pinout.md`. Verify link targets exist and load.

## 2. Specification Verification

- [x] 2.1 Run `openspec validate` across all change artifacts to verify spec delta formatting and scenario consistency.
