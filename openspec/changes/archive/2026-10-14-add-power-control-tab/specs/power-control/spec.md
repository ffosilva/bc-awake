# Spec Delta

## ADDED Requirements

### Requirement: Programmatic Power On
The system SHALL support initiating the system power-on sequence through an authenticated portal request when in STATE_OFF.

#### Scenario: Programmatic power-on while in STATE_OFF
- **WHEN** a programmatic power-on request is received while in STATE_OFF
- **THEN** the system cancels any active controller pairing mode, asserts PS_ON# LOW, records the boot start timestamp, and transitions to STATE_BOOTING

#### Scenario: Programmatic power-on while not in STATE_OFF
- **WHEN** a programmatic power-on request is received while in STATE_BOOTING or STATE_ON
- **THEN** the system ignores the request and preserves the existing power state

### Requirement: Programmatic Force Power Off
The system SHALL support forcing an immediate power cut through an authenticated portal request while running or booting.

#### Scenario: Programmatic force power-off while running or booting
- **WHEN** a programmatic force power-off request is received while in STATE_ON or STATE_BOOTING
- **THEN** the system releases PS_ON# to high-impedance, transitions to STATE_OFF, and arms the wake cooldown

#### Scenario: Programmatic force power-off while in STATE_OFF
- **WHEN** a programmatic force power-off request is received while in STATE_OFF
- **THEN** the system ignores the request and maintains STATE_OFF
