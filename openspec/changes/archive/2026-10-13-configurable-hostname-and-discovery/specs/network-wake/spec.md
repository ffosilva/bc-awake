# Spec Delta

## ADDED Requirements

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
