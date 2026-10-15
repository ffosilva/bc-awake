# Spec Delta

## ADDED Requirements

### Requirement: Native Power Button Sensing
When the motherboard power button feature is enabled, the system SHALL configure the power-button line with an internal pull-up while in STATE_OFF, and monitor it for power-on, setup portal, and force power-off triggers.

#### Scenario: Native button tap while OFF
- **WHEN** the motherboard power button feature is enabled, the system is in STATE_OFF, and the power-button line is pulled LOW and released in under 8000 ms
- **THEN** the system asserts PS_ON# LOW, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Native button tap while OFF during pairing mode
- **WHEN** the motherboard power button feature is enabled, the system is in STATE_OFF, pairing mode is active, and the power-button line is pulled LOW and released in under 8000 ms
- **THEN** the system asserts PS_ON# LOW, cancels active pairing mode, starts the boot timer, and transitions to STATE_BOOTING

#### Scenario: Native button hold while OFF
- **WHEN** the motherboard power button feature is enabled, the system is in STATE_OFF, and the power-button line is held LOW continuously for 8000 ms or longer
- **THEN** the system enters WiFi setup portal mode without asserting PS_ON#

#### Scenario: Native button during BOOTING
- **WHEN** the system transitions to STATE_BOOTING
- **THEN** the power-button line is released to high-impedance and presses on the line are ignored

#### Scenario: Native button hold while ON triggers force power off
- **WHEN** the motherboard power button feature is enabled, the system is in STATE_ON, and the power-button line is held LOW continuously for 5000 ms or longer
- **THEN** the system releases PS_ON# to high-impedance, transitions to STATE_OFF, and arms the wake cooldown

#### Scenario: Motherboard power button feature disabled
- **WHEN** the motherboard power button feature is disabled
- **THEN** the system keeps the power-button line in high-impedance INPUT mode while in STATE_OFF, and low signals on the line are ignored
