# Network Wake Specification

## Purpose
Provides Wi-Fi station connectivity and Wake-on-LAN (WOL) magic packet interception to power on the system from an unpowered state.

## Requirements

### Requirement: Wi-Fi Station Management for Network Wake
The system SHALL manage Wi-Fi station (STA) connectivity exclusively in STATE_OFF when network wake is enabled, applying target Wi-Fi TX power limits before connection initiation and assigning the configured station MAC address without maintaining Wi-Fi connectivity or deferring MAC swaps during host power-on or running states.

#### Scenario: Network wake enabled with configured Wi-Fi credentials
- **WHEN** the system enters normal operation mode in STATE_OFF with wolEnabled set to true and a non-empty wifiSsid configured
- **THEN** the system sets the Wi-Fi Station MAC address to the configured bc250Mac, clamps PHY TX power to the target's configured limit before initiating the connection, connects to the configured access point, and maintains the configured Wi-Fi TX power limit

#### Scenario: Connecting to an open passwordless network
- **WHEN** the system initiates a Wi-Fi station connection with wolEnabled set to true and wifiPassword is empty
- **THEN** the system connects to the network using open authentication without requiring a passphrase

#### Scenario: Network wake disabled or unconfigured
- **WHEN** the system enters normal operation mode with wolEnabled set to false or wifiSsid empty
- **THEN** the system keeps the Wi-Fi station interface disabled and operates strictly offline

#### Scenario: Wi-Fi connection lost in normal mode
- **WHEN** an established Wi-Fi station connection drops while the system is running in STATE_OFF
- **THEN** the system periodically attempts non-blocking reconnection in the background without obstructing button sense or Bluetooth loops

#### Scenario: Wi-Fi fallback SoftAP activates while host is powered on
- **WHEN** the host is in STATE_BOOTING or STATE_ON
- **THEN** the Wi-Fi radio is powered off and the fallback SoftAP does not activate

#### Scenario: Power state transitions to booting or on
- **WHEN** the system transitions from STATE_OFF to STATE_BOOTING or STATE_ON
- **THEN** the system immediately disconnects Wi-Fi, stops UDP listeners, and powers down the Wi-Fi radio without initiating reconnection or deferring MAC address swapping

#### Scenario: Host boot aborts or times out before reaching STATE_ON
- **WHEN** the system transitions from STATE_BOOTING back to STATE_OFF due to watchdog timeout or cancellation
- **THEN** the system re-initializes Wi-Fi station connectivity with the configured bc250Mac if network wake is enabled

#### Scenario: Power state transitions back to off
- **WHEN** the system transitions from STATE_ON to STATE_OFF with a configured bc250Mac and wolEnabled set to true
- **THEN** the system enables Wi-Fi Station mode, sets the Wi-Fi Station MAC address to bc250Mac, clamps PHY TX power to the target limit, connects to the configured access point, and listens for WOL magic packets

### Requirement: Wake-on-LAN Magic Packet Detection
The system SHALL inspect incoming UDP datagrams on WOL ports while in STATE_OFF and trigger a power-on sequence upon receiving a valid magic packet targeting the configured BC250 Ethernet MAC address, but SHALL ignore magic packets while setup mode is active.

#### Scenario: Valid magic packet received while machine is off
- **WHEN** a UDP datagram containing 6 bytes of 0xFF followed by 16 iterations of bc250Mac is received on port 9 or port 7 while in STATE_OFF, wake is not inhibited, and setup mode is not active
- **THEN** the system triggers powerOn, asserts PS_ON#, and transitions to STATE_BOOTING

#### Scenario: Magic packet received during setup mode
- **WHEN** a magic packet is received while the device is in setup mode
- **THEN** the system discards the packet and remains in the unpowered state

#### Scenario: Magic packet received during power-off cooldown
- **WHEN** a valid magic packet is received within the wake cooldown window following a power-off event
- **THEN** the system ignores the packet and remains in STATE_OFF

#### Scenario: Magic packet received while machine is booting or running
- **WHEN** a valid magic packet is received while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system ignores the packet without resetting or toggling power lines

#### Scenario: Incoming packet does not match target MAC
- **WHEN** an incoming UDP packet on port 9 or port 7 does not match the configured bc250Mac payload
- **THEN** the system discards the packet without altering power state
