# Spec Delta

## ADDED Requirements

### Requirement: Network Wake Configuration
The system SHALL provide portal UI controls and API endpoints to configure network wake settings, credentials, and target hardware addresses.

#### Scenario: Updating network wake settings with passphrase
- **WHEN** an authenticated POST request is received at /api/network with valid wolEnabled, wifiSsid, wifiPassword, and colon-delimited bc250Mac
- **THEN** the system persists the configuration to NVS and returns HTTP 200 OK

#### Scenario: Updating network wake settings for an open network
- **WHEN** an authenticated POST request is received at /api/network with an empty wifiPassword and a non-empty wifiSsid
- **THEN** the system persists the configuration with an empty password and returns HTTP 200 OK

#### Scenario: Disabling network wake
- **WHEN** an authenticated POST request is received at /api/network with wolEnabled set to false
- **THEN** the system updates wolEnabled in NVS to false and returns HTTP 200 OK

#### Scenario: Inspecting network wake status
- **WHEN** an authenticated GET request is received at /api/status
- **THEN** the response includes wolEnabled, wifiSsid, bc250Mac, and an indicator of whether a Wi-Fi password is saved
