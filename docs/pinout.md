# Pinout & wiring (ESP32-DevKitC, AI-Thinker ESP32-CAM, ESP32-C3)

Wiring reference for the BC250 power switch on:
- **ESP32-DevKitC (38-pin, ESP32-WROOM-32)**: Classic Bluetooth wake
- **AI-Thinker ESP32-CAM (ESP-32S module)**: Classic Bluetooth wake
- **ESP32-C3 (SuperMini / DevKitM-1)**: Bluetooth Low Energy (BLE) wake

Pin numbers live in [include/board.h](../include/board.h) — conditionally selected by the
active PlatformIO environment (`esp32dev`, `esp32cam`, or `esp32-c3-devkitm-1`).

## GPIO assignment

| Signal | DevKitC | ESP32-CAM | ESP32-C3 | Connects to | Notes |
|--------|---------|-----------|----------|-------------|-------|
| `BUTTON_SENSE` | **GPIO25** | **GPIO15** | **GPIO5** | Momentary switch, terminal A | Internal pull-up; pressed reads LOW |
| `BUTTON_GND` | **GPIO26** | **GND** | **GPIO6** | Momentary switch, terminal B | DevKitC/C3 drives LOW; CAM wires to header GND |
| `PS_ON_PIN` | **GPIO32** | **GPIO14** | **GPIO4** | ATX `PS_ON#` (green wire) | **Open-drain**, active LOW: LOW = PSU on, released = off |
| `BOARD_SENSE` | **GPIO34** | **GPIO13** | **GPIO3** | BC250 `TPMS1` pin 9 | ~3.3 V when up, 0 when off (DevKitC/C3: ADC with hysteresis; CAM: digital INPUT_PULLDOWN) |
| `PWR_BTN_PIN` | **GPIO27** | **GPIO2** | **GPIO7** | Motherboard `PWR_BTN#` (optional) | Direct wire, idles ~3.4 V. High-Z input; pulled LOW only while mirroring the button or sending a 300 ms shutdown pulse; never driven HIGH |
| `FLASH_LED` | — | **GPIO4** | — | Onboard white LED | Held permanently LOW so the onboard flash LED never illuminates |
| `5V` / `VIN` | **5V** | **5V** | **5V** | PSU `+5VSB` | Permanent power for the ESP |
| `GND` | **GND** | **GND** | **GND** | PSU GND + board GND | Common ground |

### Pin rationale

**On ESP32 DevKitC:**
- **GPIO27** is the optional `PWR_BTN_PIN`: a general GPIO with no strapping or boot-time function.
- **GPIO34 is ADC1** (ADC1_CH6). ADC2 can't be read while WiFi is running (setup portal), so the analog board-sense must be on ADC1. GPIO34 is input-only.
- **GPIO32** is high-impedance at reset, so `PS_ON#` stays released (PSU off) while the ESP32 boots. Not a strapping pin.
- **GPIO25/26** are general GPIOs with no boot-time function.

**On ESP32-C3 (SuperMini / DevKitM-1):**
- **GPIO7** is the optional `PWR_BTN_PIN`: free on both boards, not a strapping pin (the C3 strapping pins are GPIO2, 8 and 9).
- **GPIO3 is ADC1** (ADC1_CH3). Read as an analog voltage with factory eFuse calibration.
- **GPIO4** is open-drain for `PS_ON#`.
- **GPIO5/6** connect to the momentary switch.
- USB-CDC is built-in (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`).

**On AI-Thinker ESP32-CAM:**
- Almost all GPIOs are taken by the camera, SD card socket, flash LED, and PSRAM.
- **GPIO13** is used for `BOARD_SENSE` as a digital input with internal pull-down (`INPUT_PULLDOWN`). All ADC1 pins (GPIO32–39) are internally dedicated to the camera or red LED. ADC2 cannot be sampled when Bluetooth or WiFi is active, and GPIO13 is shared with the SD DAT3 line which floats HIGH unless pulled down.
- **GPIO14** is free from strapping functions and used as open-drain for `PS_ON#`.
- **GPIO15** has an internal pull-up and is used for `BUTTON_SENSE`. Terminal B connects directly to the header `GND` pin (saving a GPIO pin).
- **GPIO2** is the optional `PWR_BTN_PIN` (header-exposed; wired to SD DAT0, unused by this firmware). It is a strapping pin, but its value is ignored during a normal boot (GPIO0 high). See [`PWR_BTN#` wiring](#optional-pwr_btn-graceful-shutdown-wiring) for the UART-flashing caveat.
- **GPIO4** is connected to the onboard high-power white flash LED transistor and is clamped permanently LOW at startup so it never illuminates or floats.
- **Pins avoided on ESP32-CAM**:
  - `GPIO12`: Strapping pin MTDI (if pulled HIGH at boot by 3.3 V TPMS1, sets flash VDD to 1.8 V and prevents boot).
  - `GPIO16`: Connected to the external PSRAM chip CS line.
  - `GPIO0`: Boot mode strapping pin (must be LOW to flash, HIGH to boot).

## DevKitC header (left column, USB at the bottom)

![ESP32-DevKitC Pinout](images/esp32-devkitc-pinout.png)

All four signals and power are on the **left** header (the side with `EN`, `VP`, `VN`):

```
   ┌────────────────────────────┐
   │ 3V3                        │
   │ EN                         │
   │ IO36 (VP)                  │
   │ IO39 (VN)                  │
   │ IO34   ◄── TPMS1 pin 9     │  ADC1, input-only
   │ IO35                       │
   │ IO32   ──► PS_ON#          │  open-drain, active LOW
   │ IO33                       │
   │ IO25   ◄── switch A        │  pull-up
   │ IO26   ──► switch B        │  driven LOW
   │ IO27   ──► PWR_BTN# (opt.)  │  high-Z, pulled LOW only
   │ IO14                       │
   │ IO12   (strapping, avoid)  │
   │ GND    ◄── common ground   │
   │ IO13                       │
   │ SD2 / SD3 / CMD  (flash)   │  leave unconnected
   │ 5V     ◄── PSU +5VSB       │
   └────────────────────────────┘
```

> Header order can differ slightly between DevKitC clones. **Go by the silkscreen labels**
> (`IO25`, `IO26`, `IO32`, `IO34`, `5V`, `GND`), not the physical position.

## AI-Thinker ESP32-CAM header

![AI-Thinker ESP32-CAM Pinout](images/esp32cam-pinout.png)

The ESP32-CAM has two 8-pin headers (antenna at top):

```
         ┌─────────────────────────┐
         │       [ Antenna ]       │
         │                         │
         │ IO12               3V3  │
         │ IO13 ◄─ TPMS1 pin 9 GND │
         │ IO15 ◄─ switch A   IO0  │  (IO0 to GND to flash)
         │ IO14 ──► PS_ON#    U0TX │
         │ IO2  ──► PWR_BTN#  U0RX │  (optional, see below)
         │ IO4  (flash LED)   IO16 │  (PSRAM)
         │ GND  ◄─ common/swB GND  │
         │ 5V   ◄─ PSU +5VSB  VCC  │
         └─────────────────────────┘
```

> [!TIP]
> **ESP32-CAM Anti-Oscillation Circuit & Carrier Board**: To avoid 3.3V AMS1117 brownout resets on radio bursts and overcome the 47 kΩ MicroSD pull-up on GPIO13, adding bulk decoupling, a 4.7 kΩ pull-down, and a MOSFET buffer is strongly recommended. See [AI-Thinker ESP32-CAM hardware resilience circuit](#ai-thinker-esp32-cam-hardware-resilience-circuit-anti-oscillation), the [ESP32-CAM Prototype Carrier Board Guide](esp32cam-carrier-pcb.md), and the comprehensive [AI-Thinker ESP32-CAM Bill of Materials (BOM)](bom.md).

## ESP32-C3 SuperMini header (USB-C)

![ESP32-C3 SuperMini Pinout](images/esp32-c3-supermini-pinout.png)

The ultra-compact ESP32-C3 SuperMini board features a built-in USB-C port and two 8-pin headers (USB-C at top):

```
           ┌────────────────────────┐
           │       [ USB-C ]        │
           │                        │
     IO5   │ ◄─ switch A (pull-up)  │ 5V   ◄─ PSU +5VSB
     IO6   │ ──► switch B (driven L)│ GND  ◄─ common ground
     IO7   │ ──► PWR_BTN# (optional)│ 3V3
     IO8   │ (blue LED)             │ IO4  ──► PS_ON# (open-drain)
     IO9   │ (BOOT button)          │ IO3  ◄─ TPMS1 pin 9 (ADC1)
    IO10   │                        │ IO2
    IO20   │                        │ IO1
    IO21   │                        │ IO0
           └────────────────────────┘
```

## ESP32-C3 DevKitM-1 header

![ESP32-C3-DevKitM-1 Pinout](images/esp32-c3-devkitm-1-pinout.png)

The ESP32-C3-DevKitM-1 development board has two 15-pin headers (antenna at top, USB at bottom):

```
           ┌─────────────────────────────┐
           │         [ Antenna ]         │
           │                             │
       GND │                             │ GND
       3V3 │                             │ IO21 (TX)
       3V3 │                             │ IO20 (RX)
       IO2 │                             │ GND
IO3 (ADC1) │ ◄─ TPMS1 pin 9              │ IO9  (BOOT)
       GND │                             │ IO8  (RGB)
       RST │                             │ GND
       GND │                             │ IO7  ──► PWR_BTN# (optional)
       IO0 │                             │ IO6  ──► switch B (driven LOW)
       IO1 │                             │ IO5  ◄── switch A (pull-up)
      IO10 │                             │ IO4  ──► PS_ON# (open-drain)
       GND │                             │ GND
        5V │ ◄─ PSU +5VSB                │ IO18 (USB D-)
        5V │                             │ IO19 (USB D+)
       GND │ ◄─ common ground            │ GND
           │          [ USB ]            │
           └─────────────────────────────┘
```

## Connections

```
 PSU +5VSB ────────────────────────► ESP32 5V / VIN
 PSU GND ──────────────────────────► ESP32 GND  (common ground, also to the BC250)

 Switch terminal A ────────────────► DevKitC: GPIO25 | CAM: GPIO15 | C3: GPIO5  (pull-up)
 Switch terminal B ────────────────► DevKitC: GPIO26 | CAM: GND    | C3: GPIO6  (driven LOW / GND)

 PSU PS_ON# (green) ──[optional buffer, see below]── DevKitC: GPIO32 | CAM: GPIO14 | C3: GPIO4  (open-drain, active LOW)

 BC250 TPMS1 pin 9 ────────────────► DevKitC: GPIO34 | CAM: GPIO13 | C3: GPIO3  (3.3V board sense)

  Motherboard PWR_BTN# (optional) ──► DevKitC: GPIO27 | CAM: GPIO2  | C3: GPIO7  (direct wire, high-Z / LOW only)
```

### Optional `PWR_BTN#` (graceful shutdown) wiring

Connect the motherboard's front-panel power-button pin (the one that idles high, **not**
the ground pin) directly to `PWR_BTN_PIN`. Ground is already shared through the ATX
connector. Measure it first: on the BC250 board it idles at **~3.4 V**, just under the
ESP32's ~3.6 V limit. If your board's pin idles at 5 V, do not wire it directly.

```
 Motherboard FPANEL                      ESP32
 +------------------+                    +----------------------+
 | PWR_BTN# (~3.4V) |------------------->| PWR_BTN_PIN (INPUT)  |
 | GND              |--[common GND]------| GND                  |
 +------------------+                    +----------------------+
```

- **Never driven HIGH.** The pin is a high-impedance input and is pulled LOW only while the
  feature is enabled and the machine is ON: either mirroring your button press or for the
  300 ms portal "Shut down now" pulse.
- **Long press and backstop.** Mirroring lets the motherboard apply its own short-press and
  long-press behavior. The firmware's 5 s hold-to-cut-PSU still releases `PS_ON#`
  regardless, so power is cut even if the board has no hardware override.
- **No timeout.** If the OS ignores the shutdown, nothing happens automatically.
- **ESP32-CAM GPIO2 / UART flashing.** The board's pull-up holds GPIO2 HIGH, which can stop
  the ESP32 entering download mode. **Unplug the `PWR_BTN#` wire before flashing over UART**;
  OTA updates are unaffected. A normal boot is unaffected because GPIO2 is ignored when
  GPIO0 is high, but verify once that the reset-time pull-down does not register as a press.

### `PS_ON#` 5 V caution (recommended buffer)

`PS_ON#` idles at ~5 V, pulled up **inside the PSU**. Open-drain firmware means the
ESP32 never *drives* 5 V, but the pin still *sees* 5 V whenever it's released, and
that is above the ESP32 GPIO absolute maximum (VDD + 0.3 V ≈ 3.6 V). It tends to work,
but it stresses the pad over time. Recommended fix — use a small low-side switch so the
ESP32 only ever drives a 3.3 V gate:

```
                                  PSU PS_ON# (green)
                                         │
                                         ├───────────  (PSU pull-up to ~5V is internal)
                                         │
                                       ┌─┴─┐
                    1 kΩ                │ D │   2N7000 / BSS138 N-MOSFET
 GPIO32 ──[ 1 kΩ ]──────────────────── │ G │   (or any small NPN: 1 kΩ base resistor,
                                        │ S │    emitter to GND)
                                       └─┬─┘
                                         │
                                        GND

 + 100 kΩ from gate to GND so PS_ON# stays released while the ESP32 boots.
```

The buffer **inverts** the signal: GPIO32 HIGH turns the MOSFET on and pulls `PS_ON#`
LOW (PSU on). If you add it, flip the logic levels in
[include/board.h](../include/board.h):

```cpp
const int PS_ON_ASSERT  = HIGH;  // MOSFET on  -> PS_ON# pulled LOW -> PSU on
const int PS_ON_RELEASE = LOW;   // MOSFET off -> PSU pull-up wins  -> PSU off
```

and change `pinMode(PS_ON_PIN, OUTPUT_OPEN_DRAIN)` to `OUTPUT` in
[src/main.cpp](../src/main.cpp) and [src/portal.cpp](../src/portal.cpp). If you skip the
buffer, at minimum put a ~1 kΩ series resistor between `PS_ON#` and GPIO32 to limit
the clamp-diode current.

## AI-Thinker ESP32-CAM hardware resilience circuit (anti-oscillation)

The **AI-Thinker ESP32-CAM** is particularly prone to infinite power-cycle reboot loops due to three hardware characteristics:
1. **LDO brownout on radio bursts**: The onboard AMS1117-3.3 linear regulator has a high dropout voltage (~1.2 V). Wi-Fi and Bluetooth transmission bursts draw 400–500 mA pulses. Over typical ATX harness wires, lead resistance and inductance cause the 5 V rail to dip below 4.5 V, collapsing the 3.3 V rail and triggering an ESP32 brownout reset (`ESP_RST_BROWNOUT`).
2. **MicroSD pull-up on GPIO 13 (`BOARD_SENSE`)**: GPIO 13 is shared with MicroSD `DAT3` and has a physical **47 kΩ SMD pull-up to 3.3 V** on the camera PCB. When TPMS1 goes low or disconnects, this pull-up fights the ESP32's internal ~45 kΩ pull-down, biasing GPIO 13 at **~1.61 V** (the digital meta-stable threshold). Noise easily trips this as `HIGH`, tricking the ESP into adopting `STATE_ON` right after a brownout reset.
3. **GPIO 14 over-voltage & float**: `PS_ON#` idles at ~5 V, stressing the GPIO 14 pad and floating high-Z during resets.

To make an ESP32-CAM build rock-solid for 24/7 long-term operation, add this passive conditioning circuit (on a small perfboard or inline harness). For a complete KiCad single-layer PCB layout and step-by-step perfboard wiring diagram, see the [AI-Thinker ESP32-CAM Prototype Carrier Board Guide](esp32cam-carrier-pcb.md).

### ESP32-CAM Resilience Schematic

```
========================================================================================
 1. POWER CONDITIONING (Dual-Rail Bulk Decoupling)
========================================================================================

 ATX +5VSB (Pin 9) ─────────┬───────────────────────────────► ESP32-CAM "5V"
                            │
                          + │
                       [ 470 µF ]  Electrolytic (10V–16V, Low ESR)
                          - │
                            ├───────[ 100 nF ] Ceramic (0.1 µF)
                            │
 ATX GND (Pin 15/17) ───────┴───────┬───────────────────────► ESP32-CAM "GND"
                                    │
                                  - │
                               [ 47 µF - 100 µF ] Electrolytic or Tantalum (6.3V+)
                                  + │
                                    ├───────[ 100 nF ] Ceramic
                                    │
 ESP32-CAM "3V3" ───────────────────┴───────────────────────  (Stabilizes LDO output)


========================================================================================
 2. INPUT CONDITIONING (TPMS1 / BOARD_SENSE -> GPIO 13)
========================================================================================

 Motherboard TPMS1 (Pin 9)
         │
         │
         └───[ R1: 1 kΩ ]───┬───────────────────────────────► ESP32-CAM "IO13"
                            │                                  (BOARD_SENSE)
                          [ R2 ] 4.7 kΩ (Pull-Down to GND)
                            │
                          [ C1 ] 100 nF Ceramic (0.1 µF)
                            │
                           GND


========================================================================================
 3. OUTPUT BUFFER & ISOLATION (PS_ON# -> GPIO 14)
========================================================================================

                                            ATX PS_ON# (Green Wire, Pin 16)
                                                   │
                                                   ├────────  (PSU ~5V internal pull-up)
                                                   │
                                                 ┌─┴─┐
                             R3: 1 kΩ            │ D │  2N7000 / BSS138 N-MOSFET
 ESP32-CAM "IO14" ───────────[ 1 kΩ ]──────┬─────┤ G │
 (PS_ON_PIN)                               │     │ S │
                                         [ C2 ]  └─┬─┘
                                         [ R4 ]    │
                                           │      GND
                                          GND
      (R4 = 100 kΩ pull-down to GND; C2 = 100 nF ceramic gate snubber)
========================================================================================
```

### Bill of Materials (BOM)

For an exhaustive catalog including footprints, ratings, sourcing links, programming accessories, and machine-readable CSV export, see the dedicated [docs/bom.md](bom.md) and [docs/esp32cam-carrier-pcb.md](esp32cam-carrier-pcb.md).

| Designator | Component | Value / Rating | Purpose |
|------------|-----------|----------------|---------|
| `C_PWR1` | Electrolytic Capacitor | **470 µF**, 10V–16V (Low ESR) | Bulk reservoir on `5V`/VIN rail to absorb Wi-Fi/BT transmission surges |
| `C_PWR2` | Ceramic Capacitor | **100 nF** (0.1 µF), 25V X7R | High-frequency RF decoupling across `5V` and `GND` |
| `C_PWR3` | Electrolytic / Tantalum | **47 µF – 100 µF**, 6.3V+ (Optional) | Buffers AMS1117 3.3V rail output against transient dips |
| `C_PWR4` | Ceramic Capacitor | **100 nF** (0.1 µF), 25V X7R (Optional) | High-frequency RF decoupling across `3V3` and `GND` |
| `R1` | Metal Film Resistor | **1 kΩ**, 1/4W | Series current limit & input filter resistor for TPMS1 |
| `R2` | Metal Film Resistor | **4.7 kΩ**, 1/4W | Pull-down to GND: divides onboard 47 kΩ pull-up down to ~0.30 V when off |
| `C1` | Ceramic Capacitor | **100 nF** (0.1 µF), 25V X7R | Low-pass filter ($f_c \approx 1.6\text{ kHz}$) with R1 against VRM noise |
| `Q1` | N-Channel MOSFET | **2N7000** (TO-92) or **BSS138** (SOT-23) | Low-side buffer isolates GPIO 14 from PSU 5 V line |
| `R3` | Metal Film Resistor | **1 kΩ**, 1/4W | Gate series protection resistor |
| `R4` | Metal Film Resistor | **100 kΩ**, 1/4W | Gate bleed-down to GND: keeps MOSFET off when ESP32 resets |
| `C2` | Ceramic Capacitor | **100 nF** (0.1 µF), 25V X7R | Gate snubber: absorbs bootloader microsecond output glitches |

### Wiring & Assembly Notes

- **Logic Inversion with MOSFET**: The N-MOSFET inverts logic. In [include/board.h](../include/board.h), set `PS_ON_ASSERT = HIGH` and `PS_ON_RELEASE = LOW`, and change `OUTPUT_OPEN_DRAIN` to `OUTPUT` in `initHardware()`.
- **Wire Gauge**: Use 20–22 AWG wire for `5V` and `GND`. Avoid thin 28–30 AWG breadboard jumpers for power, as their resistance causes voltage drop under load.
- **Capacitor Placement**: Solder the 470 µF electrolytic capacitor as close as physically possible to the ESP32-CAM's header pins.
- **Never put raw capacitors directly on `PS_ON#`**: Always filter the MOSFET gate (`R3`/`R4`/`C2`), never the green wire directly to GND. Placing a large capacitor directly on `PS_ON#` discharges violently through the transistor and forces the PSU on during cold boot.

## Connector pinouts

**ATX 24-pin main connector** — tap three pins:

```
               +3.3V ─┤  1 │ 13 ├─ +3.3V
               +3.3V ─┤  2 │ 14 ├─ −12V
                 GND ─┤  3 │ 15 ├─ GND
                 +5V ─┤  4 │ 16 ├─ PS_ON#   ◄── GPIO32 (green, open-drain, active LOW)
                 GND ─┤  5 │ 17 ├─ GND      ◄── ESP GND (any GND pin works)
                 +5V ─┤  6 │ 18 ├─ GND
                 GND ─┤  7 │ 19 ├─ GND
              PWR_OK ─┤  8 │ 20 ├─ (RSVD)
ESP 5V/VIN ◄── +5VSB ─┤  9 │ 21 ├─ +5V
                +12V ─┤ 10 │ 22 ├─ +5V
                +12V ─┤ 11 │ 23 ├─ +5V
               +3.3V ─┤ 12 │ 24 ├─ GND
```

**TPMS1 header** — single pin for board-power sense:

```
   PCICLK ─┤  1   2 ├─ GND
    FRAME ─┤  3   4 ├─ SMB_CLK_MAIN
  PCIRST# ─┤  5   6 ├─ SMB_DATA_MAIN
     LAD3 ─┤  7   8 ├─ LAD2
       3V ─┤  9  10 ├─ LAD1      ◄── pin 9 (3V) = board-on sense ──► GPIO34
     LAD0 ─┤ 11  12 ├─ GND
          ─┤     14 ├─ S_PWRDWN#
     3VSB ─┤ 15  16 ├─ SERIRQ#
      GND ─┤ 17  18 ├─ GND
```

Pin 9 is the only TPMS1 pin used: it reads ~3.3 V when the board is powered and 0 V
when off. No ground wire is needed from this header — the ESP32 already shares ground
with the board through the ATX connector.

`TPMS1` is a higher-impedance signal that hovers near the logic threshold, so it is read
as an **analog** voltage (16× oversampled, with hysteresis) rather than a digital pin.

## Powering and flashing

- Power the ESP32 from `+5VSB` into the `5V` (VIN) pin so it runs whether the machine is
  on or off. Don't connect USB and `+5VSB` at the same time without a diode/ideal-diode
  between them, or the two 5 V sources will fight.
- The DevKitC's USB-UART bridge (CP210x/CH340) is used for flashing and the serial log
  (115200 baud). No `BOOT`/`EN` button handling is needed on most boards; if upload
  fails to connect, hold `BOOT` while it says `Connecting...`.
- For the **ESP32-CAM**, use an external USB-to-UART adapter (or ESP32-CAM-MB daughterboard)
  connected to `U0TX` and `U0RX`. Jumper **`IO0` to `GND`** before powering on to enter
  flashing mode; disconnect `IO0` from `GND` and reset the module to boot the firmware.
