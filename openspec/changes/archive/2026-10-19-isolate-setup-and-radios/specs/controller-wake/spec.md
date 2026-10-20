# Spec Delta

## MODIFIED Requirements

### Requirement: Controller Wake Detection
The system SHALL power on the host computer when any bound wireless controller from the authorized controllers list (up to 8) is detected while the system is in STATE_OFF in normal operation mode, and SHALL ignore wake triggers while running in setup mode.

#### Scenario: Bound controller activated while machine is off
- **WHEN** any controller present in the bound controllers list is observed within 4000 ms while in STATE_OFF, wake is not inhibited, and setup mode is not active
- **THEN** the system triggers powerOn, asserts PS_ON#, and transitions to STATE_BOOTING

#### Scenario: Bound controller activated during setup mode
- **WHEN** any bound controller is observed while the device is running in setup mode
- **THEN** the system does not trigger powerOn, keeps PS_ON# released, and remains in STATE_OFF

#### Scenario: Unbound controller activated
- **WHEN** an incoming connection request is received from a controller MAC not present in the bound controllers list
- **THEN** the system rejects the connection without triggering power-on and remains in STATE_OFF

### Requirement: Classic Bluetooth Host Impersonation
The system SHALL run page listening under the host PC's adapter MAC address and reject incoming connection requests on Classic Bluetooth targets while in STATE_OFF, requesting the remote device name on incoming connection requests only while interactive controller pairing listener mode is active, and SHALL completely disable and de-initialize the Bluetooth controller while the host is booting or powered on.

#### Scenario: Controller pages the host adapter
- **WHEN** a paired controller attempts a connection to the host adapter MAC while the machine is in STATE_OFF in normal mode
- **THEN** the system checks if the sender MAC matches any bound controller, marks presence if matched, immediately rejects the HCI connection request without link authentication or remote name request, and powers on the machine if matched

#### Scenario: Controller pages during active pairing listener
- **WHEN** a controller attempts a connection to the host adapter MAC while interactive controller pairing listener mode is active in setup mode
- **THEN** the system records the incoming controller details, requests the remote device name if not yet cached, and immediately rejects the HCI connection request without powering on the machine

#### Scenario: System transitions out of STATE_OFF
- **WHEN** the system leaves STATE_OFF into STATE_BOOTING or STATE_ON
- **THEN** the system disables and de-initializes the Bluetooth controller completely so no radio activity or MAC conflict occurs while the host is running

#### Scenario: Discovery requested while Bluetooth controller is already running
- **WHEN** wakeSetDiscovery is invoked while the Classic Bluetooth controller is already initialized and enabled
- **THEN** the system reuses the active Bluetooth controller session without re-calling esp_iface_mac_addr_set, re-registering VHCI callbacks, or issuing an HCI Reset

#### Scenario: Portal or fallback SoftAP starts while host is powered on
- **WHEN** the host is in STATE_BOOTING or STATE_ON
- **THEN** the Bluetooth controller remains completely disabled and de-initialized

### Requirement: Bluetooth Low Energy Scanning
The system SHALL continuously scan for advertising packets matching any controller in the bound controllers list (up to 8) on BLE targets while in STATE_OFF in normal mode, and SHALL completely de-initialize the BLE stack when transitioning to STATE_BOOTING or STATE_ON.

#### Scenario: BLE advertisement detected
- **WHEN** an advertisement matching any controller present in the bound controllers list is received while in STATE_OFF in normal mode
- **THEN** the system updates the last-seen timestamp and initiates power-on if cooldown has expired

#### Scenario: BLE stack disabled when host powers on
- **WHEN** the system transitions from STATE_OFF to STATE_BOOTING or STATE_ON on a BLE target
- **THEN** the system stops scanning and de-initializes the BLE stack completely
