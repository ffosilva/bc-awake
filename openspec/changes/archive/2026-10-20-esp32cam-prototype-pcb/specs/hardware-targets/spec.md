# Spec Delta

## ADDED Requirements

### Requirement: AI-Thinker ESP32-CAM Prototype Carrier Board
The repository SHALL provide a KiCad single-layer prototype carrier PCB design for the AI-Thinker ESP32-CAM integrating female socket headers, resilience passive components, and external terminals formatted for protoboard replication.

#### Scenario: Female header socket dimensions
- **WHEN** socketing an AI-Thinker ESP32-CAM module
- **THEN** the carrier provides two parallel 1x8 female pin headers with 2.54 mm pin pitch and 22.86 mm (0.9 in) row-to-row spacing

#### Scenario: Single-layer routing for protoboard replication
- **WHEN** fabricating or hand-wiring on dot-matrix protoboard with holes and pads
- **THEN** all traces are routed on a single copper layer aligned to a 2.54 mm grid with a dedicated component placement silkscreen layer
