# Spec Delta

## ADDED Requirements

### Requirement: Motherboard Power Button Configuration User Interface
The portal web interface SHALL provide a Power tab allowing the user to configure and save the optional motherboard power button (`gracefulShutdown`) setting during setup mode.

#### Scenario: Displaying and saving motherboard power button setting
- **WHEN** the user opens the Power tab in setup mode, toggles the motherboard power button checkbox, and clicks Save
- **THEN** the interface sends an authenticated POST request to `/api/power` with the selected `gracefulShutdown` boolean and displays a confirmation message

## MODIFIED Requirements

### Requirement: Setup Portal Activation
The system SHALL launch a WiFi SoftAP setup portal automatically when unconfigured, or when explicitly requested by user input while in STATE_OFF, and SHALL prevent the host computer from powering on while running in setup mode.

#### Scenario: Unconfigured board boots into setup portal
- **WHEN** the firmware boots and isConfigured is false or forceSetup is true
- **THEN** the system keeps PS_ON# released in STATE_OFF, does not start Wi-Fi station mode, launches the WiFi SoftAP broadcasting 'BC250_<SUFFIX>' on 192.168.4.1 with Bluetooth discovery enabled, and starts the web server

#### Scenario: Powering on host computer during setup portal
- **WHEN** the momentary button is tapped while the setup portal is active in STATE_OFF
- **THEN** the system ignores the button tap, keeps PS_ON# released to high-impedance, and prevents the host computer from powering on

#### Scenario: Button held for setup while off
- **WHEN** the momentary button is held continuously for 8000 ms or longer while in STATE_OFF in normal mode
- **THEN** the system sets forceSetup to true and restarts into the setup portal with PS_ON# released

#### Scenario: Setup mode initiated via serial command
- **WHEN** an 's' or 'S' character is received on the serial interface while in STATE_OFF
- **THEN** the system sets forceSetup to true and restarts into the setup portal, whereas receiving 's' or 'S' while in STATE_BOOTING or STATE_ON is ignored

#### Scenario: Button held to exit setup portal
- **WHEN** the momentary button is held continuously for 8000 ms while running in setup mode
- **THEN** the system clears forceSetup, sets provisioned to true, and reboots into normal operation mode

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL restrict the HTTP web portal and REST API server exclusively to Setup Mode over SoftAP connectivity, and SHALL NOT start or keep the web server or mDNS service online during normal operation mode over Wi-Fi Station connectivity.

#### Scenario: Web server active in normal station mode
- **WHEN** the system operates in normal station mode
- **THEN** the HTTP web server does not bind or listen on port 80 and the web portal is kept offline

#### Scenario: Web server deferred when Wi-Fi is disabled or disconnected
- **WHEN** the system boots or operates in normal operation mode while Wi-Fi is disabled or not connected
- **THEN** the HTTP web server does not attempt to bind or listen on port 80 and the system runs normal power control and Bluetooth wake without asserting or resetting

#### Scenario: mDNS hostname advertised
- **WHEN** the system is running in normal station mode
- **THEN** mDNS advertisement is disabled and the web portal is not advertised on the local network

#### Scenario: Reporting host power state in status API
- **WHEN** a client performs a GET request to /api/status while in setup mode
- **THEN** the JSON response includes a string field boardState reflecting the current power state ("OFF")

#### Scenario: Web UI adapts to powered-on state
- **WHEN** the setup portal web interface is loaded in setup mode
- **THEN** the interface keeps all configuration tabs editable because setup mode locks the host in STATE_OFF

#### Scenario: Web UI allows network configuration while host is on in setup mode
- **WHEN** the setup portal web interface is loaded in setup SoftAP mode
- **THEN** the interface keeps the Network tab inputs and save button editable

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"

## REMOVED Requirements

### Requirement: Portal Power Actions Endpoints
**Reason**: The web portal is only available in Setup Mode, during which the host PC is locked in `STATE_OFF` and prevented from powering on.
**Migration**: Use the physical power button, Bluetooth controller wake, or Wake-on-LAN in normal operation mode.

### Requirement: Graceful Shutdown Action Endpoint
**Reason**: Wireless radios and the web portal are completely disabled when the host PC is in `STATE_ON`, making `/api/shutdown` unreachable.
**Migration**: Enable the motherboard power button setting in Setup Mode and tap the physical power button while the PC is running.

### Requirement: Power Tab User Interface
**Reason**: Remote power action buttons (`Power On`, `Force Cut Power`, `Shut Down OS`) are obsolete because the web portal is offline in normal mode and host power is locked off in Setup Mode.
**Migration**: Replaced by `Motherboard Power Button Configuration User Interface` for configuring the `gracefulShutdown` setting.
