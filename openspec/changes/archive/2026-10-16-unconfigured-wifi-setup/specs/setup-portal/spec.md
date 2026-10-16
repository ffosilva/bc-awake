# Spec Delta: Setup Portal

## MODIFIED Requirements

### Requirement: Setup Portal Activation
The system SHALL launch a WiFi SoftAP setup portal automatically when unconfigured, or when explicitly requested by user input while off, while maintaining active power-button and power-state monitoring so the host computer can power on and operate during configuration.

#### Scenario: Unconfigured board boots into setup portal
- **WHEN** the firmware boots and isConfigured is false or forceSetup is true
- **THEN** the system launches the WiFi SoftAP broadcasting 'BC250_<SUFFIX>' on 192.168.4.1 and starts the web server while initializing the power state machine in STATE_OFF with push-button controls enabled

#### Scenario: Powering on host computer during setup portal
- **WHEN** the momentary button is pressed while the setup portal is active in STATE_OFF
- **THEN** the system asserts PS_ON# to power up the PSU, transitions to STATE_BOOTING and STATE_ON upon detecting TPMS1 HIGH, and keeps the WiFi SoftAP and web server running without interruption

#### Scenario: Button held for setup while off
- **WHEN** the momentary button is held continuously for 8000 ms or longer while in STATE_OFF
- **THEN** the system sets forceSetup to true and starts the setup portal without cutting PSU power if already running

#### Scenario: Setup mode initiated via serial command
- **WHEN** an 's' or 'S' character is received on the serial interface
- **THEN** the system sets forceSetup to true and starts the setup portal without releasing PS_ON#

#### Scenario: Button held to exit setup portal
- **WHEN** the momentary button is held continuously for 8000 ms while running in setup mode and in STATE_OFF
- **THEN** the system clears forceSetup and transitions to normal operation mode

### Requirement: Network Wake Configuration
The system SHALL provide portal UI controls and API endpoints to configure network wake settings, credentials, and target hardware addresses, and SHALL allow network configuration while in setup mode regardless of host computer power state.

#### Scenario: Updating network wake settings with passphrase
- **WHEN** an authenticated POST request is received at /api/network with valid wolEnabled, wifiSsid, wifiPassword, colon-delimited bc250Mac, and optional hostname
- **THEN** the system persists the configuration to NVS and returns HTTP 200 OK

#### Scenario: Updating network wake settings for an open network
- **WHEN** an authenticated POST request is received at /api/network with an empty wifiPassword and a non-empty wifiSsid
- **THEN** the system persists the configuration with an empty password and returns HTTP 200 OK

#### Scenario: Disabling network wake
- **WHEN** an authenticated POST request is received at /api/network with wolEnabled set to false
- **THEN** the system updates wolEnabled in NVS to false and returns HTTP 200 OK

#### Scenario: Inspecting network wake status
- **WHEN** an authenticated GET request is received at /api/status
- **THEN** the response includes wolEnabled, wifiSsid, bc250Mac, hostname, and an indicator of whether a Wi-Fi password is saved

#### Scenario: Updating network wake settings while host machine is on during setup mode
- **WHEN** an authenticated POST request is received at /api/network while the system is in setup mode and the host machine is in STATE_BOOTING or STATE_ON
- **THEN** the system accepts and persists the network configuration to NVS and returns HTTP 200 OK

#### Scenario: Attempting to update network wake settings while host machine is on
- **WHEN** an authenticated POST request is received at /api/network in normal station mode while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the update with HTTP status 403 Forbidden without modifying stored network configuration

#### Scenario: Validating hostname format
- **WHEN** an authenticated POST request is received at /api/network with a hostname containing characters outside [a-z0-9-] or length exceeding 32 characters
- **THEN** the system rejects the update with HTTP status 400 Bad Request

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL keep the HTTP web portal and REST API online in normal operation mode over Wi-Fi Station connectivity, SHALL advertise its presence via mDNS, and SHALL adapt interface editing locks based on operational mode and host power state.

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
- **WHEN** the setup portal web interface is loaded in normal station mode while the board is in STATE_ON
- **THEN** the interface marks the Network tab settings as read-only, disables the interactive pairing listener action in the Bluetooth tab, and disables firmware update controls in the System tab

#### Scenario: Web UI allows network configuration while host is on in setup mode
- **WHEN** the setup portal web interface is loaded in setup SoftAP mode while the board is in STATE_ON
- **THEN** the interface keeps the Network tab inputs and save button editable, while keeping the interactive pairing listener action and firmware update controls disabled

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"
