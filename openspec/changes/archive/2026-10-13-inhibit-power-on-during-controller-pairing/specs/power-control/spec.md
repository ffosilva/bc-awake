# Spec Delta: Power Control

## MODIFIED Requirements

### Requirement: Push-Button Power On
The system SHALL trigger a system power-on sequence when the momentary power button is tapped while in STATE_OFF, including when controller pairing mode is active.

#### Scenario: Short button press while off
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF
- **THEN** the system asserts PS_ON# LOW, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Short button press while off during pairing mode
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF and controller pairing mode is active
- **THEN** the system asserts PS_ON# LOW, cancels any active pairing mode, starts the boot timer, and transitions to STATE_BOOTING
