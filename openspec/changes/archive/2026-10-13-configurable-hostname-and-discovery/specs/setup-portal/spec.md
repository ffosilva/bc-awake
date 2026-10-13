# Spec Delta

## MODIFIED Requirements

### Requirement: Network Wake Configuration
The system SHALL provide portal UI controls and API endpoints to configure network wake settings, credentials, and target hardware addresses while off, and SHALL enforce read-only access while the host is on.

#### Scenario: Updating network wake settings with passphrase
- **WHEN** an authenticated POST request is received at /api/network with valid wolEnabled, wifiSsid, wifiPassword, colon-delimited bc250Mac, and optional hostname while in STATE_OFF
- **THEN** the system persists the configuration to NVS and returns HTTP 200 OK

#### Scenario: Updating network wake settings for an open network
- **WHEN** an authenticated POST request is received at /api/network with an empty wifiPassword and a non-empty wifiSsid while in STATE_OFF
- **THEN** the system persists the configuration with an empty password and returns HTTP 200 OK

#### Scenario: Disabling network wake
- **WHEN** an authenticated POST request is received at /api/network with wolEnabled set to false while in STATE_OFF
- **THEN** the system updates wolEnabled in NVS to false and returns HTTP 200 OK

#### Scenario: Inspecting network wake status
- **WHEN** an authenticated GET request is received at /api/status
- **THEN** the response includes wolEnabled, wifiSsid, bc250Mac, hostname, and an indicator of whether a Wi-Fi password is saved

#### Scenario: Attempting to update network wake settings while host machine is on
- **WHEN** an authenticated POST request is received at /api/network while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the update with HTTP status 403 Forbidden without modifying stored network configuration

#### Scenario: Validating hostname format
- **WHEN** an authenticated POST request is received at /api/network with a hostname containing characters outside [a-z0-9-] or length exceeding 32 characters
- **THEN** the system rejects the update with HTTP status 400 Bad Request

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL keep the HTTP web portal and REST API online in normal operation mode over Wi-Fi Station connectivity and SHALL advertise its presence via mDNS.

#### Scenario: Web server active in normal station mode
- **WHEN** the system connects to a configured Wi-Fi network in normal operation mode
- **THEN** the HTTP web server starts on port 80 and serves the portal UI and REST endpoints on the Station IP address

#### Scenario: mDNS hostname advertised
- **WHEN** the system establishes Wi-Fi Station connectivity
- **THEN** the system registers the configured hostname (defaulting to bc250-switch.local) via mDNS and configures the DHCP client hostname

#### Scenario: Reporting host power state in status API
- **WHEN** a client performs a GET request to /api/status
- **THEN** the JSON response includes a string field boardState reflecting the current power state ("OFF", "BOOTING", or "ON")

#### Scenario: Web UI adapts to powered-on state
- **WHEN** the setup portal web interface is loaded while the board is in STATE_ON
- **THEN** the interface marks the Wi-Fi configuration card as read-only and disables the interactive pairing listener modal

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"
