# Power Control Specification

## Purpose
Manages the ATX power supply state machine, push-button inputs, boot watchdog, and board shutdown detection for the BC250 system.

## Requirements

### Requirement: Power State Management
The power controller SHALL maintain three mutually exclusive operational states: STATE_OFF, STATE_BOOTING, and STATE_ON, driving the ATX PS_ON# signal accordingly without glitching PS_ON# LOW during startup, leaving PS_ON# asserted in STATE_OFF due to peripheral-manager rejection, or re-triggering power-on from residual capacitor voltage after a brownout reset.

#### Scenario: Initial boot with board unpowered
- **WHEN** the ESP32 starts up and the board sense signal does not remain continuously HIGH across the 250 ms startup qualification window
- **THEN** the system pre-loads the hardware GPIO output latch for PS_ON_PIN to PS_ON_RELEASE via the ESP-IDF GPIO driver before enabling OUTPUT_OPEN_DRAIN, synchronizes the Arduino GPIO HAL to PS_ON_RELEASE after pinMode, enters STATE_OFF, and keeps the PS_ON# line released to high-impedance

#### Scenario: Initial boot with board already energized
- **WHEN** the ESP32 starts up from a non-brownout reset and the board sense signal remains continuously HIGH across the full 250 ms startup qualification window
- **THEN** the system enters STATE_ON, asserts PS_ON# LOW to synchronize with the running board, and staggers radio initialization so ATX PSU current and RF calibration do not coincide

#### Scenario: Startup recovery after brownout reset
- **WHEN** the ESP32 starts up with reset reason ESP_RST_BROWNOUT
- **THEN** the system keeps PS_ON# released in STATE_OFF, waits for residual rail capacitance to settle before sampling board sense or starting wireless radios, and does not assert PS_ON# from decaying TPMS1 voltage

### Requirement: Push-Button Power On
The system SHALL trigger a system power-on sequence when the momentary power button is tapped while in STATE_OFF, including when controller pairing mode is active.

#### Scenario: Short button press while off
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF
- **THEN** the system asserts PS_ON# LOW, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Short button press while off during pairing mode
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF and controller pairing mode is active
- **THEN** the system asserts PS_ON# LOW, cancels any active pairing mode, starts the boot timer, and transitions to STATE_BOOTING

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
- **WHEN** analog board sense voltage is read on ADC-equipped hardware targets
- **THEN** the system averages 16 consecutive ADC samples and applies hysteresis thresholds of 2000 mV for HIGH and 800 mV for LOW

#### Scenario: Noise rejection on digital board sense target
- **WHEN** digital board sense level is read on the ESP32-CAM target
- **THEN** the system samples GPIO13 across 16 consecutive reads spaced by 200 microseconds (~3.2 ms total window) and only updates the raw sense state when all 16 samples agree on HIGH or LOW, holding the previous state otherwise

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

