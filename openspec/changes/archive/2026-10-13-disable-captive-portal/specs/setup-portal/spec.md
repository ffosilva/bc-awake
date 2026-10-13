# Spec Delta

## MODIFIED Requirements

### Requirement: Setup Portal Activation
The system SHALL launch a WiFi SoftAP setup portal when explicitly requested by user input or configuration flag.

#### Scenario: Button held for setup while off
- **WHEN** the momentary button is held continuously for 8000 ms or longer while in STATE_OFF
- **THEN** the system sets forceSetup to true and restarts into setup mode

#### Scenario: Setup mode initiated via serial command
- **WHEN** an 's' or 'S' character is received on the serial interface
- **THEN** the system sets forceSetup to true and restarts into setup mode

#### Scenario: Button held to exit setup portal
- **WHEN** the momentary button is held continuously for 8000 ms while running in setup mode
- **THEN** the system clears forceSetup and restarts into normal operation mode

### Requirement: Captive Network and Web Serving
The system SHALL broadcast an open WiFi SoftAP named 'BC250 Switch Setup' and serve the embedded setup interface from firmware memory without triggering captive network detection.

#### Scenario: Client connects to setup network
- **WHEN** a client associates with the BC250 Switch Setup access point
- **THEN** the system does not intercept DNS queries or trigger OS captive portal detection, and the web server serves embedded gzipped index.html directly from firmware memory with Content-Encoding: gzip at root path '/' without requiring a mounted filesystem

#### Scenario: Unknown route handling
- **WHEN** an unmapped HTTP path outside the REST API and root endpoints is requested
- **THEN** the web server returns HTTP status 404 Not Found rather than redirecting or serving the setup interface
