# Spec Delta

## MODIFIED Requirements

### Requirement: Captive Network and Web Serving
The system SHALL broadcast an open WiFi SoftAP named 'BC250_<SUFFIX>', where <SUFFIX> is the 6-character uppercase hexadecimal representation of the last 3 bytes of the factory Wi-Fi MAC address, and serve the embedded setup interface from firmware memory without triggering captive network detection.

#### Scenario: Client connects to setup network
- **WHEN** a client associates with the BC250 SoftAP (matching 'BC250_<SUFFIX>' using the board's last 3 MAC bytes)
- **THEN** the system does not intercept DNS queries or trigger OS captive portal detection, and the web server serves embedded gzipped index.html directly from firmware memory with Content-Encoding: gzip at root path '/' without requiring a mounted filesystem

#### Scenario: Unknown route handling
- **WHEN** an unmapped HTTP path outside the REST API and root endpoints is requested
- **THEN** the web server returns HTTP status 404 Not Found rather than redirecting or serving the setup interface
