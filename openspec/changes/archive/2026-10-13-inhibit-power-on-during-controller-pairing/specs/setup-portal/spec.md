# Spec Delta: Setup Portal

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
- **WHEN** an authenticated POST request is sent to /api/listener/start on an ESP32-CAM target with flashOnWake enabled
- **THEN** the system pulses the onboard flash LED (GPIO 4) with 3 rapid flashes of 60 ms on and 60 ms off

#### Scenario: Attempting to start listener while host machine is on
- **WHEN** an authenticated POST request is sent to /api/listener/start while the host is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the request with HTTP status 409 Conflict to prevent radio interference with the host's active Bluetooth adapter

## ADDED Requirements

### Requirement: Setup Portal Interactive Pairing User Interface
The portal web interface SHALL provide real-time visual feedback, live countdown, power-on pause reassurance, and explicit timeout handling during interactive controller pairing.

#### Scenario: Displaying pairing dialog with live countdown and notice
- **WHEN** the user initiates controller addition by clicking Add Controller
- **THEN** the modal displays an informational notice stating that board power-on is paused, states that the case button remains active, and renders a live countdown timer decrementing from 60 seconds

#### Scenario: Handling pairing listener timeout in interface
- **WHEN** the 60-second pairing timer expires without detecting a controller
- **THEN** the modal transitions to a timeout state informing the user that normal wake detection has resumed and provides Try Again and Close actions

#### Scenario: Canceling pairing in interface
- **WHEN** the user clicks the Cancel button in the active pairing modal
- **THEN** the interface sends an authenticated POST request to /api/listener/stop, stops polling, and closes the modal

#### Scenario: Confirming controller saved with cooldown reassurance
- **WHEN** the user saves a detected controller
- **THEN** the interface confirms that the controller was saved and displays a temporary notice that wake detection will resume after 15 seconds
