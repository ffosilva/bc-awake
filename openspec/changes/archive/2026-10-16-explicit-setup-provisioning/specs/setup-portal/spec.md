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
- **THEN** the system clears forceSetup, sets provisioned to true, and transitions to normal operation mode

### Requirement: Controller Binding and Provisioning Completion
The system SHALL persist bound controller configurations and adapter addresses and reboot into normal operation when finished.

#### Scenario: Binding controller MAC address
- **WHEN** an authenticated POST request is sent to /api/select with a valid colon-separated MAC address
- **THEN** the system saves the address to NVS and returns HTTP 200 OK

#### Scenario: Adding bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/add with a valid MAC address and non-empty name
- **THEN** the system appends the controller to the bound list in NVS and returns HTTP 200 OK

#### Scenario: Removing bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/remove with a bound MAC address
- **THEN** the system deletes the controller from the bound list in NVS and returns HTTP 200 OK

#### Scenario: Renaming bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/rename with a bound MAC address and new friendly name
- **THEN** the system updates the friendly name in NVS and returns HTTP 200 OK

#### Scenario: Exceeding maximum controller capacity
- **WHEN** a client attempts to add a 9th controller to a list already containing 8 controllers
- **THEN** the system rejects the request with HTTP status 400 Bad Request

#### Scenario: Completing setup
- **WHEN** an authenticated POST request is sent to /api/finish on a device with a portal password set
- **THEN** the system clears forceSetup, sets provisioned to true in NVS, returns HTTP 200 OK, and reboots into normal mode after 800 ms, regardless of whether an adapter address, bound controllers, or network wake settings are configured

#### Scenario: Completing setup without authentication
- **WHEN** a POST request is sent to /api/finish without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized and does not clear forceSetup or modify provisioned state
