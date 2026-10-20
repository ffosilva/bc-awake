# Spec Delta

## MODIFIED Requirements

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

### Requirement: Push-Button Power On
The system SHALL trigger a system power-on sequence when the momentary power button is tapped while in STATE_OFF in normal operation mode, but SHALL NOT trigger power-on while running in setup mode.

#### Scenario: Short button press while off
- **WHEN** the momentary button is pressed and released in under 8000 ms while in STATE_OFF and setup mode is not active
- **THEN** the system asserts PS_ON# LOW, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Short button press while off during pairing mode
- **WHEN** the momentary button is pressed and released in under 8000 ms while in setup mode or controller pairing mode
- **THEN** the system ignores the power-on tap, keeps PS_ON# released to high-impedance, and maintains STATE_OFF

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

## REMOVED Requirements

### Requirement: Programmatic Power On
**Reason**: The web portal runs exclusively in Setup Mode, and the host machine is prevented from powering on during Setup Mode.
**Migration**: Power on the machine in normal operation mode using the physical power button, a bound Bluetooth controller, or a Wake-on-LAN magic packet.

### Requirement: Programmatic Force Power Off
**Reason**: Wireless radios and the web portal are completely disabled while the host machine is in `STATE_BOOTING` or `STATE_ON`.
**Migration**: Hold the physical power button for 5 seconds while the machine is on to force a power cut.

### Requirement: Graceful Shutdown Pulse
**Reason**: Wireless radios and the web portal are completely disabled while the host machine is in `STATE_ON`.
**Migration**: Enable the motherboard power button setting in Setup Mode and tap the physical power button while the machine is on to trigger a clean OS shutdown.
