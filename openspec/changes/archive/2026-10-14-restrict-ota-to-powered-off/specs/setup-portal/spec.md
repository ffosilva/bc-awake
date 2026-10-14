# Spec Delta

## MODIFIED Requirements

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
