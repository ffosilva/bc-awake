# Spec Delta: Hardware Targets

## ADDED Requirements

### Requirement: Target Hardware Identification Metadata
Each compilation target SHALL define a unique target string corresponding to the hardware platform architecture and board type.

#### Scenario: DevKitC target identification
- **WHEN** compiled for the esp32dev environment
- **THEN** the target string is defined as esp32dev

#### Scenario: ESP32-CAM target identification
- **WHEN** compiled for the esp32cam environment
- **THEN** the target string is defined as esp32cam

#### Scenario: ESP32-C3 target identification
- **WHEN** compiled for the esp32-c3-devkitm-1 environment
- **THEN** the target string is defined as esp32-c3

### Requirement: Dual-Bank OTA Partition Layout
All build environments SHALL utilize a flash partition layout providing dual equal-sized application banks and a reserved filesystem partition.

#### Scenario: Partition allocation on 4 MB flash
- **WHEN** the partition table is parsed by the bootloader
- **THEN** the flash contains app0 (ota_0) and app1 (ota_1) of 1.81 MB (0x1D0000) each, an otadata partition of 8 KB (0x2000), and a SPIFFS partition of 256 KB (0x40000)
