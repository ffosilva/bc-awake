# Spec Delta

## ADDED Requirements

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
