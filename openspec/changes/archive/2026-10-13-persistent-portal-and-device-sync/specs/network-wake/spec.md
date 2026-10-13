# Spec Delta

## MODIFIED Requirements

### Requirement: Wi-Fi Station Management for Network Wake
The system SHALL manage Wi-Fi station (STA) connectivity in normal operation mode when network wake is enabled, dynamically assigning the station MAC address according to host power state.

#### Scenario: Network wake enabled with configured Wi-Fi credentials
- **WHEN** the system enters normal operation mode in STATE_OFF with wolEnabled set to true and a non-empty wifiSsid configured
- **THEN** the system sets the Wi-Fi Station MAC address to the configured bc250Mac and initiates a non-blocking Wi-Fi station connection to the configured access point

#### Scenario: Connecting to an open passwordless network
- **WHEN** the system initiates a Wi-Fi station connection with wolEnabled set to true and wifiPassword is empty
- **THEN** the system connects to the network using open authentication without requiring a passphrase

#### Scenario: Network wake disabled or unconfigured
- **WHEN** the system enters normal operation mode with wolEnabled set to false or wifiSsid empty
- **THEN** the system keeps the Wi-Fi station interface disabled and operates strictly offline

#### Scenario: Wi-Fi connection lost in normal mode
- **WHEN** an established Wi-Fi station connection drops while the system is running in normal operation mode
- **THEN** the system periodically attempts non-blocking reconnection in the background without obstructing button sense or Bluetooth loops

#### Scenario: Power state transitions to booting or on
- **WHEN** the system transitions from STATE_OFF to STATE_BOOTING or STATE_ON
- **THEN** the system disconnects Wi-Fi, resets the Wi-Fi Station MAC address to the ESP32 factory default MAC, and reconnects to the configured access point

#### Scenario: Power state transitions back to off
- **WHEN** the system transitions from STATE_ON to STATE_OFF with a configured bc250Mac
- **THEN** the system disconnects Wi-Fi, sets the Wi-Fi Station MAC address to bc250Mac, and reconnects to the configured access point
