# Spec Delta

## ADDED Requirements

### Requirement: Power Button Mirroring
When graceful shutdown is enabled, the system SHALL mirror the momentary power button to the motherboard power-button line while in STATE_ON, pulling the line LOW while the button is pressed and releasing it to high-impedance when released, without ever driving it HIGH.

#### Scenario: Short press while ON
- **WHEN** graceful shutdown is enabled, the system is in STATE_ON, and the momentary button is pressed and released
- **THEN** the system pulls the power-button line LOW for the debounced duration of the press, releases it to high-impedance on release, and leaves PS_ON# asserted

#### Scenario: Long press while ON
- **WHEN** graceful shutdown is enabled and the button is held while in STATE_ON
- **THEN** the power-button line stays LOW for as long as the button is held, up to the force power-off threshold, so the motherboard applies its own short-press and long-press behavior

#### Scenario: Press while OFF or BOOTING
- **WHEN** the momentary button is pressed while in STATE_OFF or STATE_BOOTING
- **THEN** the system does not drive the power-button line and existing power-on and setup behavior is unchanged

#### Scenario: Press began while OFF
- **WHEN** a press began while in STATE_OFF and the state becomes STATE_ON before the button is released
- **THEN** the system does not drive the power-button line for that press

#### Scenario: Line released when leaving ON
- **WHEN** the system leaves STATE_ON while the power-button line is being driven LOW
- **THEN** the system releases the line to high-impedance immediately

#### Scenario: Graceful shutdown disabled
- **WHEN** graceful shutdown is disabled
- **THEN** the system never configures the power-button line as an output and a press while ON has no effect other than the existing force power-off hold

### Requirement: Graceful Shutdown Pulse
When graceful shutdown is enabled, the system SHALL support a programmatic shutdown request that pulls the power-button line LOW for 300 ms and then releases it to high-impedance.

#### Scenario: Shutdown requested through the API while ON
- **WHEN** graceful shutdown is enabled and an authenticated shutdown request is received while in STATE_ON
- **THEN** the system pulls the power-button line LOW for 300 ms without blocking other processing, releases it to high-impedance, and leaves PS_ON# asserted

#### Scenario: Repeated shutdown request
- **WHEN** a shutdown request is received after a previous pulse completed and the system remains in STATE_ON
- **THEN** the system sends another 300 ms pulse, like another physical press

#### Scenario: Shutdown request during an active pulse
- **WHEN** a shutdown request is received while a previous pulse is still active
- **THEN** the system does not extend or queue a second pulse

#### Scenario: Shutdown request while not ON
- **WHEN** a shutdown request is received while in STATE_OFF or STATE_BOOTING
- **THEN** the system does not drive the power-button line

### Requirement: Graceful Shutdown Completion
After the power-button line has been driven, either by mirroring or by a pulse, the system SHALL rely on the existing board-following shutdown detection to release PS_ON#, and SHALL NOT force a hard power-off on a timeout.

#### Scenario: Operating system or motherboard shuts the board down
- **WHEN** the power-button line has been driven and the board sense signal remains LOW for 1500 ms while in STATE_ON
- **THEN** the system releases PS_ON# to high-impedance and transitions to STATE_OFF

#### Scenario: Operating system does not shut down
- **WHEN** a graceful shutdown pulse has been sent and the board sense signal stays HIGH
- **THEN** the system remains in STATE_ON with PS_ON# asserted and takes no automatic action

#### Scenario: Force power off remains the backstop
- **WHEN** graceful shutdown is enabled and the button is held continuously for 5000 ms or longer while in STATE_ON
- **THEN** the system releases the power-button line, releases PS_ON# to high-impedance, transitions to STATE_OFF, and arms the wake cooldown, exactly as without graceful shutdown
