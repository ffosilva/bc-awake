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
- **WHEN** an authenticated POST request is sent to /api/finish on a fully configured device
- **THEN** the system clears forceSetup, returns HTTP 200 OK, and reboots into normal mode after 800 ms

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
The setup portal SHALL provide an authenticated HTTP POST endpoint accepting firmware binary streams to update the inactive flash partition and reboot upon completion.

#### Scenario: Successful firmware update stream
- **WHEN** an authenticated POST request with valid X-Auth-Token is sent to /api/update containing a valid firmware binary payload
- **THEN** the system pauses Bluetooth discovery, writes the binary to the inactive OTA bank, marks the partition bootable, returns HTTP 200 with {"ok":true}, and reboots after 800 ms

#### Scenario: Unauthorized firmware update attempt
- **WHEN** a POST request is sent to /api/update without a valid X-Auth-Token header
- **THEN** the system rejects the request with HTTP status 401 Unauthorized without writing to flash

#### Scenario: Firmware write error or truncated payload
- **WHEN** the uploaded stream fails validation or encounters a flash write error
- **THEN** the system aborts the update, returns HTTP status 500 with an error description, and remains running without rebooting into the corrupted partition

### Requirement: Setup Portal Firmware Update User Interface
The portal web interface SHALL provide an in-page firmware update interface with file selection and upload progress tracking.

#### Scenario: User selects and uploads firmware binary
- **WHEN** the user selects a .bin file and submits the update form in the portal UI
- **THEN** the UI transmits the file to /api/update using XMLHttpRequest with progress percentage displayed in real time

#### Scenario: Firmware update completes in UI
- **WHEN** the upload finishes successfully and receives HTTP 200
- **THEN** the UI displays a rebooting notice and waits for the board to restart

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
- **THEN** the interface marks the Wi-Fi configuration card as read-only and disables the interactive pairing listener modal

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

