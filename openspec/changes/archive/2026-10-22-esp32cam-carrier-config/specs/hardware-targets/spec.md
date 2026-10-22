# Spec Delta

## ADDED Requirements

### Requirement: AI-Thinker ESP32-CAM Carrier Board Configuration
The system SHALL support a dedicated compilation target and macro configuration for the AI-Thinker ESP32-CAM prototype carrier board that drives `PS_ON#` through a low-side NPN BJT switch and senses `TPMS1` board power via `ADC2` when wireless radios are inactive.

#### Scenario: Pin assignment and peripheral routing on carrier board
- **WHEN** compiled with `BOARD_ESP32CAM` and `BOARD_ESP32CAM_CARRIER`
- **THEN** the system routes `BUTTON_SENSE` to GPIO15, `PS_ON_PIN` to GPIO14 as a push-pull active-`HIGH` output driving the base of an NPN transistor, `BOARD_SENSE` to GPIO13 with `ADC2` attenuation, and clamps GPIO4 permanently LOW

## MODIFIED Requirements

### Requirement: Target Hardware Identification Metadata
Each compilation target SHALL define a unique target string corresponding to the hardware platform architecture and board type.

#### Scenario: DevKitC target identification
- **WHEN** compiled for the esp32dev environment
- **THEN** the target string is defined as esp32dev

#### Scenario: ESP32-CAM target identification
- **WHEN** compiled for the esp32cam environment
- **THEN** the target string is defined as esp32cam

#### Scenario: ESP32-CAM Carrier target identification
- **WHEN** compiled for the esp32cam-carrier environment
- **THEN** the target string is defined as esp32cam-carrier

#### Scenario: ESP32-C3 target identification
- **WHEN** compiled for the esp32-c3-devkitm-1 environment
- **THEN** the target string is defined as esp32-c3

### Requirement: AI-Thinker ESP32-CAM Hardware Circuit Resilience
The system documentation and reference specifications SHALL define power decoupling, input termination, and output isolation circuits for the AI-Thinker ESP32-CAM to prevent brownouts and floating pin states.

#### Scenario: Power rail decoupling specification
- **WHEN** deploying an AI-Thinker ESP32-CAM module
- **THEN** reference wiring specifies bulk low-ESR capacitance of at least 470 µF with high-frequency 100 nF ceramic decoupling across 5V and GND

#### Scenario: Input termination and filtering on GPIO 13
- **WHEN** wiring TPMS1 board-sense to GPIO 13 on ESP32-CAM
- **THEN** reference wiring specifies an external pull-down resistor of 4.7 kΩ to 10 kΩ to override the onboard 47 kΩ MicroSD pull-up, paired with an RC low-pass filter

#### Scenario: PS_ON# output isolation buffer
- **WHEN** driving ATX PS_ON# from GPIO 14 on ESP32-CAM
- **THEN** reference wiring specifies a low-side NPN BJT buffer (BC547 / BC548) with base current-limiting resistor, base pull-down, and snubber network to isolate the 3.3V GPIO from 5V pull-up

### Requirement: AI-Thinker ESP32-CAM Prototype Carrier Board
The repository SHALL provide a KiCad single-layer prototype carrier PCB design for the AI-Thinker ESP32-CAM integrating female socket headers, resilience passive components, and external terminals formatted for protoboard replication.

#### Scenario: Female header socket dimensions
- **WHEN** socketing an AI-Thinker ESP32-CAM module
- **THEN** the carrier provides two parallel 1x8 female pin headers with 2.54 mm pin pitch and 22.86 mm (0.9 in) row-to-row spacing

#### Scenario: Single-layer routing for protoboard replication
- **WHEN** fabricating or hand-wiring on dot-matrix protoboard with holes and pads
- **THEN** all traces are routed on a single copper layer aligned to a 2.54 mm grid with a dedicated component placement silkscreen layer

#### Scenario: Socket pin mapping to physical ESP32-CAM headers
- **WHEN** wiring female socket headers to the carrier circuit
- **THEN** the socket pins match the physical AI-Thinker ESP32-CAM module pinout with 5V on Left Pin 1, GND on Left Pin 2 and Right Pin 4, signal lines on Left Pins 4-7, and leaving IO16 (PSRAM, Right Pin 2) and GND/R (Reset, Right Pin 8) completely isolated and ungrounded

#### Scenario: Transistor footprint and orientation for BC547 and BC548
- **WHEN** placing the low-side buffer transistor on the carrier board
- **THEN** the footprint matches a standard TO-92 inline package with Pin 1 (Collector) connected to `PS_ON#`, Pin 2 (Base) connected to gate/base drive network, and Pin 3 (Emitter) connected to `GND`, with silkscreen indicating the flat face facing the bottom row (Row 9)

### Requirement: AI-Thinker ESP32-CAM Bill of Materials
The repository SHALL provide a detailed Bill of Materials (BOM) in documentation and machine-readable format specifying component packages, electrical ratings, tolerances, and wiring harness components dedicated specifically to the AI-Thinker ESP32-CAM module and prototype carrier board.

#### Scenario: Documentation BOM reference
- **WHEN** consulting the ESP32-CAM Bill of Materials documentation
- **THEN** the documentation provides an itemized list of all carrier PCB passives, headers, ATX harness leads, switches, and the ESP32-CAM module with footprint and electrical specifications, specifying BC547 / BC548 for Q1

#### Scenario: Machine-readable BOM export
- **WHEN** building or validating the ESP32-CAM carrier board artifacts
- **THEN** an automated script generates a structured CSV BOM containing reference designators, quantities, values, footprints, electrical ratings, and descriptions for the ESP32-CAM carrier design specifying BC547 / BC548
