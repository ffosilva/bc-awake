# Spec Delta: Controller Wake

## MODIFIED Requirements

### Requirement: Bluetooth Low Energy Scanning
The system SHALL continuously scan for advertising packets matching any controller in the bound controllers list (up to 8) on BLE targets.

#### Scenario: BLE advertisement detected
- **WHEN** an advertisement matching any controller present in the bound controllers list is received while in STATE_OFF
- **THEN** the system updates the last-seen timestamp and initiates power-on if cooldown has expired

## ADDED Requirements

### Requirement: Dynamic Runtime Controller List Synchronization
The system SHALL synchronize the active in-memory wake detection address list immediately upon any runtime modification of the bound controllers list without requiring a system reboot.

#### Scenario: Controller added at runtime
- **WHEN** a controller is successfully added via the setup portal or REST API while the system is running
- **THEN** the system immediately updates the active wake filter so that wake attempts from the new controller are recognized without rebooting the controller

#### Scenario: Controller removed at runtime
- **WHEN** a controller is removed via the setup portal or REST API while the system is running
- **THEN** the system immediately updates the active wake filter so that wake attempts from the removed controller are rejected without rebooting the controller
