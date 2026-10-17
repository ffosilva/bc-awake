# Spec Delta

## MODIFIED Requirements

### Requirement: Persistent Storage of Configuration Parameters
The system SHALL store and retrieve configuration keys from non-volatile storage under the 'bc250' namespace. All NVS namespace and key names SHALL NOT exceed 15 characters to adhere to ESP-IDF NVS limits.

#### Scenario: Loading configuration at startup
- **WHEN** the firmware boots
- **THEN** the system reads controllers list, hostAddr, passHash, forceSetup, wolEnabled, wifiSsid, wifiPassword, bc250Mac, gracefulShutdown (using key 'graceful'), and provisioned (using key 'provisioned') from NVS Preferences, treating an absent key as disabled or false

#### Scenario: Updating configuration values
- **WHEN** any configuration setter is called
- **THEN** the system writes the updated value to NVS immediately using a key of 15 characters or fewer

#### Scenario: Migrating legacy single controller configuration
- **WHEN** the firmware boots, the controllers key is absent in NVS, but a legacy wakeAddr key exists
- **THEN** the system imports the legacy address into the controllers list as a single bound controller and maintains wakeAddr mirroring

### Requirement: Configuration Reset to Factory Defaults
The system SHALL clear all persistent configuration keys in non-volatile storage under the 'bc250' namespace, restore runtime configuration variables to factory defaults, and record a flag requesting setup mode on next boot.

#### Scenario: Resetting configuration to defaults
- **WHEN** a configuration reset is triggered
- **THEN** the system clears all stored keys in the 'bc250' NVS namespace, restores in-memory configuration to default values (empty controllers, empty hostAddr, empty passHash, empty wifiSsid, empty wifiPassword, empty bc250Mac, default hostname, wolEnabled disabled, provisioned disabled), sets forceSetup to true in NVS, and initiates a reboot into setup mode
