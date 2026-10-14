# Spec Delta

## ADDED Requirements

### Requirement: Configuration Reset to Factory Defaults
The system SHALL clear all persistent configuration keys in non-volatile storage under the 'bc250' namespace, restore runtime configuration variables to factory defaults, and record a flag requesting setup mode on next boot.

#### Scenario: Resetting configuration to defaults
- **WHEN** a configuration reset is triggered
- **THEN** the system clears all stored keys in the 'bc250' NVS namespace, restores in-memory configuration to default values (empty controllers, empty hostAddr, empty passHash, empty wifiSsid, empty wifiPassword, empty bc250Mac, default hostname, flashOnWake enabled, wolEnabled disabled), sets forceSetup to true in NVS, and initiates a reboot into setup mode
