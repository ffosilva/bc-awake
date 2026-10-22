# Spec Delta

## MODIFIED Requirements

### Requirement: Power State Management
The power controller SHALL maintain three mutually exclusive operational states: STATE_OFF, STATE_BOOTING, and STATE_ON, driving the ATX PS_ON# signal accordingly without glitching PS_ON# LOW during startup, leaving PS_ON# asserted in STATE_OFF due to peripheral-manager rejection, or re-triggering power-on from residual capacitor voltage after a brownout reset, and SHALL completely disable all wireless radios during STATE_BOOTING and STATE_ON in normal operation mode.

#### Scenario: Initial boot with board unpowered
- **WHEN** the ESP32 starts up in normal operation mode and the board sense signal does not remain continuously HIGH across the 250 ms startup qualification window
- **THEN** the system pre-loads the hardware GPIO output latch for PS_ON_PIN to PS_ON_RELEASE via the ESP-IDF GPIO driver before configuring the pin mode, enters STATE_OFF, keeps the PS_ON# line released, and starts configured wake listeners

#### Scenario: Initial boot with board already energized
- **WHEN** the ESP32 starts up in normal operation mode from a non-brownout reset and the board sense signal remains continuously HIGH across the full 250 ms startup qualification window
- **THEN** the system enters STATE_ON, asserts PS_ON# to synchronize with the running board, and keeps all wireless radios disabled

#### Scenario: Startup recovery after brownout reset
- **WHEN** the ESP32 starts up with reset reason ESP_RST_BROWNOUT
- **THEN** the system keeps PS_ON# released in STATE_OFF, waits for residual rail capacitance to settle before sampling board sense or starting wireless radios, and does not assert PS_ON# from decaying TPMS1 voltage

#### Scenario: Transition to booting or on disables wireless radios
- **WHEN** the system transitions to STATE_BOOTING or STATE_ON
- **THEN** the system immediately disconnects and disables the Wi-Fi interface, terminates and de-initializes the Bluetooth controller, and stops all radio transmissions

#### Scenario: Transition to off restores wake listeners
- **WHEN** the system transitions from STATE_ON or STATE_BOOTING to STATE_OFF in normal operation mode
- **THEN** the system re-initializes Bluetooth wake listening and re-starts Wi-Fi station WOL listeners if enabled in configuration

#### Scenario: PS_ON# pin drive mode on direct wire targets
- **WHEN** compiled for direct-wire targets (esp32dev, bare esp32cam, or esp32-c3)
- **THEN** PS_ON_PIN is configured as open-drain (OUTPUT_OPEN_DRAIN), active-LOW (PS_ON_ASSERT = LOW, PS_ON_RELEASE = HIGH)

#### Scenario: PS_ON# pin drive mode on carrier board target
- **WHEN** compiled for BOARD_ESP32CAM_CARRIER
- **THEN** PS_ON_PIN is configured as push-pull output (OUTPUT), active-HIGH (PS_ON_ASSERT = HIGH, PS_ON_RELEASE = LOW) to drive the base of the BC547/BC548 transistor

### Requirement: Board Sense Signal Conditioning
The system SHALL filter and condition the board sense signal against high-impedance line noise and transient spikes across both analog and digital hardware targets.

#### Scenario: Noise rejection via oversampled hysteresis
- **WHEN** analog board sense voltage is read on standard ADC-equipped hardware targets (DevKitC, ESP32-C3)
- **THEN** the system averages 16 consecutive ADC samples and applies hysteresis thresholds of 2000 mV for HIGH and 800 mV for LOW

#### Scenario: Noise rejection on digital board sense target
- **WHEN** digital board sense level is read on the bare ESP32-CAM target without carrier board
- **THEN** the system samples GPIO13 across 16 consecutive reads spaced by 200 microseconds (~3.2 ms total window) with INPUT_PULLDOWN and only updates the raw sense state when all 16 samples agree on HIGH or LOW, holding the previous state otherwise

#### Scenario: Noise rejection on ESP32-CAM carrier board target
- **WHEN** board sense is read on the BOARD_ESP32CAM_CARRIER target
- **THEN** the system reads ADC2 with 16 consecutive samples and applies hysteresis thresholds of 900 mV for HIGH and 500 mV for LOW while wireless radios are off, and detaches ADC2 before wireless radios start in STATE_OFF
