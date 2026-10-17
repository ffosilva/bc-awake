# Spec Delta

## MODIFIED Requirements

### Requirement: AI-Thinker ESP32-CAM Configuration
The system SHALL configure dedicated GPIOs, digital board sensing, and reduced Wi-Fi RF transmit power for the AI-Thinker ESP32-CAM target while keeping the onboard flash LED (GPIO 4) permanently disabled (clamped LOW).

#### Scenario: Pin assignment avoiding strapping and camera lines
- **WHEN** compiled with BOARD_ESP32CAM
- **THEN** the system routes BUTTON_SENSE to GPIO15, PS_ON_PIN to GPIO14, BOARD_SENSE to GPIO13 with INPUT_PULLDOWN, configures GPIO4 as an output driven permanently LOW so the onboard flash LED never illuminates, and caps Wi-Fi TX power to 11 dBm across SoftAP and Station modes
