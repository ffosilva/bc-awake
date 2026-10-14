# Spec Delta

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
- **THEN** the Bluetooth tab contains the adapter address (Classic targets), bound controllers or BLE picker, and the ESP32-CAM flash option when applicable; the Network tab contains Wake-on-LAN, Wi-Fi, hostname, and BC250 Ethernet MAC settings; the System tab contains firmware information and update, password change, configuration reset, and the exit setup action

## ADDED Requirements

### Requirement: Configuration Reset Action and Endpoint
The system SHALL provide an authenticated HTTP POST endpoint at `/api/reset` to clear all persistent configuration and reboot into setup mode, and the System tab SHALL provide an in-page confirmed reset action to invoke it.

#### Scenario: Submitting configuration reset with valid authentication
- **WHEN** an authenticated POST request is sent to `/api/reset` with a valid session token
- **THEN** the system clears all stored configuration to defaults, sets forceSetup to true, returns HTTP 200 with `{"ok":true}`, and reboots the device after 800 ms

#### Scenario: Configuration reset rejected without authentication
- **WHEN** a POST request is sent to `/api/reset` without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized without modifying stored configuration

#### Scenario: Initiating configuration reset from System tab
- **WHEN** the user clicks the reset configuration button in the System tab
- **THEN** the interface reveals an in-page confirmation prompt explaining that all Wi-Fi credentials, bound controllers, and password will be erased, without using browser-native dialogs

#### Scenario: Canceling configuration reset in UI
- **WHEN** the user cancels the in-page reset confirmation prompt
- **THEN** the confirmation prompt collapses and no API request is sent

#### Scenario: Confirming configuration reset in UI
- **WHEN** the user confirms the in-page reset prompt
- **THEN** the interface sends an authenticated POST request to `/api/reset`, displays a notice that the device is resetting and rebooting into setup mode, and disables the action buttons
