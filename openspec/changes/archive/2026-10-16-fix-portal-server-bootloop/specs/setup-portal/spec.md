# Spec Delta

## MODIFIED Requirements

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL keep the HTTP web portal and REST API online in normal operation mode over Wi-Fi Station connectivity, SHALL advertise its presence via mDNS, and SHALL defer starting the web server while Wi-Fi is disabled or disconnected.

#### Scenario: Web server active in normal station mode
- **WHEN** the system connects to a configured Wi-Fi network in normal operation mode
- **THEN** the HTTP web server starts on port 80 and serves the portal UI and REST endpoints on the Station IP address

#### Scenario: Web server deferred when Wi-Fi is disabled or disconnected
- **WHEN** the system boots or operates in normal operation mode while Wi-Fi is disabled or not connected
- **THEN** the HTTP web server does not attempt to bind or listen on port 80 and the system runs normal power control and Bluetooth wake without asserting or resetting

#### Scenario: mDNS hostname advertised
- **WHEN** the system establishes Wi-Fi Station connectivity
- **THEN** the system registers the configured hostname (defaulting to bc-awake.local) via mDNS and configures the DHCP client hostname

#### Scenario: Reporting host power state in status API
- **WHEN** a client performs a GET request to /api/status
- **THEN** the JSON response includes a string field boardState reflecting the current power state ("OFF", "BOOTING", or "ON")

#### Scenario: Web UI adapts to powered-on state
- **WHEN** the setup portal web interface is loaded while the board is in STATE_ON
- **THEN** the interface marks the Network tab settings as read-only, disables the interactive pairing listener action in the Bluetooth tab, and disables firmware update controls in the System tab

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"
