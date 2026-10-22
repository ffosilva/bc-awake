# Power Control Specification

## Purpose
Manages the ATX power supply state machine, push-button inputs, boot watchdog, and board shutdown detection for the BC250 system.

## Requirements

### Requirement: Power State Management
The power controller SHALL maintain three mutually exclusive operational states: STATE_OFF, STATE_BOOTING, and STATE_ON, driving the ATX PS_ON# signal accordingly without glitching PS_ON# LOW during startup, leaving PS_ON# asserted in STATE_OFF due to peripheral-manager rejection, or re-triggering power-on from residual capacitor voltage after a brownout reset, and SHALL completely disable all wireless radios during STATE_BOOTING and STATE_ON in normal operation mode.

#### Scenario: Initial boot with board unpowered
- **WHEN** the ESP32 starts up in normal operation mode and the board sense signal does not remain continuously HIGH across the 250 ms startup qualification window
- **THEN** the system pre-loads the hardware GPIO output latch for PS_ON_PIN to PS_ON_RELEASE via the ESP-IDF GPIO driver before enabling OUTPUT_OPEN_DRAIN, synchronizes the Arduino GPIO HAL to PS_ON_RELEASE after pinMode, enters STATE_OFF, keeps the PS_ON# line released to high-impedance, and starts configured wake listeners

#### Scenario: Initial boot with board already energized
- **WHEN** the ESP32 starts up in normal operation mode from a non-brownout reset and the board sense signal remains continuously HIGH across the full 250 ms startup qualification window
- **THEN** the system enters STATE_ON, asserts PS_ON# LOW to synchronize with the running board, and keeps all wireless radios disabled

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

### Requirement: Push-Button Power On
The system SHALL trigger a system power-on sequence when the momentary power button is tapped while in STATE_OFF in normal operation mode, but SHALL NOT trigger power-on while running in setup mode.

#### Scenario: Short button press while off
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF and setup mode is not active
- **THEN** the system asserts PS_ON# LOW, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Short button press while off during pairing mode
- **WHEN** the momentary button is pressed and released in under 8000 ms while in setup mode or controller pairing mode
- **THEN** the system ignores the power-on tap, keeps PS_ON# released to high-impedance, and maintains STATE_OFF

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

### Requirement: Board-Following Shutdown Detection
The system SHALL detect when the host motherboard shuts down independently and automatically release the power supply.

#### Scenario: Host operating system powers down
- **WHEN** the board sense signal remains continuously LOW for 1500 ms while in STATE_ON
- **THEN** the system releases PS_ON# to high-impedance and transitions to STATE_OFF

### Requirement: Force Power Off
The system SHALL force an immediate power cut when the user holds the momentary power button during system operation.

#### Scenario: Long button hold while powered on
- **WHEN** the button is held continuously for 5000 ms or longer while in STATE_ON
- **THEN** the system releases PS_ON# to high-impedance, transitions to STATE_OFF, and arms the wake cooldown

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

### Requirement: Graceful Shutdown Completion
After the power-button line has been driven by button mirroring while in STATE_ON, the system SHALL rely on the existing board-following shutdown detection to release PS_ON#, and SHALL NOT force a hard power-off on a timeout.

#### Scenario: Operating system or motherboard shuts the board down
- **WHEN** the power-button line has been driven and the board sense signal remains LOW for 1500 ms while in STATE_ON
- **THEN** the system releases PS_ON# to high-impedance and transitions to STATE_OFF

#### Scenario: Operating system does not shut down
- **WHEN** the power-button line has been driven by a short button press and the board sense signal stays HIGH
- **THEN** the system remains in STATE_ON with PS_ON# asserted and takes no automatic action

#### Scenario: Force power off remains the backstop
- **WHEN** graceful shutdown is enabled and the button is held continuously for 5000 ms or longer while in STATE_ON
- **THEN** the system releases the power-button line, releases PS_ON# to high-impedance, transitions to STATE_OFF, and arms the wake cooldown, exactly as without graceful shutdown
