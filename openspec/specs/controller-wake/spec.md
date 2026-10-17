# Controller Wake Specification

## Purpose
Detects paired wireless gamepads via Classic Bluetooth or BLE to wake the BC250 host from an unpowered state.

## Requirements

### Requirement: Controller Wake Detection
The system SHALL power on the host computer when any bound wireless controller from the authorized controllers list (up to 8) is detected while the system is in STATE_OFF.

#### Scenario: Bound controller activated while machine is off
- **WHEN** any controller present in the bound controllers list is observed within 4000 ms while in STATE_OFF and wake is not inhibited
- **THEN** the system triggers powerOn, asserts PS_ON#, and transitions to STATE_BOOTING

#### Scenario: Unbound controller activated
- **WHEN** an incoming connection request is received from a controller MAC not present in the bound controllers list
- **THEN** the system rejects the connection without triggering power-on and remains in STATE_OFF

### Requirement: Wake Cooldown Inhibitor
The system SHALL inhibit controller wake triggers for a defined cooldown duration following every power-off event.

#### Scenario: Controller reconnect packets during cooldown
- **WHEN** controller activity is detected within 15000 ms after transitioning to STATE_OFF
- **THEN** the system ignores the wake signal and remains in STATE_OFF

### Requirement: Classic Bluetooth Host Impersonation
The system SHALL run page listening under the host PC's adapter MAC address and reject incoming connection requests on Classic Bluetooth targets, requesting the remote device name on incoming connection requests only while interactive controller pairing listener mode is active.

#### Scenario: Controller pages the host adapter
- **WHEN** a paired controller attempts a connection to the host adapter MAC while the machine is in STATE_OFF and interactive pairing mode is not active
- **THEN** the system checks if the sender MAC matches any bound controller, marks presence if matched, immediately rejects the HCI connection request without link authentication or remote name request, and powers on the machine if matched

#### Scenario: Controller pages during active pairing listener
- **WHEN** a controller attempts a connection to the host adapter MAC while interactive controller pairing listener mode is active in STATE_OFF
- **THEN** the system records the incoming controller details, requests the remote device name if not yet cached, and immediately rejects the HCI connection request

#### Scenario: System transitions out of STATE_OFF
- **WHEN** the system leaves STATE_OFF into STATE_BOOTING or STATE_ON
- **THEN** the system disables page scan to allow the controller to connect to the real host adapter

### Requirement: Bluetooth Low Energy Scanning
The system SHALL continuously scan for advertising packets matching any controller in the bound controllers list (up to 8) on BLE targets.

#### Scenario: BLE advertisement detected
- **WHEN** an advertisement matching any controller present in the bound controllers list is received while in STATE_OFF
- **THEN** the system updates the last-seen timestamp and initiates power-on if cooldown has expired

### Requirement: Pairing Mode Wake Inhibitor
The system SHALL inhibit controller wake triggers and network wake triggers while interactive controller pairing mode is active, even when a connection request is received from an already-bound controller.

#### Scenario: Bound controller activated during pairing mode
- **WHEN** any controller present in the bound controllers list attempts connection while pairing mode is active in STATE_OFF
- **THEN** the system ignores the wake trigger without updating the wake presence timestamp, reports the controller as detected and already bound to the listener session, and remains in STATE_OFF

#### Scenario: Controller saved arms post-pairing cooldown
- **WHEN** a controller is successfully saved via the setup portal
- **THEN** the system terminates pairing mode and arms the wake cooldown inhibitor for 15000 ms to suppress immediate wake from lingering controller paging packets

### Requirement: Dynamic Runtime Controller List Synchronization
The system SHALL synchronize the active in-memory wake detection address list immediately upon any runtime modification of the bound controllers list without requiring a system reboot.

#### Scenario: Controller added at runtime
- **WHEN** a controller is successfully added via the setup portal or REST API while the system is running
- **THEN** the system immediately updates the active wake filter so that wake attempts from the new controller are recognized without rebooting the controller

#### Scenario: Controller removed at runtime
- **WHEN** a controller is removed via the setup portal or REST API while the system is running
- **THEN** the system immediately updates the active wake filter so that wake attempts from the removed controller are rejected without rebooting the controller

