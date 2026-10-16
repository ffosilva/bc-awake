# Spec Delta

## MODIFIED Requirements

### Requirement: Interactive Controller Listener
The system SHALL capture incoming connection requests during setup to provide real-time controller detection while the host machine is in STATE_OFF, and SHALL reject sensing requests while the host is powered on.

#### Scenario: Controller pages during active listener session
- **WHEN** a paired controller pages the host adapter while an authenticated client is querying /api/listener/status and the host is in STATE_OFF
- **THEN** the system returns detected as true along with the controller MAC address, resolved or heuristic friendly name, and whether it is already bound

#### Scenario: Clearing or starting listener session
- **WHEN** an authenticated POST request is sent to /api/listener/start while the host is in STATE_OFF
- **THEN** the system clears the last-captured controller detection slot and returns HTTP 200 OK

#### Scenario: Attempting to start listener while host machine is on
- **WHEN** an authenticated POST request is sent to /api/listener/start while the host is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the request with HTTP status 409 Conflict to prevent radio interference with the host's active Bluetooth adapter

### Requirement: Network Wake Configuration
The system SHALL provide portal UI controls and API endpoints to configure network wake settings, credentials, and target hardware addresses while off, and SHALL enforce read-only access while the host is on.

#### Scenario: Updating network wake settings with passphrase
- **WHEN** an authenticated POST request is received at /api/network with valid wolEnabled, wifiSsid, wifiPassword, and colon-delimited bc250Mac while in STATE_OFF
- **THEN** the system persists the configuration to NVS and returns HTTP 200 OK

#### Scenario: Updating network wake settings for an open network
- **WHEN** an authenticated POST request is received at /api/network with an empty wifiPassword and a non-empty wifiSsid while in STATE_OFF
- **THEN** the system persists the configuration with an empty password and returns HTTP 200 OK

#### Scenario: Disabling network wake
- **WHEN** an authenticated POST request is received at /api/network with wolEnabled set to false while in STATE_OFF
- **THEN** the system updates wolEnabled in NVS to false and returns HTTP 200 OK

#### Scenario: Inspecting network wake status
- **WHEN** an authenticated GET request is received at /api/status
- **THEN** the response includes wolEnabled, wifiSsid, bc250Mac, and an indicator of whether a Wi-Fi password is saved

#### Scenario: Attempting to update network wake settings while host machine is on
- **WHEN** an authenticated POST request is received at /api/network while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the update with HTTP status 403 Forbidden without modifying stored network configuration

## ADDED Requirements

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL keep the HTTP web portal and REST API online in normal operation mode over Wi-Fi Station connectivity and SHALL advertise its presence via mDNS.

#### Scenario: Web server active in normal station mode
- **WHEN** the system connects to a configured Wi-Fi network in normal operation mode
- **THEN** the HTTP web server starts on port 80 and serves the portal UI and REST endpoints on the Station IP address

#### Scenario: mDNS hostname advertised
- **WHEN** the system establishes Wi-Fi Station connectivity
- **THEN** the system advertises the hostname bc-awake.local via mDNS

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
