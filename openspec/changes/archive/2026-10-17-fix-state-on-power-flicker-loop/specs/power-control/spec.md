# Spec Delta

## MODIFIED Requirements

### Requirement: Power State Management
The power controller SHALL maintain three mutually exclusive operational states: STATE_OFF, STATE_BOOTING, and STATE_ON, driving the ATX PS_ON# signal accordingly without glitching PS_ON# LOW during startup or re-triggering power-on from residual capacitor voltage after a brownout reset.

#### Scenario: Initial boot with board unpowered
- **WHEN** the ESP32 starts up and the board sense signal does not remain continuously HIGH across the 250 ms startup qualification window
- **THEN** the system pre-loads PS_ON_PIN to PS_ON_RELEASE before enabling OUTPUT_OPEN_DRAIN, enters STATE_OFF, and keeps the PS_ON# line released to high-impedance

#### Scenario: Initial boot with board already energized
- **WHEN** the ESP32 starts up from a non-brownout reset and the board sense signal remains continuously HIGH across the full 250 ms startup qualification window
- **THEN** the system enters STATE_ON, asserts PS_ON# LOW to synchronize with the running board, and staggers radio initialization so ATX PSU current and RF calibration do not coincide

#### Scenario: Startup recovery after brownout reset
- **WHEN** the ESP32 starts up with reset reason ESP_RST_BROWNOUT
- **THEN** the system keeps PS_ON# released in STATE_OFF, waits for residual rail capacitance to settle before sampling board sense or starting wireless radios, and does not assert PS_ON# from decaying TPMS1 voltage

### Requirement: Board Sense Signal Conditioning
The system SHALL filter and condition the board sense signal against high-impedance line noise and transient spikes across both analog and digital hardware targets.

#### Scenario: Noise rejection via oversampled hysteresis
- **WHEN** analog board sense voltage is read on ADC-equipped hardware targets
- **THEN** the system averages 16 consecutive ADC samples and applies hysteresis thresholds of 2000 mV for HIGH and 800 mV for LOW

#### Scenario: Noise rejection on digital board sense target
- **WHEN** digital board sense level is read on the ESP32-CAM target
- **THEN** the system samples GPIO13 across 16 consecutive reads spaced by 200 microseconds (~3.2 ms total window) and only updates the raw sense state when all 16 samples agree on HIGH or LOW, holding the previous state otherwise
