# Spec Delta

## MODIFIED Requirements

### Requirement: Wi-Fi Station Management for Network Wake
The system SHALL manage Wi-Fi station (STA) connectivity in normal operation mode when network wake is enabled, applying target Wi-Fi TX power limits and dynamically assigning the station MAC address according to host power state without overlapping ATX PSU turn-on inrush.

#### Scenario: Network wake enabled with configured Wi-Fi credentials
- **WHEN** the system enters normal operation mode in STATE_OFF with wolEnabled set to true and a non-empty wifiSsid configured
- **THEN** the system sets the Wi-Fi Station MAC address to the configured bc250Mac, initiates a non-blocking Wi-Fi station connection to the configured access point, and applies the target's configured Wi-Fi TX power limit

#### Scenario: Connecting to an open passwordless network
- **WHEN** the system initiates a Wi-Fi station connection with wolEnabled set to true and wifiPassword is empty
- **THEN** the system connects to the network using open authentication without requiring a passphrase

#### Scenario: Network wake disabled or unconfigured
- **WHEN** the system enters normal operation mode with wolEnabled set to false or wifiSsid empty
- **THEN** the system keeps the Wi-Fi station interface disabled and operates strictly offline

#### Scenario: Wi-Fi connection lost in normal mode
- **WHEN** an established Wi-Fi station connection drops while the system is running in normal operation mode
- **THEN** the system periodically attempts non-blocking reconnection in the background without obstructing button sense or Bluetooth loops

#### Scenario: Wi-Fi fallback SoftAP activates while host is powered on
- **WHEN** the Wi-Fi station connection remains disconnected for 20000 ms while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system activates the fallback SoftAP and web server with target Wi-Fi TX power limits while keeping Bluetooth discovery and page scan disabled

#### Scenario: Power state transitions to booting or on
- **WHEN** the system transitions from STATE_OFF to STATE_BOOTING or STATE_ON
- **THEN** the system staggers the Wi-Fi MAC transition after PSU turn-on, disconnects Wi-Fi, resets the Wi-Fi Station MAC address to the ESP32 factory default MAC, reconnects to the configured access point, and applies the target's configured Wi-Fi TX power limit

#### Scenario: Power state transitions back to off
- **WHEN** the system transitions from STATE_ON to STATE_OFF with a configured bc250Mac
- **THEN** the system disconnects Wi-Fi, sets the Wi-Fi Station MAC address to bc250Mac, reconnects to the configured access point, and applies the target's configured Wi-Fi TX power limit
