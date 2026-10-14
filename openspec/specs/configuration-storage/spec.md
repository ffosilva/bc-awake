# Configuration Storage Specification

## Purpose
Manages persistent non-volatile storage for controller bindings, adapter MACs, credentials, and runtime options.

## Requirements

### Requirement: Persistent Storage of Configuration Parameters
The system SHALL store and retrieve configuration keys from non-volatile storage under the 'bc250' namespace.

#### Scenario: Loading configuration at startup
- **WHEN** the firmware boots
- **THEN** the system reads controllers list, hostAddr, passHash, forceSetup, flashOnWake, wolEnabled, wifiSsid, wifiPassword, and bc250Mac from NVS Preferences

#### Scenario: Updating configuration values
- **WHEN** any configuration setter is called
- **THEN** the system writes the updated value to NVS immediately

#### Scenario: Migrating legacy single controller configuration
- **WHEN** the firmware boots, the controllers key is absent in NVS, but a legacy wakeAddr key exists
- **THEN** the system imports the legacy address into the controllers list as a single bound controller and maintains wakeAddr mirroring

### Requirement: Provisioning Status Evaluation
The system SHALL determine whether the device is fully provisioned based on required target-specific parameters.

#### Scenario: Provisioned Classic Bluetooth device
- **WHEN** passHash and hostAddr are non-empty strings and at least one bound controller exists in the controllers list on Classic BT targets
- **THEN** isConfigured returns true

#### Scenario: Provisioned BLE device
- **WHEN** passHash and wakeAddr are both non-empty strings on BLE targets
- **THEN** isConfigured returns true

#### Scenario: Missing required parameters
- **WHEN** any required parameter or the controller list is empty
- **THEN** isConfigured returns false

### Requirement: Configuration Reset to Factory Defaults
The system SHALL clear all persistent configuration keys in non-volatile storage under the 'bc250' namespace, restore runtime configuration variables to factory defaults, and record a flag requesting setup mode on next boot.

#### Scenario: Resetting configuration to defaults
- **WHEN** a configuration reset is triggered
- **THEN** the system clears all stored keys in the 'bc250' NVS namespace, restores in-memory configuration to default values (empty controllers, empty hostAddr, empty passHash, empty wifiSsid, empty wifiPassword, empty bc250Mac, default hostname, flashOnWake enabled, wolEnabled disabled), sets forceSetup to true in NVS, and initiates a reboot into setup mode
