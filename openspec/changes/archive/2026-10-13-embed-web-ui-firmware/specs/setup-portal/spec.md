# Spec Delta: Setup Portal

## MODIFIED Requirements

### Requirement: Captive Network and Web Serving
The system SHALL broadcast an open WiFi SoftAP named 'BC250 Switch Setup' and serve the embedded setup interface from firmware memory.

#### Scenario: Client connects to setup network
- **WHEN** a client associates with the BC250 Switch Setup access point and queries any DNS address
- **THEN** the captive DNS server resolves all queries to 192.168.4.1 and the web server serves embedded gzipped index.html directly from firmware memory with Content-Encoding: gzip without requiring a mounted filesystem
