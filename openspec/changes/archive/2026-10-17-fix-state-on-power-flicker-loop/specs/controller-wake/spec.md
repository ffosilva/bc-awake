# Spec Delta

## MODIFIED Requirements

### Requirement: Classic Bluetooth Host Impersonation
The system SHALL run page listening under the host PC's adapter MAC address and reject incoming connection requests on Classic Bluetooth targets, requesting the remote device name on incoming connection requests only while interactive controller pairing listener mode is active, and SHALL NOT re-initialize an already-enabled Bluetooth controller or activate discovery or page scan while the host is powered on.

#### Scenario: Controller pages the host adapter
- **WHEN** a paired controller attempts a connection to the host adapter MAC while the machine is in STATE_OFF and interactive pairing mode is not active
- **THEN** the system checks if the sender MAC matches any bound controller, marks presence if matched, immediately rejects the HCI connection request without link authentication or remote name request, and powers on the machine if matched

#### Scenario: Controller pages during active pairing listener
- **WHEN** a controller attempts a connection to the host adapter MAC while interactive controller pairing listener mode is active in STATE_OFF
- **THEN** the system records the incoming controller details, requests the remote device name if not yet cached, and immediately rejects the HCI connection request

#### Scenario: System transitions out of STATE_OFF
- **WHEN** the system leaves STATE_OFF into STATE_BOOTING or STATE_ON
- **THEN** the system disables page scan and discovery to allow the controller to connect to the real host adapter and prevent radio coexistence spikes

#### Scenario: Discovery requested while Bluetooth controller is already running
- **WHEN** wakeSetDiscovery is invoked while the Classic Bluetooth controller is already initialized and enabled
- **THEN** the system reuses the active Bluetooth controller session without re-calling esp_iface_mac_addr_set, re-registering VHCI callbacks, or issuing an HCI Reset

#### Scenario: Portal or fallback SoftAP starts while host is powered on
- **WHEN** the setup portal or Wi-Fi fallback SoftAP starts while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system keeps Bluetooth discovery and page scan disabled
