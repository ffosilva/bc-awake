# Spec Delta

## ADDED Requirements

### Requirement: AI-Thinker ESP32-CAM Bill of Materials
The repository SHALL provide a detailed Bill of Materials (BOM) in documentation and machine-readable format specifying component packages, electrical ratings, tolerances, and wiring harness components dedicated specifically to the AI-Thinker ESP32-CAM module and prototype carrier board.

#### Scenario: Documentation BOM reference
- **WHEN** consulting the ESP32-CAM Bill of Materials documentation
- **THEN** the documentation provides an itemized list of all carrier PCB passives, headers, ATX harness leads, switches, and the ESP32-CAM module with footprint and electrical specifications

#### Scenario: Machine-readable BOM export
- **WHEN** building or validating the ESP32-CAM carrier board artifacts
- **THEN** an automated script generates a structured CSV BOM containing reference designators, quantities, values, footprints, electrical ratings, and descriptions for the ESP32-CAM carrier design
