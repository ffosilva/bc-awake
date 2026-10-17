# Spec Delta

## MODIFIED Requirements

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

## REMOVED Requirements

### Requirement: ESP32-CAM Flash LED Wake Pulse
**Reason**: Pulsing the high-current onboard white flash LED (GPIO 4) on the AI-Thinker ESP32-CAM during controller wake causes a ~300 mA 3.3V rail current spike concurrent with ATX PSU turn-on and Bluetooth/Wi-Fi transmissions, leading to brownout resets and power ON/OFF flickering.
**Migration**: Remove all flash LED pulsing and configuration; GPIO 4 is clamped permanently LOW on ESP32-CAM targets.
