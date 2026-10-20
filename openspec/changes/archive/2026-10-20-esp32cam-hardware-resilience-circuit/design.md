# Design

## Context

See `proposal.md` for motivation. The AI-Thinker ESP32-CAM utilizes an AMS1117-3.3 LDO with high dropout voltage (~1.2 V), lacks onboard bulk capacitance, and exposes pin-sharing quirks:
- `GPIO 13` (`BOARD_SENSE`) is tied to the MicroSD socket `DAT3` line with an onboard 47 kΩ pull-up to 3.3V, causing the pin to float at ~1.61 V against the internal ~45 kΩ pull-down when TPMS1 is unasserted.
- `GPIO 14` (`PS_ON_PIN`) connects to ATX `PS_ON#`, exposing the 3.3V output driver to 5V open-circuit pull-up voltage and releasing to high-impedance during resets.
- Wire inductance over the ATX 24-pin harness causes 5V rail dips during Wi-Fi / Bluetooth transmissions.

This design defines a modular hardware resilience conditioning circuit consisting of power rail decoupling, input termination/filtering, and output isolation buffering.

## Goals / Non-Goals

**Goals:**
- Provide complete schematics, component values, and assembly instructions for ESP32-CAM power decoupling, input conditioning, and output isolation.
- Eliminate brownout resets caused by radio bursts over high-impedance ATX power leads.
- Eliminate input chattering and false "board on" detection caused by the 47 kΩ MicroSD pull-up on `GPIO 13`.
- Prevent boot-time relay chatter and protect `GPIO 14` from 5V over-voltage.

**Non-Goals:**
- Mandating external buffer circuits on ESP32 DevKitC or ESP32-C3 targets where ADC1 sensing and pin routing are unaffected by MicroSD pull-ups.
- Redesigning the custom PCB; all circuits are implementable via protoboard, perfboard, or point-to-point inline harness wiring.

## Decisions

### 1. Dual-Rail Bulk and High-Frequency Power Decoupling
- **Decision**:
  - Place a **470 µF** (10V–16V, low-ESR) electrolytic capacitor and a **100 nF** (0.1 µF) ceramic capacitor directly across the ESP32-CAM `5V` and `GND` header pins.
  - Optionally place a **47 µF to 100 µF** electrolytic/tantalum capacitor and **100 nF** ceramic capacitor across the ESP32-CAM `3V3` and `GND` pins.
- **Rationale**:
  - Wi-Fi transmission bursts draw 400–500 mA transient pulses with rise times under 1 µs. Over 20–40 cm ATX harness wires, wire inductance ($L \approx 1\text{ nH/mm}$) causes a voltage drop $\Delta V = L \frac{di}{dt}$ that pulls 5V below the AMS1117 dropout threshold (~4.5V).
  - Bulk 470 µF capacitance supplies local transient energy, while 100 nF ceramic capacitors short high-frequency RF noise (>10 MHz) that electrolytics cannot filter due to their parasitic equivalent series inductance (ESL).
- **Alternatives considered**:
  - *Small 10 µF ceramic capacitor*: Rejected because 10 µF is insufficient to sustain the 3.3V rail through sustained 20 ms radio calibration bursts.

### 2. External 4.7 kΩ Pull-Down and RC Low-Pass Filter on `GPIO 13`
- **Decision**:
  - Connect a **4.7 kΩ** resistor from `GPIO 13` to `GND`.
  - Connect a **1 kΩ** series current-limiting resistor between motherboard `TPMS1 pin 9` and `GPIO 13`.
  - Connect a **100 nF** ceramic capacitor between `GPIO 13` and `GND`.
- **Rationale**:
  - The onboard 47 kΩ pull-up forms a voltage divider with the external 4.7 kΩ resistor:
    $$V_{off} = 3.3\text{ V} \times \frac{4.7\text{ k}\Omega}{47\text{ k}\Omega + 4.7\text{ k}\Omega} \approx 0.30\text{ V}$$
    This drops the unasserted level firmly into the valid logic LOW range ($V_{IL} < 0.8\text{ V}$), preventing threshold hover.
  - The 1 kΩ series resistor and 100 nF capacitor form a low-pass filter with cutoff frequency $f_c \approx 1.6\text{ kHz}$ ($\tau = 0.1\text{ ms}$), rejecting GPU/VRM switching spikes and mains hum.
- **Alternatives considered**:
  - *Internal pull-down only*: Rejected because internal pull-down (~45 kΩ) leaves the voltage divider at 1.61 V.
  - *Removing the surface-mount 47 kΩ resistor with hot air*: Rejected to avoid requiring micro-soldering tools.

### 3. N-Channel MOSFET Buffer with Gate RC Snubber on `GPIO 14`
- **Decision**:
  - Use a small N-channel MOSFET (2N7000, BS170, or BSS138) in a low-side open-drain configuration.
  - Drive the gate through a **1 kΩ** series resistor from `GPIO 14`.
  - Tie a **100 kΩ** pull-down resistor from Gate to `GND`.
  - Add a **100 nF** ceramic capacitor from Gate to `GND`.
- **Rationale**:
  - Isolates the ESP32 GPIO from the PSU 5V pull-up.
  - The 100 kΩ pull-down ensures the MOSFET remains completely OFF (PSU released) while the ESP32 reboots or floats its GPIOs.
  - The 100 nF gate capacitor suppresses microsecond bootloader toggles.
- **Alternatives considered**:
  - *Placing a large capacitor directly across PS_ON# and GND*: Strictly rejected because open-drain discharge creates destructive current spikes ($>5\text{ A}$) and causes cold-boot false turn-ons while the cap charges.

## Risks / Trade-offs

- **[MOSFET buffer inverts logic levels]** -> When adding the MOSFET buffer, `PS_ON_ASSERT` must be configured to `HIGH` and `PS_ON_RELEASE` to `LOW` in `include/board.h`. Mitigation: Document both direct-wire and buffered configurations with clear conditional instructions.
- **[Component count]** -> Requires 5 passive components (2 resistors, 2 capacitors, 1 FET). Mitigation: All components are common, hobbyist-accessible through-hole or module parts easily mounted on a small stripboard or inline heat-shrink assembly.
