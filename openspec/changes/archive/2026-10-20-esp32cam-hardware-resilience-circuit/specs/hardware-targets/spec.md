# Spec Delta

## ADDED Requirements

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
