# Network Wake Specification

## Purpose
Provides Wi-Fi station connectivity and Wake-on-LAN (WOL) magic packet interception to power on the system from an unpowered state.

## Requirements

### Requirement: Wi-Fi Station Management for Network Wake
The system SHALL manage Wi-Fi station (STA) connectivity in normal operation mode when network wake is enabled, applying target Wi-Fi TX power limits before connection initiation and dynamically assigning the station MAC address according to host power state without overlapping ATX PSU turn-on inrush or transient boot states.

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
- **WHEN** an established Wi-Fi station connection drops while the system is running in normal operation mode
- **THEN** the system periodically attempts non-blocking reconnection in the background without obstructing button sense or Bluetooth loops

#### Scenario: Wi-Fi fallback SoftAP activates while host is powered on
- **WHEN** the Wi-Fi station connection remains disconnected for 20000 ms while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system activates the fallback SoftAP and web server with target Wi-Fi TX power limits while keeping Bluetooth discovery and page scan disabled

#### Scenario: Power state transitions to booting or on
- **WHEN** the system transitions from STATE_OFF to STATE_BOOTING or STATE_ON
- **THEN** the system maintains the existing Wi-Fi station connection untouched during STATE_BOOTING to avoid overlapping ATX PSU turn-on inrush, defers restoring the factory default MAC address across a 5000 ms stabilization window after entering STATE_ON, clamps PHY TX power to the target limit before connection initiation, and reconnects to the configured access point

#### Scenario: Host boot aborts or times out before reaching STATE_ON
- **WHEN** the system transitions from STATE_BOOTING back to STATE_OFF due to watchdog timeout or user cancellation
- **THEN** the system cancels any pending MAC address swap without disconnecting Wi-Fi or changing station configuration

#### Scenario: Power state transitions back to off
- **WHEN** the system transitions from STATE_ON to STATE_OFF with a configured bc250Mac
- **THEN** the system disconnects Wi-Fi, sets the Wi-Fi Station MAC address to bc250Mac, clamps PHY TX power to the target limit before connection initiation, reconnects to the configured access point, and maintains target Wi-Fi TX power limits

### Requirement: Wake-on-LAN Magic Packet Detection
The system SHALL inspect incoming UDP datagrams on WOL ports and trigger a power-on sequence upon receiving a valid magic packet targeting the configured BC250 Ethernet MAC address.

#### Scenario: Valid magic packet received while machine is off
- **WHEN** a UDP datagram containing 6 bytes of 0xFF followed by 16 iterations of bc250Mac is received on port 9 or port 7 while in STATE_OFF and wake is not inhibited
- **THEN** the system triggers powerOn, asserts PS_ON#, and transitions to STATE_BOOTING

#### Scenario: Magic packet received during power-off cooldown
- **WHEN** a valid magic packet is received within the wake cooldown window following a power-off event
- **THEN** the system ignores the packet and remains in STATE_OFF

#### Scenario: Magic packet received while machine is booting or running
- **WHEN** a valid magic packet is received while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system ignores the packet without resetting or toggling power lines

#### Scenario: Incoming packet does not match target MAC
- **WHEN** an incoming UDP packet on port 9 or port 7 does not match the configured bc250Mac payload
- **THEN** the system discards the packet without altering power state

### Requirement: UDP Broadcast Network Discovery
The system SHALL respond to network discovery broadcast datagrams on UDP port 25025 across all power states whenever Wi-Fi station connectivity is active.

#### Scenario: Valid discovery magic phrase received
- **WHEN** a UDP broadcast datagram containing the ASCII string "BC250_DISCOVER" is received on port 25025
- **THEN** the system sends a unicast UDP datagram to the sender's IP and port containing a JSON v1 payload with device, hostname, ip, mac, boardState, version, and port

#### Scenario: Discovery request received while machine is powered on
- **WHEN** a valid discovery datagram is received on port 25025 while the system is in STATE_ON
- **THEN** the system responds with unicast JSON containing boardState set to "ON" and the factory default MAC address

#### Scenario: Discovery request received while machine is powered off
- **WHEN** a valid discovery datagram is received on port 25025 while the system is in STATE_OFF
- **THEN** the system responds with unicast JSON containing boardState set to "OFF" and the currently active MAC address

#### Scenario: Invalid discovery payload received
- **WHEN** an incoming UDP datagram on port 25025 does not match "BC250_DISCOVER"
- **THEN** the system discards the packet without sending a response
