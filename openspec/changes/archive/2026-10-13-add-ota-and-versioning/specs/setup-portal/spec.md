# Spec Delta: Setup Portal

## ADDED Requirements

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
