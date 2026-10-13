# Spec Delta: Controller Wake

## ADDED Requirements

### Requirement: Pairing Mode Wake Inhibitor
The system SHALL inhibit controller wake triggers and network wake triggers while interactive controller pairing mode is active, even when a connection request is received from an already-bound controller.

#### Scenario: Bound controller activated during pairing mode
- **WHEN** any controller present in the bound controllers list attempts connection while pairing mode is active in STATE_OFF
- **THEN** the system ignores the wake trigger without updating the wake presence timestamp, reports the controller as detected and already bound to the listener session, and remains in STATE_OFF

#### Scenario: Controller saved arms post-pairing cooldown
- **WHEN** a controller is successfully saved via the setup portal
- **THEN** the system terminates pairing mode and arms the wake cooldown inhibitor for 15000 ms to suppress immediate wake from lingering controller paging packets
