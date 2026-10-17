# Spec Delta

## MODIFIED Requirements

### Requirement: Boot Watchdog
The system SHALL monitor the board sense line during boot, require the board sense signal to remain continuously HIGH for 250 ms before confirming active power, and abort power-up if the motherboard fails to signal active power within the boot timeout.

#### Scenario: Board boots successfully
- **WHEN** the board sense signal transitions to HIGH and remains continuously HIGH for 250 ms within 10000 ms after entering STATE_BOOTING
- **THEN** the system transitions from STATE_BOOTING to STATE_ON and maintains PS_ON# asserted

#### Scenario: Transient spike during boot ignored
- **WHEN** the board sense signal pulses HIGH for less than 250 ms while in STATE_BOOTING
- **THEN** the system remains in STATE_BOOTING and does not prematurely transition to STATE_ON

#### Scenario: Board boot timeout failure
- **WHEN** the board sense signal fails to reach a stable 250 ms HIGH within 10000 ms in STATE_BOOTING
- **THEN** the system releases PS_ON# to high-impedance and transitions to STATE_OFF

### Requirement: Board Sense Signal Conditioning
The system SHALL filter and condition the board sense signal against high-impedance line noise and transient spikes across both analog and digital hardware targets.

#### Scenario: Noise rejection via oversampled hysteresis
- **WHEN** analog board sense voltage is read on ADC-equipped hardware targets
- **THEN** the system averages 16 consecutive ADC samples and applies hysteresis thresholds of 2000 mV for HIGH and 800 mV for LOW

#### Scenario: Noise rejection on digital board sense target
- **WHEN** digital board sense level is read on the ESP32-CAM target
- **THEN** the system samples GPIO13 across 16 consecutive reads and only updates the raw sense state when all 16 samples agree on HIGH or LOW, holding the previous state otherwise
