# Proposal

## Why

AI-Thinker ESP32-CAM modules deployed in BC Awake are susceptible to infinite power-cycle oscillation loops and spurious resets caused by physical hardware limitations:
1. High-dropout AMS1117-3.3 linear regulators on the ESP32-CAM suffer 3.3V rail collapse and brownout resets during Wi-Fi and Bluetooth transmission bursts over high-impedance ATX `+5VSB` wiring.
2. An onboard 47 kΩ pull-up resistor on the ESP32-CAM PCB (MicroSD `DAT3` line shared with `GPIO 13` / `BOARD_SENSE`) creates a voltage divider against the internal pull-down, biasing the pin at ~1.61 V in the digital meta-stable threshold when the motherboard is powered off.
3. Direct wiring of `GPIO 14` to ATX `PS_ON#` exposes the 3.3V GPIO pad to 5V pull-up voltages and makes the power supply switch vulnerable to boot-time high-impedance glitches.

While firmware mitigation exists, hardware-level RC filtering, external pull-down termination, and output buffer isolation are required to guarantee long-term operational resilience.

## What Changes

- Specify hardware power rail decoupling requirements for ESP32-CAM targets: 470 µF low-ESR electrolytic and 100 nF ceramic capacitors across `5V` and `GND`, plus optional 47–100 µF decoupling across `3V3` and `GND`.
- Specify an external RC low-pass filter with pull-down termination on `GPIO 13` (`BOARD_SENSE`): a 1 kΩ series resistor, 4.7 kΩ pull-down to GND (overcoming the 47 kΩ onboard pull-up), and 100 nF ceramic capacitor to GND.
- Specify an N-channel MOSFET (2N7000 / BSS138) low-side buffer with gate snubber RC network (1 kΩ series, 100 kΩ gate pull-down, 100 nF capacitor) for `GPIO 14` (`PS_ON#`) to isolate the ESP32 from 5V and prevent boot-state relay flutter.
- Update `docs/pinout.md` and `README.md` with explicit schematics, ASCII wiring diagrams, component values, and bill of materials (BOM) for the ESP32-CAM target.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `hardware-targets`: Define recommended external circuit filtering, input pull-down termination, and output isolation buffer specifications for the AI-Thinker ESP32-CAM hardware target.

## Impact

- **Documentation**: `docs/pinout.md`, `README.md`.
- **Hardware**: External breadboard/protoboard circuit additions for ESP32-CAM installations.
- **Software**: Optional logic inversion configuration in `include/board.h` (`PS_ON_ASSERT` / `PS_ON_RELEASE`) when adopting the inverting MOSFET buffer.
