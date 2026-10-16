# Spec Delta

## MODIFIED Requirements

### Requirement: REST API Synchronization
The host sync tool SHALL authenticate with the ESP32 REST API, verify host adapter consistency, and synchronize the list of bound controllers using existing endpoints.

#### Scenario: Successful controller list synchronization
- **WHEN** the sync tool connects to http://bc-awake.local and authenticates with the configured password
- **THEN** it updates the host adapter address via /api/host if different, removes controllers no longer paired locally via /api/controllers/remove, and adds newly paired local controllers via /api/controllers/add

#### Scenario: Automatic discovery via UDP broadcast
- **WHEN** url is set to auto in bc250-sync.conf
- **THEN** the sync tool broadcasts BC250_DISCOVER to 255.255.255.255:25025, extracts the target URL from the unicast JSON response, and synchronizes with that endpoint

#### Scenario: Device limit protection
- **WHEN** more than 8 controllers are paired locally on the host
- **THEN** the sync tool synchronizes only the first 8 controllers and logs a warning

#### Scenario: ESP32 switch unreachable
- **WHEN** the host sync tool cannot reach the ESP32 switch over mDNS, UDP discovery, or HTTP
- **THEN** the service logs a connection failure and exits cleanly to be retried on the next timer interval
