# Spec Delta

## MODIFIED Requirements

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
