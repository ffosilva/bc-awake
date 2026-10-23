## ADDED Requirements

### Requirement: ESP32-CAM Status LED Pin
Every AI-Thinker ESP32-CAM target, with or without the carrier board, SHALL use the module's onboard red LED on GPIO33, which is active LOW, as the status LED.

#### Scenario: ESP32-CAM status LED pin
- **WHEN** compiled with BOARD_ESP32CAM (esp32cam or esp32cam-carrier)
- **THEN** the system routes the status LED to GPIO33 as an output with inverted polarity, where driving the pin LOW lights the LED and HIGH turns it off

#### Scenario: Other targets
- **WHEN** compiled for esp32dev or esp32-c3
- **THEN** no status LED pin is defined
