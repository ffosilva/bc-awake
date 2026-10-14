# Setup Portal Specification

## Purpose
Provides a captive WiFi access point and web interface for device configuration, controller discovery, and provisioning.

## Requirements

### Requirement: Setup Portal Activation
The system SHALL launch a WiFi SoftAP setup portal when explicitly requested by user input or configuration flag.

#### Scenario: Button held for setup while off
- **WHEN** the momentary button is held continuously for 8000 ms or longer while in STATE_OFF
- **THEN** the system sets forceSetup to true and restarts into setup mode

#### Scenario: Setup mode initiated via serial command
- **WHEN** an 's' or 'S' character is received on the serial interface
- **THEN** the system sets forceSetup to true and restarts into setup mode

#### Scenario: Button held to exit setup portal
- **WHEN** the momentary button is held continuously for 8000 ms while running in setup mode
- **THEN** the system clears forceSetup and restarts into normal operation mode

### Requirement: Captive Network and Web Serving
The system SHALL broadcast an open WiFi SoftAP named 'BC250_<SUFFIX>', where <SUFFIX> is the 6-character uppercase hexadecimal representation of the last 3 bytes of the factory Wi-Fi MAC address, and serve the embedded setup interface from firmware memory without triggering captive network detection.

#### Scenario: Client connects to setup network
- **WHEN** a client associates with the BC250 SoftAP (matching 'BC250_<SUFFIX>' using the board's last 3 MAC bytes)
- **THEN** the system does not intercept DNS queries or trigger OS captive portal detection, and the web server serves embedded gzipped index.html directly from firmware memory with Content-Encoding: gzip at root path '/' without requiring a mounted filesystem

#### Scenario: Unknown route handling
- **WHEN** an unmapped HTTP path outside the REST API and root endpoints is requested
- **THEN** the web server returns HTTP status 404 Not Found rather than redirecting or serving the setup interface

### Requirement: Portal Password Protection
The system SHALL require password authentication for configuration changes and issue single-session bearer tokens.

#### Scenario: First-time password creation
- **WHEN** a POST request is made to /api/password with a password of 6 or more characters and no password was previously set
- **THEN** the system stores the SHA-256 hash in NVS and returns a session token

#### Scenario: Authenticating with existing password
- **WHEN** a POST request is made to /api/login with the correct password
- **THEN** the system returns a valid session token for subsequent protected API requests

#### Scenario: Unauthorized access attempt
- **WHEN** a protected endpoint is called without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized

### Requirement: Wireless Device Discovery
The system SHALL discover nearby Bluetooth controllers and report them through the device discovery API.

#### Scenario: Client requests discovered devices
- **WHEN** an authenticated GET request is received at /api/devices
- **THEN** the system returns a JSON list of discovered devices containing MAC address, friendly name, RSSI, and paged status

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

### Requirement: Captive Portal User Interface Resilience
The setup web interface SHALL perform all user input entry, editing, and confirmations via in-page DOM elements without relying on browser-native dialog prompts (`window.prompt`, `window.alert`, `window.confirm`).

#### Scenario: Activating inline rename for bound controller
- **WHEN** a user clicks the rename button for a bound controller
- **THEN** the row switches to an inline editing state displaying an input pre-filled with the current friendly name alongside Save and Cancel actions, without invoking browser-native prompt dialogs

#### Scenario: Saving inline controller rename
- **WHEN** a user submits the inline edit with a valid non-empty name
- **THEN** the interface sends an authenticated POST request to /api/controllers/rename and updates the bound controller list with the new name upon success

#### Scenario: Canceling inline controller rename
- **WHEN** a user clicks the cancel action during an active inline edit
- **THEN** the row reverts to displaying the current name and standard action buttons without dispatching an API request

### Requirement: Network Wake Configuration
The system SHALL provide portal UI controls and API endpoints to configure network wake settings, credentials, and target hardware addresses while off, and SHALL enforce read-only access while the host is on.

#### Scenario: Updating network wake settings with passphrase
- **WHEN** an authenticated POST request is received at /api/network with valid wolEnabled, wifiSsid, wifiPassword, colon-delimited bc250Mac, and optional hostname while in STATE_OFF
- **THEN** the system persists the configuration to NVS and returns HTTP 200 OK

#### Scenario: Updating network wake settings for an open network
- **WHEN** an authenticated POST request is received at /api/network with an empty wifiPassword and a non-empty wifiSsid while in STATE_OFF
- **THEN** the system persists the configuration with an empty password and returns HTTP 200 OK

#### Scenario: Disabling network wake
- **WHEN** an authenticated POST request is received at /api/network with wolEnabled set to false while in STATE_OFF
- **THEN** the system updates wolEnabled in NVS to false and returns HTTP 200 OK

#### Scenario: Inspecting network wake status
- **WHEN** an authenticated GET request is received at /api/status
- **THEN** the response includes wolEnabled, wifiSsid, bc250Mac, hostname, and an indicator of whether a Wi-Fi password is saved

#### Scenario: Attempting to update network wake settings while host machine is on
- **WHEN** an authenticated POST request is received at /api/network while the system is in STATE_BOOTING or STATE_ON
- **THEN** the system rejects the update with HTTP status 403 Forbidden without modifying stored network configuration

#### Scenario: Validating hostname format
- **WHEN** an authenticated POST request is received at /api/network with a hostname containing characters outside [a-z0-9-] or length exceeding 32 characters
- **THEN** the system rejects the update with HTTP status 400 Bad Request

### Requirement: Firmware Version and Target Reporting
The setup portal status API SHALL report the compiled firmware version and target hardware identifier, and the portal web interface SHALL display them.

#### Scenario: Status request returns version and target
- **WHEN** a client performs a GET request to /api/status
- **THEN** the JSON response includes string fields version and target reflecting the compiled firmware metadata

#### Scenario: Portal UI displays version badge
- **WHEN** the portal web interface is loaded in a browser
- **THEN** the header displays the active firmware version and target identifier

### Requirement: Over-the-Air Firmware Upload Endpoint
The setup portal SHALL provide an authenticated HTTP POST endpoint accepting firmware binary streams to update the inactive flash partition and reboot upon completion, and SHALL reject update attempts while the host machine is powered on.

#### Scenario: Successful firmware update stream
- **WHEN** an authenticated POST request with valid X-Auth-Token is sent to /api/update containing a valid firmware binary payload while the host machine is in STATE_OFF
- **THEN** the system pauses Bluetooth discovery, writes the binary to the inactive OTA bank, marks the partition bootable, returns HTTP 200 with {"ok":true}, and reboots after 800 ms

#### Scenario: Firmware update rejected while host is powered on
- **WHEN** an authenticated POST request is sent to /api/update while the host machine is in STATE_ON or STATE_BOOTING
- **THEN** the system rejects the update with HTTP status 409 Conflict, aborts the stream before modifying flash memory, and leaves the host machine unaffected

#### Scenario: Unauthorized firmware update attempt
- **WHEN** a POST request is sent to /api/update without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized without writing to flash

#### Scenario: Firmware write error or truncated payload
- **WHEN** the uploaded stream fails validation or encounters a flash write error
- **THEN** the system aborts the update, returns HTTP status 500 with an error description, and remains running without rebooting into the corrupted partition

### Requirement: Setup Portal Firmware Update User Interface
The portal web interface SHALL provide an in-page firmware update interface with file selection and upload progress tracking, and SHALL disable firmware updates when the host machine is powered on.

#### Scenario: User selects and uploads firmware binary
- **WHEN** the user selects a .bin file and submits the update form in the portal UI while the host machine is in STATE_OFF
- **THEN** the UI transmits the file to /api/update using XMLHttpRequest with progress percentage displayed in real time

#### Scenario: Firmware update completes in UI
- **WHEN** the upload finishes successfully and receives HTTP 200
- **THEN** the UI displays a rebooting notice and waits for the board to restart

#### Scenario: Firmware update controls disabled while host is powered on
- **WHEN** the setup portal web interface loads or refreshes status while the host machine is in STATE_ON or STATE_BOOTING
- **THEN** the interface disables the firmware file input and update button, and displays an informative notice that firmware updates are only available while the machine is powered off

#### Scenario: Firmware update controls enabled when host is powered off
- **WHEN** the setup portal web interface status updates and the host machine is in STATE_OFF
- **THEN** the interface enables the firmware file input and clears any power lock notice

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
- **THEN** the interface marks the Network tab settings as read-only, disables the interactive pairing listener action in the Bluetooth tab, and disables firmware update controls in the System tab

#### Scenario: Board power status indicator displays powered on
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "ON"
- **THEN** the UI header displays a green indicator light icon reflecting "POWERED ON"

#### Scenario: Board power status indicator displays powered off
- **WHEN** the setup portal web interface loads or refreshes status while boardState is "OFF"
- **THEN** the UI header displays a red indicator light icon reflecting "POWERED OFF"

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

### Requirement: Portal Power Actions Endpoints
The portal SHALL provide authenticated HTTP POST endpoints at `/api/power/on` and `/api/power/off` to trigger system power-on and immediate power cut.

#### Scenario: Power on requested while in STATE_OFF
- **WHEN** an authenticated POST request is sent to `/api/power/on` while in STATE_OFF
- **THEN** the system cancels any active controller pairing mode, starts the boot timer, asserts PS_ON# LOW, transitions to STATE_BOOTING, and returns HTTP 200 with `{"ok":true}`

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

### Requirement: Portal Password Form Submission and Credential Management
The portal web interface SHALL present the initial password creation and unlock screen within a semantic form that supports keyboard submission via the Enter key and provides standard credential attributes enabling web browser password managers to offer password saving without requiring a username.

#### Scenario: Submitting unlock via Enter key
- **WHEN** a user enters their password into the unlock input field and presses the Enter key
- **THEN** the interface dispatches the form submission, validates the input, and sends an authentication request to `/api/login` without requiring the user to click the Unlock button

#### Scenario: Submitting first-time password creation via Enter key
- **WHEN** a user enters matching passwords into the creation inputs and presses the Enter key in either field
- **THEN** the interface dispatches the form submission and sends the setup request to `/api/password` without requiring the user to click the Continue button

#### Scenario: Browser credential manager password saving compatibility
- **WHEN** a user successfully unlocks or creates a portal password
- **THEN** the password form provides semantic autocomplete attributes (`current-password` when unlocking, `new-password` when creating) and signals successful form submission to the browser to prompt the browser credential manager to save the password without requiring a username
