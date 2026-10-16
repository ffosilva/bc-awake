# Spec Delta

## Purpose
Provides a Linux systemd background service to discover local Bluetooth adapters and paired controllers, synchronizing them to the ESP32 switch via its REST API.

## ADDED Requirements

### Requirement: Local Bluetooth State Discovery
The host sync tool SHALL discover the primary Bluetooth controller MAC address and all paired gamepad devices from the local Linux BlueZ subsystem.

#### Scenario: Discovering host adapter and paired controllers
- **WHEN** the host sync tool runs on a Linux system with active BlueZ services
- **THEN** it retrieves the default Bluetooth adapter MAC address and a list of paired gamepad MAC addresses and friendly names

#### Scenario: No paired controllers found
- **WHEN** the host sync tool queries BlueZ and no controllers are paired
- **THEN** it reports an empty controller list without halting or raising errors

### Requirement: REST API Synchronization
The host sync tool SHALL authenticate with the ESP32 REST API, verify host adapter consistency, and synchronize the list of bound controllers using existing endpoints.

#### Scenario: Successful controller list synchronization
- **WHEN** the sync tool connects to http://bc-awake.local and authenticates with the configured password
- **THEN** it updates the host adapter address via /api/host if different, removes controllers no longer paired locally via /api/controllers/remove, and adds newly paired local controllers via /api/controllers/add

#### Scenario: Device limit protection
- **WHEN** more than 8 controllers are paired locally on the host
- **THEN** the sync tool synchronizes only the first 8 controllers and logs a warning

#### Scenario: ESP32 switch unreachable
- **WHEN** the host sync tool cannot reach the ESP32 switch over mDNS or HTTP
- **THEN** the service logs a connection failure and exits cleanly to be retried on the next timer interval

### Requirement: Systemd Service Integration
The sync tool SHALL provide systemd service and timer units compatible with standard and immutable Linux distributions including Bazzite, SteamOS, Ubuntu, and Arch Linux.

#### Scenario: Periodic background sync execution
- **WHEN** the systemd timer triggers
- **THEN** the service unit executes the sync tool, performs the synchronization pass, and terminates upon completion
