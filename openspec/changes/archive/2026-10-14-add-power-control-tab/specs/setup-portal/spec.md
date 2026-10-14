# Spec Delta

## ADDED Requirements

### Requirement: Portal Power Actions Endpoints
The portal SHALL provide authenticated HTTP POST endpoints at `/api/power/on` and `/api/power/off` to trigger system power-on and immediate power cut.

#### Scenario: Power on requested while in STATE_OFF
- **WHEN** an authenticated POST request is sent to `/api/power/on` while in STATE_OFF
- **THEN** the system cancels any active controller pairing session, starts the boot timer, asserts PS_ON# LOW, transitions to STATE_BOOTING, and returns HTTP 200 with `{"ok":true}`

#### Scenario: Power on requested while not in STATE_OFF
- **WHEN** an authenticated POST request is sent to `/api/power/on` while in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the request with HTTP status 409 Conflict without modifying power state

#### Scenario: Force power off requested while running or booting
- **WHEN** an authenticated POST request is sent to `/api/power/off` while in STATE_ON or STATE_BOOTING
- **THEN** the system releases PS_ON# to high-impedance, transitions to STATE_OFF, arms the wake cooldown, and returns HTTP 200 with `{"ok":true}`

#### Scenario: Force power off requested while in STATE_OFF
- **WHEN** an authenticated POST request is sent to `/api/power/off` while in STATE_OFF
- **THEN** the system rejects the request with HTTP status 409 Conflict without modifying power state

#### Scenario: Power action endpoints rejected without authentication
- **WHEN** a POST request is sent to `/api/power/on` or `/api/power/off` without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized without modifying power state

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

## MODIFIED Requirements

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
- **THEN** the Bluetooth tab contains the adapter address (Classic targets), bound controllers or BLE picker, and the ESP32-CAM flash option when applicable; the Network tab contains Wake-on-LAN, Wi-Fi, hostname, and BC250 Ethernet MAC settings; the Power tab contains live power state telemetry, ATX power supply controls (Power On and Force Cut Power with confirmation), and motherboard power button settings and pulse action; the System tab contains firmware information and update, password change, configuration reset, and the exit setup action
