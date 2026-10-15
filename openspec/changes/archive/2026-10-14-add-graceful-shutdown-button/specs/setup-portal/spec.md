# Spec Delta

## ADDED Requirements

### Requirement: Graceful Shutdown Setting Endpoint
The portal SHALL provide authenticated endpoints to read and persist the optional graceful shutdown setting, which is disabled by default.

#### Scenario: Reporting the setting
- **WHEN** an authenticated client performs a GET request to /api/status
- **THEN** the JSON response includes gracefulShutdown set to the persisted value and the current power state

#### Scenario: Saving the setting
- **WHEN** an authenticated POST request to /api/power carries a valid gracefulShutdown value
- **THEN** the system persists the value immediately and returns HTTP 200 with `{"ok":true}`

#### Scenario: Saving rejected without authentication
- **WHEN** a POST request is sent to /api/power without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized without modifying stored configuration

### Requirement: Graceful Shutdown Action Endpoint
The portal SHALL provide an authenticated POST endpoint at /api/shutdown that triggers a graceful shutdown pulse only when the feature is enabled and the system is in STATE_ON.

#### Scenario: Shutdown while ON with feature enabled
- **WHEN** an authenticated POST request is sent to /api/shutdown while gracefulShutdown is enabled and the system is in STATE_ON
- **THEN** the system sends the 300 ms power-button pulse and returns HTTP 200 with `{"ok":true}`

#### Scenario: Shutdown with feature disabled
- **WHEN** an authenticated POST request is sent to /api/shutdown while gracefulShutdown is disabled
- **THEN** the system returns HTTP 409 and does not pulse the power-button line

#### Scenario: Shutdown while not ON
- **WHEN** an authenticated POST request is sent to /api/shutdown while the system is in STATE_OFF or STATE_BOOTING
- **THEN** the system returns HTTP 409 and does not pulse the power-button line

#### Scenario: Shutdown rejected without authentication
- **WHEN** a POST request is sent to /api/shutdown without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized and does not pulse the power-button line

### Requirement: Graceful Shutdown User Interface
The System tab SHALL present graceful shutdown as an optional feature, with an enable toggle that is off by default and a Shut down action available only while the feature is enabled and the machine is ON.

#### Scenario: Toggle presented as optional
- **WHEN** the System tab is displayed
- **THEN** the interface shows a graceful shutdown toggle labeled as optional, with a note that the motherboard power-button pin must be wired to the target's power-button GPIO

#### Scenario: Enabling the feature
- **WHEN** the user turns the toggle on and saves
- **THEN** the interface sends an authenticated POST request to /api/power and confirms the result inline

#### Scenario: Shut down action visibility
- **WHEN** the System tab is displayed and gracefulShutdown is disabled or the machine is not ON
- **THEN** the Shut down action is disabled or hidden

#### Scenario: Triggering shutdown from the interface
- **WHEN** the user triggers Shut down while it is available
- **THEN** the interface sends an authenticated POST request to /api/shutdown and shows that the shutdown request was sent

## MODIFIED Requirements

### Requirement: Tabbed Portal Layout
After authentication, the portal web interface SHALL present its settings in three tabs — Bluetooth, Network, and System — with the password screen as the only blocking step before them.

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
- **THEN** the Bluetooth tab contains the adapter address (Classic targets), bound controllers or BLE picker, and the ESP32-CAM flash option when applicable; the Network tab contains Wake-on-LAN, Wi-Fi, hostname, and BC250 Ethernet MAC settings; the System tab contains firmware information and update, the optional graceful shutdown toggle and Shut down action, password change, configuration reset, and the exit setup action
