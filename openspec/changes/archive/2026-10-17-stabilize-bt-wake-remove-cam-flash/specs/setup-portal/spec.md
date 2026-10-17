# Spec Delta

## MODIFIED Requirements

### Requirement: Interactive Controller Listener
The system SHALL capture incoming connection requests during setup to provide real-time controller detection while the host machine is in STATE_OFF, and SHALL reject sensing requests while the host is powered on.

#### Scenario: Controller pages during active listener session
- **WHEN** a paired controller pages the host adapter while an authenticated client is querying /api/listener/status and the host is in STATE_OFF
- **THEN** the system returns detected as true along with the controller MAC address, resolved or heuristic friendly name, and whether it is already bound

#### Scenario: Clearing or starting listener session
- **WHEN** an authenticated POST request is sent to /api/listener/start while the host is in STATE_OFF
- **THEN** the system clears the last-captured controller detection slot, arms a 60000 ms pairing timeout, activates pairing wake inhibition, and returns HTTP 200 OK

#### Scenario: Explicit cancellation of listener session
- **WHEN** an authenticated POST request is sent to /api/listener/stop
- **THEN** the system disarms the pairing timeout, deactivates pairing wake inhibition, and returns HTTP 200 OK

#### Scenario: Pairing timeout expires
- **WHEN** 60000 ms elapses following /api/listener/start without a controller being detected
- **THEN** the system automatically terminates the listener session, deactivates pairing wake inhibition, and returns timedOut as true in subsequent /api/listener/status queries

#### Scenario: Rapid flash burst on ESP32-CAM pairing start
- **WHEN** an authenticated POST request is sent to /api/listener/start on an ESP32-CAM target
- **THEN** the system starts the listener session while keeping the onboard flash LED (GPIO 4) LOW without pulsing

#### Scenario: Attempting to start listener while host machine is on
- **WHEN** an authenticated POST request is sent to /api/listener/start while the host is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the request with HTTP status 409 Conflict to prevent radio interference with the host's active Bluetooth adapter

### Requirement: Tabbed Portal Layout
After authentication, the portal web interface SHALL present its settings in four tabs — Bluetooth, Network, Power, and System — with the password screen as the only blocking step before them.

#### Scenario: Landing after login
- **WHEN** the user creates a password or logs in successfully
- **THEN** the interface shows the tabbed view with the Bluetooth tab selected, without requiring any intermediate configuration screen

#### Scenario: Landing with no Bluetooth adapter address saved
- **WHEN** the user logs in on a Classic Bluetooth target with no adapter address saved
- **THEN** the interface shows the tabbed view directly and does not force entry of the adapter address

#### Scenario: Switching tabs
- **WHEN** the user selects a different tab
- **THEN** the interface shows that tab's content without reloading the page or discarding the session token

#### Scenario: Tab contents
- **WHEN** the tabbed view is displayed
- **THEN** the Bluetooth tab contains the adapter address (Classic targets) and bound controllers or BLE picker; the Network tab contains Wake-on-LAN, Wi-Fi, hostname, and BC250 Ethernet MAC settings; the Power tab contains live power state telemetry, ATX power supply controls (Power On and Force Cut Power with confirmation), and motherboard power button settings and pulse action; the System tab contains firmware information and update, password change, configuration reset, and the exit setup action
