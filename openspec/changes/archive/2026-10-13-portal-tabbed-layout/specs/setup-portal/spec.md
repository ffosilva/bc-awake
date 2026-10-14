# Spec Delta

## MODIFIED Requirements

### Requirement: Controller Binding and Provisioning Completion
The system SHALL persist bound controller configurations and adapter addresses and reboot into normal operation when finished.

#### Scenario: Binding controller MAC address
- **WHEN** an authenticated POST request is sent to /api/select with a valid colon-separated MAC address
- **THEN** the system saves the address to NVS and returns HTTP 200 OK

#### Scenario: Adding bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/add with a valid MAC address and non-empty friendly name
- **THEN** the system appends the controller to the bound list in NVS and returns HTTP 200 OK

#### Scenario: Removing bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/remove with a bound MAC address
- **THEN** the system deletes the controller from the bound list in NVS and returns HTTP 200 OK

#### Scenario: Renaming bound controller
- **WHEN** an authenticated POST request is sent to /api/controllers/rename with a bound MAC address and new friendly name
- **THEN** the system updates the friendly name in NVS and returns HTTP 200 OK

#### Scenario: Exceeding maximum controller capacity
- **WHEN** a client attempts to add a 9th controller to a list already containing 8 controllers
- **THEN** the system rejects the request with HTTP status 400 Bad Request

#### Scenario: Completing setup
- **WHEN** an authenticated POST request is sent to /api/finish on a device with a portal password set
- **THEN** the system clears forceSetup, returns HTTP 200 OK, and reboots into normal mode after 800 ms, regardless of whether an adapter address, bound controllers, or network wake settings are configured

#### Scenario: Completing setup without authentication
- **WHEN** a POST request is sent to /api/finish without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized and does not clear forceSetup

### Requirement: Persistent Web Portal and mDNS Service
The system SHALL keep the HTTP web portal and REST API online in normal operation mode over Wi-Fi Station connectivity and SHALL advertise its presence via mDNS.

#### Scenario: Web server active in normal station mode
- **WHEN** the system connects to a configured Wi-Fi network in normal operation mode
- **THEN** the HTTP web server starts on port 80 and serves the portal UI and REST endpoints on the Station IP address

#### Scenario: mDNS hostname advertised
- **WHEN** the system establishes Wi-Fi Station connectivity
- **THEN** the system registers the configured hostname (defaulting to bc250-switch.local) via mDNS and configures the DHCP client hostname

#### Scenario: Reporting host power state in status API
- **WHEN** a client performs a GET request to /api/status
- **THEN** the JSON response includes a string field boardState reflecting the current power state ("OFF", "BOOTING", or "ON")

#### Scenario: Web UI adapts to powered-on state
- **WHEN** the setup portal web interface is loaded while the board is in STATE_ON
- **THEN** the interface marks the Network tab settings as read-only and disables the interactive pairing listener action in the Bluetooth tab

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"

## ADDED Requirements

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
- **THEN** the Bluetooth tab contains the adapter address (Classic targets), bound controllers or BLE picker, and the ESP32-CAM flash option when applicable; the Network tab contains Wake-on-LAN, Wi-Fi, hostname, and BC250 Ethernet MAC settings; the System tab contains firmware information and update, password change, and the exit setup action

### Requirement: Non-Blocking Configuration Warnings
The portal web interface SHALL indicate missing optional configuration as non-blocking warnings rather than preventing navigation or completion.

#### Scenario: Adapter address missing
- **WHEN** the Bluetooth tab is shown on a Classic target with no adapter address saved
- **THEN** the interface displays a notice that Bluetooth wake is inactive until an adapter address is set, and the Bluetooth tab label shows a warning indicator

#### Scenario: No controllers bound
- **WHEN** the Bluetooth tab is shown with no bound controllers
- **THEN** the interface displays a notice that no controller can wake the host, without disabling any other tab or the exit setup action

### Requirement: Bluetooth Adapter Address Editing
On Classic Bluetooth targets, the Bluetooth tab SHALL allow viewing and saving the host adapter address independently of other settings, and SHALL disable interactive pairing until an adapter address is saved.

#### Scenario: Saving adapter address
- **WHEN** the user enters a valid colon-delimited MAC address in the adapter field and saves it
- **THEN** the interface sends an authenticated POST request to /api/host and confirms success inline

#### Scenario: Invalid adapter address
- **WHEN** the user enters a value that is not a valid colon-delimited MAC address
- **THEN** the save action is disabled and no request is sent

#### Scenario: Interactive pairing without adapter address
- **WHEN** no adapter address is saved on a Classic target
- **THEN** the Add controller action is disabled with an inline hint to set the adapter address first, while manual controller MAC entry remains available

#### Scenario: BLE target
- **WHEN** the Bluetooth tab is shown on a BLE target
- **THEN** no adapter address field is displayed and the BLE device picker is shown instead

### Requirement: Independent Section Saving
Each portal tab SHALL persist its own settings through its own save actions, and exiting setup SHALL NOT implicitly save settings from any tab.

#### Scenario: Saving network settings
- **WHEN** the user saves settings in the Network tab
- **THEN** only network wake settings are submitted to /api/network and the result is reported within the Network tab

#### Scenario: Exiting with unsaved edits
- **WHEN** the user triggers exit setup while a tab has unsaved edits
- **THEN** the interface does not submit those edits; only /api/finish is called

### Requirement: Setup Mode Reporting
The status API SHALL report whether the device is running in SoftAP setup mode, and the portal web interface SHALL show the exit setup action only in that mode.

#### Scenario: Status in setup mode
- **WHEN** a client performs a GET request to /api/status while the device runs the SoftAP setup portal
- **THEN** the JSON response includes setupMode set to true

#### Scenario: Status in normal mode
- **WHEN** a client performs a GET request to /api/status while the device runs in normal operation over Wi-Fi Station
- **THEN** the JSON response includes setupMode set to false

#### Scenario: Exit action visibility
- **WHEN** the System tab is displayed
- **THEN** the "Exit setup mode & reboot" action is visible only if setupMode is true

#### Scenario: Exiting setup from the System tab
- **WHEN** the user triggers "Exit setup mode & reboot" and /api/finish returns HTTP 200
- **THEN** the interface displays a rebooting notice

### Requirement: Portal Password Change
The System tab SHALL allow an authenticated user to change the portal password, requiring the new password to be entered twice.

#### Scenario: Mismatched confirmation
- **WHEN** the new password and its confirmation differ
- **THEN** the interface shows an error and sends no request

#### Scenario: Password too short
- **WHEN** the new password has fewer than 6 characters
- **THEN** the interface shows an error and sends no request

#### Scenario: Successful password change
- **WHEN** the new password and confirmation match, are 6 or more characters, and the authenticated POST to /api/password succeeds
- **THEN** the interface replaces its session token with the token returned by the server, clears the fields, and confirms the change, so subsequent requests remain authorized
