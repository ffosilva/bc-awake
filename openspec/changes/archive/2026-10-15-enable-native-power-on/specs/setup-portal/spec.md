# Spec Delta

## MODIFIED Requirements

### Requirement: Power Tab User Interface
The portal web interface SHALL provide a Power tab displaying live power state telemetry, ATX power supply controls with an in-page confirmed force-cut dialog, and optional motherboard power button controls.

#### Scenario: Displaying Power tab
- **WHEN** the user selects the Power tab
- **THEN** the interface renders real-time power state telemetry, ATX power supply controls (Power On and Force Cut Power), and motherboard power button controls

#### Scenario: Triggering power on from Power tab
- **WHEN** the user clicks Power On while the machine is in STATE_OFF
- **THEN** the interface sends an authenticated POST request to `/api/power/on` and updates the power indicator upon success

#### Scenario: Opening force cut confirmation dialog
- **WHEN** the user clicks Force Cut Power while the machine is in STATE_ON or STATE_BOOTING
- **THEN** the interface opens an in-page modal dialog explaining the risk of data corruption and prompting for confirmation

#### Scenario: Confirming force cut power
- **WHEN** the user confirms the force cut modal dialog
- **THEN** the interface sends an authenticated POST request to `/api/power/off`, closes the dialog, and updates the power indicator upon success

#### Scenario: Canceling force cut power
- **WHEN** the user cancels the force cut modal dialog
- **THEN** the modal dialog closes without dispatching an API request

#### Scenario: Motherboard power button pulse action
- **WHEN** motherboard power button support is enabled and the board is in STATE_ON
- **THEN** the interface displays an active "Shut Down OS" button that sends an authenticated POST request to `/api/shutdown` on click

#### Scenario: Motherboard power button action unavailable
- **WHEN** motherboard power button support is disabled or the board is not in STATE_ON
- **THEN** the "Shut Down OS" button is disabled or accompanied by an explanatory message

#### Scenario: Motherboard power button feature description
- **WHEN** the Power tab displays the motherboard power button toggle
- **THEN** the accompanying description indicates that enabling the feature supports both powering on from the native power button when off and clean OS shutdown when on
