# Hardware Targets Specification

## Purpose
Defines target-specific pin assignments, peripheral routing, RF power constraints, and build configurations.

## Requirements

### Requirement: ESP32-WROOM DevKitC Configuration
The system SHALL configure GPIOs and analog peripherals for the ESP32-WROOM DevKitC target using Classic Bluetooth.

#### Scenario: Pin and peripheral assignment
- **WHEN** compiled with BOARD_ESP32DEV
- **THEN** the system routes BUTTON_SENSE to GPIO25, BUTTON_GND to GPIO26, PS_ON_PIN to GPIO32, and BOARD_SENSE to GPIO34 on ADC1 with 11 dB attenuation

### Requirement: AI-Thinker ESP32-CAM Configuration
The system SHALL configure dedicated GPIOs, digital board sensing, and reduced Wi-Fi RF transmit power for the AI-Thinker ESP32-CAM target while keeping the onboard flash LED (GPIO 4) permanently disabled (clamped LOW).

#### Scenario: Pin assignment avoiding strapping and camera lines
- **WHEN** compiled with BOARD_ESP32CAM
- **THEN** the system routes BUTTON_SENSE to GPIO15, PS_ON_PIN to GPIO14, BOARD_SENSE to GPIO13 with INPUT_PULLDOWN, configures GPIO4 as an output driven permanently LOW so the onboard flash LED never illuminates, and caps Wi-Fi TX power to 11 dBm across SoftAP and Station modes

### Requirement: ESP32-C3 Configuration
The system SHALL configure GPIOs, factory-calibrated ADC1, reduced WiFi RF power, and BLE wake for the ESP32-C3 target.

#### Scenario: Pin assignment and RF power limit
- **WHEN** compiled with BOARD_ESP32C3
- **THEN** the system routes BUTTON_SENSE to GPIO5, BUTTON_GND to GPIO6, PS_ON_PIN to GPIO4, BOARD_SENSE to GPIO3, sets WiFi TX power to 8.5 dBm, and uses NimBLE for BLE wake

### Requirement: Target Hardware Identification Metadata
Each compilation target SHALL define a unique target string corresponding to the hardware platform architecture and board type.

#### Scenario: DevKitC target identification
- **WHEN** compiled for the esp32dev environment
- **THEN** the target string is defined as esp32dev

#### Scenario: ESP32-CAM target identification
- **WHEN** compiled for the esp32cam environment
- **THEN** the target string is defined as esp32cam

#### Scenario: ESP32-C3 target identification
- **WHEN** compiled for the esp32-c3-devkitm-1 environment
- **THEN** the target string is defined as esp32-c3

### Requirement: Dual-Bank OTA Partition Layout
All build environments SHALL utilize a flash partition layout providing dual equal-sized application banks and a reserved filesystem partition.

#### Scenario: Partition allocation on 4 MB flash
- **WHEN** the partition table is parsed by the bootloader
- **THEN** the flash contains app0 (ota_0) and app1 (ota_1) of 1.81 MB (0x1D0000) each, an otadata partition of 8 KB (0x2000), and a SPIFFS partition of 256 KB (0x40000)

### Requirement: Optional Power-Button Output Pin
Each hardware target SHALL define a dedicated GPIO for the optional motherboard power-button connection, chosen to avoid camera, flash, PSRAM, and boot-strapping conflicts for normal operation.

#### Scenario: ESP32-CAM power-button pin
- **WHEN** compiled with BOARD_ESP32CAM
- **THEN** the system routes the power-button line to GPIO2

#### Scenario: ESP32-WROOM DevKitC power-button pin
- **WHEN** compiled with BOARD_ESP32DEV
- **THEN** the system routes the power-button line to GPIO27

#### Scenario: ESP32-C3 power-button pin
- **WHEN** compiled with BOARD_ESP32C3
- **THEN** the system routes the power-button line to GPIO7

#### Scenario: Pin left untouched when feature unused
- **WHEN** graceful shutdown is disabled
- **THEN** the power-button pin is left as a high-impedance input on every target

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
- **THEN** reference wiring specifies a low-side N-channel MOSFET buffer with gate pull-down and snubber network to isolate the 3.3V GPIO from 5V pull-up

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

### Requirement: AI-Thinker ESP32-CAM Bill of Materials
The repository SHALL provide a detailed Bill of Materials (BOM) in documentation and machine-readable format specifying component packages, electrical ratings, tolerances, and wiring harness components dedicated specifically to the AI-Thinker ESP32-CAM module and prototype carrier board.

#### Scenario: Documentation BOM reference
- **WHEN** consulting the ESP32-CAM Bill of Materials documentation
- **THEN** the documentation provides an itemized list of all carrier PCB passives, headers, ATX harness leads, switches, and the ESP32-CAM module with footprint and electrical specifications

#### Scenario: Machine-readable BOM export
- **WHEN** building or validating the ESP32-CAM carrier board artifacts
- **THEN** an automated script generates a structured CSV BOM containing reference designators, quantities, values, footprints, electrical ratings, and descriptions for the ESP32-CAM carrier design

