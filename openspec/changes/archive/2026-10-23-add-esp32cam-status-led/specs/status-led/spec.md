## ADDED Requirements

### Requirement: Status LED Patterns
On targets that define a status LED, the system SHALL drive the LED from firmware to reflect the power state and setup mode, without blocking the main loop.

#### Scenario: Machine powered on
- **WHEN** the system is in STATE_ON and not in setup mode
- **THEN** the status LED is solidly lit

#### Scenario: Machine booting
- **WHEN** the system is in STATE_BOOTING and not in setup mode
- **THEN** the status LED blinks at about 4 Hz

#### Scenario: Machine off
- **WHEN** the system is in STATE_OFF and not in setup mode
- **THEN** the status LED is completely off

### Requirement: Setup Mode Indication
While the setup portal is active, the system SHALL blink the status LED at 1 Hz, overriding the power-state pattern, and SHALL keep blocking power-on.

#### Scenario: Setup mode active
- **WHEN** the system is in setup mode
- **THEN** the status LED blinks at 1 Hz with an approximately 50% duty cycle

#### Scenario: Short press in setup mode
- **WHEN** the momentary button is tapped while in setup mode
- **THEN** the machine is not powered on and the status LED continues blinking at 1 Hz

#### Scenario: Leaving setup mode
- **WHEN** the button is held for 8000 ms or longer while OFF in setup mode
- **THEN** the system leaves setup mode and restarts into normal operation, where the status LED follows the power-state pattern (off in STATE_OFF)

### Requirement: Status LED Safe Defaults
The system SHALL leave the status LED off from the moment the firmware initializes it until a pattern is applied, and SHALL NOT affect any other pin or feature.

#### Scenario: Target without status LED
- **WHEN** compiled for a target that defines no status LED
- **THEN** no GPIO is configured for a status LED and behavior is unchanged
