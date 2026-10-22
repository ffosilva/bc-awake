# AI-Thinker ESP32-CAM Bill of Materials (BOM)

This document provides a comprehensive, itemized Bill of Materials (BOM) for assembling and deploying the **BC Awake** controller using the **AI-Thinker ESP32-CAM** board and its dedicated single-layer carrier board / perfboard prototype.

For the corresponding KiCad layout, schematic, and single-layer track routing, see [docs/esp32cam-carrier-pcb.md](esp32cam-carrier-pcb.md).  
An automated machine-readable export of the carrier board components is generated at [`hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv).

---

## 1. Prototype Carrier Board Components

All components on the carrier board use standard **Through-Hole Technology (THT)** with leads and pad coordinates aligned to a **2.54 mm (0.1 in / 100 mil)** perfboard grid.

| Item | Designator | Qty | Value | Package / Footprint | Ratings & Tolerances | Reference MPN / Search Keywords | Purpose & Circuit Function |
|:----:|:-----------|:---:|:------|:--------------------|:---------------------|:--------------------------------|:---------------------------|
| 1 | `C_PWR1` | 1 | 470 µF | Radial Electrolytic (D8.0mm, P3.5mm) | 16V (or 25V), ±20%, Low-ESR, 105°C | Panasonic `EEU-FR1C471`, Nichicon `UPW1C471MPD` | 5V bulk reservoir capacitor; absorbs Wi-Fi/Bluetooth TX current transients (>300 mA pulses) and prevents ESP32 brownout resets. |
| 2 | `C1`, `C2`, `C_PWR2`, `C_PWR4` | 4 | 100 nF (0.1 µF) | Ceramic Disc / MLCC (D5.0mm, P2.5mm) | 25V or 50V, X7R / Y5V, ±10% or ±20% | Kemet `C315C104K5R5TA`, Vishay `K104K15X7RF5TL2`, Generic 100nF disc | High-frequency bypass and RF noise suppression: `C_PWR2` (5V rail), `C_PWR4` (3.3V rail), `C1` (TPMS1 RC filter), `C2` (BC547 base snubber). |
| 3 | `C_PWR3` | 1 | 47 µF | Radial Electrolytic / Tantalum (D6.3mm, P2.5mm) | 16V, ±20%, 85°C/105°C | Panasonic `ECA-1CHG470`, Nichicon `UVR1C470MDD` | 3.3V bulk reservoir; buffers the ESP32-CAM onboard AMS1117-3.3 regulator output during heavy CPU/radio load. |
| 4 | `J_CAM_L`, `J_CAM_R` | 2 | 1×8 Female Header | 1×8 Female Pin Socket (2.54 mm pitch) | 2.54 mm (100 mil) pitch, 8.5 mm insulator height | Standard 1×8 2.54mm Female Header Socket | Mates with the two 8-pin male headers of the AI-Thinker ESP32-CAM module spaced 22.86 mm (0.9 in) apart. |
| 5 | `J_EXT` | 1 | 1×6 Pin Header / Terminal | 1×6 Pin Header or 2.54mm Screw Terminal Block | 2.54 mm pitch, vertical through-hole | Molex `22-28-4063`, Wurth `691216510006`, Generic 1x6 header | Connects the carrier board to the external wiring harness: TPMS1, BTN_A, PWR_BTN#, PS_ON#, GND, +5VSB. |
| 6 | `Q1` | 1 | BC547 | TO-92 Inline (C-B-E pinout) | $V_{CEO} = 45\text{ V}$, $I_C = 100\text{ mA}$, NPN BJT | onsemi `BC547B`, Diotec `BC547B` / `BC548` | NPN BJT low-side open-collector buffer; isolates ESP32-CAM GPIO14 from the ATX +5V standby pull-up and pulls `PS_ON#` LOW when active. |
| 7 | `R1`, `R3` | 2 | 1 kΩ | Axial Resistor (DIN0207, P7.62mm / P10.16mm) | 1/4 W (0.25 W), 1% Metal Film (or 5% Carbon) | Yageo `MFR-25FRF52-1K`, TE Connectivity `LR0204F1K0` | `R1`: Series current limiter for TPMS1 sense input. `R3`: Series base protection resistor for Q1. |
| 8 | `R2` | 1 | 4.7 kΩ | Axial Resistor (DIN0207, P7.62mm / P10.16mm) | 1/4 W (0.25 W), 1% Metal Film (or 5% Carbon) | Yageo `MFR-25FRF52-4K7`, TE Connectivity `LR0204F4K7` | Pull-down resistor on GPIO13 (`BOARD_SENSE`); divides against the ESP32-CAM internal 47 kΩ SD card DAT3 pull-up to keep input solidly LOW when PC is OFF. |
| 9 | `R4` | 1 | 100 kΩ | Axial Resistor (DIN0207, P7.62mm / P10.16mm) | 1/4 W (0.25 W), 1% Metal Film (or 5% Carbon) | Yageo `MFR-25FRF52-100K`, TE Connectivity `LR0204F100K` | Base pull-down resistor for Q1; guarantees `Q1` base remains at 0V during ESP32 power-on reset, bootloader flashing, and deep sleep. |

### Component Notes & Selection Rules

- **Electrolytic Capacitors (`C_PWR1`, `C_PWR3`)**:
  - `C_PWR1` is critical. The ESP32 2.4 GHz radio draws momentary peaks up to 300–400 mA. Without at least 220 µF–470 µF low-ESR bulk capacitance directly adjacent to the 5V power inlet, the board can suffer spontaneous brownout resets during Bluetooth connection handshakes.
  - Pay strict attention to polarity: the negative stripe on radial capacitors must connect to the `GND` bus (Row 8 on the carrier board).
- **BC547 / BC548 NPN Transistor Pinout**:
  - The TO-92 package lead arrangement used on this board corresponds to standard BC547 / BC548 (Pin 1 = Collector, Pin 2 = Base, Pin 3 = Emitter). Looking from the component side, the flat face points toward Row 9 (bottom). Pin 1 connects to `PS_ON#` (Col 5, Row 7), Pin 2 to Base drive (Col 6, Row 7), and Pin 3 to `GND` (Col 7, Row 9).
- **Resistor Power Ratings**:
  - Standard 1/4W (0.25W) axial metal film resistors are recommended for their precision (1%), temperature stability, and standard lead diameter (0.5 mm) fitting cleanly into perfboard holes.

---

## 2. Interconnect Wiring Harness & Chassis Hardware

The wiring harness connects the carrier board's external terminal (`J_EXT`) to the PC host power supply, motherboard headers, and front panel switch.

| Item | Component | Qty | Specifications | Recommended Hardware / Part | Purpose & Wiring Details |
|:----:|:----------|:---:|:---------------|:----------------------------|:-------------------------|
| 1 | ATX 24-Pin Tap / Breakout Leads | 1 | 3 conductors (20–22 AWG stranded copper wire) | 24-pin ATX power supply jumper cable with tap-off leads, or wire tap splice connectors | Taps +5VSB (Pin 9, Purple), COM/GND (Pin 15/17, Black), and PS_ON# (Pin 16, Green) from the main PC power supply cable. |
| 2 | TPMS1 Sensing Lead | 1 | 1 conductor (24–26 AWG flexible hookup wire) | Single female DuPont jumper lead (2.54 mm or 2.0 mm pitch depending on motherboard) | Connects Motherboard TPMS1 Header Pin 9 (board 3.3V standby/power status) to `J_EXT` pin 1 (`TPMS1`). |
| 3 | Momentary Front-Panel Switch | 1 | SPST Normally Open (NO) momentary push button | Chassis front-panel button, 12mm vandal-resistant push button, or 6x6 tactile switch | Connects between `J_EXT` pin 2 (`BTN_A`) and common Ground (`J_EXT` pin 5). Shorting terminal A to GND signals local power toggling. |
| 4 | Optional Shutdown Lead (`PWR_BTN#`) | 1 | 1 pair (24–28 AWG twisted hookup wire) | 2-pin female DuPont header lead | Connects `J_EXT` pin 3 (`PWR_BTN#`) to the Motherboard front-panel `POWER SW` positive pin for optional ACPI graceful shutdown triggering. |
| 5 | Carrier Board Substrate | 1 | Single-sided dot matrix copper protoboard / perfboard | Standard 2.54 mm pitch FR4 or phenolic perfboard, min. 30 × 12 holes (approx. 76 × 30 mm) | Mechanical substrate for mounting carrier passives, sockets, and solder-bridge bottom routing tracks. |
| 6 | Bus Wire / Solder Wire | — | 20–22 AWG solid tinned copper wire / 60-40 rosin core solder | 0.8 mm rosin core solder wire; solid wire for `+5VSB` and `GND` high-current bus tracks | Provides low-resistance conductors along carrier Row 8 (GND bus) and Row 9 (+5VSB bus). |

### Wire Gauge & Color Recommendations

| Signal Name | Carrier Pin (`J_EXT`) | Recommended Wire Color | Minimum Wire Gauge | Destination Connection |
|:------------|:---------------------:|:-----------------------|:-------------------|:-----------------------|
| `+5VSB` | Pin 1 | **Purple** / **Red** | 20–22 AWG | ATX 24-Pin Connector Pin 9 |
| `GND` | Pin 2 | **Black** | 20–22 AWG | ATX 24-Pin Connector Pin 15 / 17 |
| `TPMS1` | Pin 3 | **Gray** / **White** | 24–26 AWG | Motherboard TPMS1 Header Pin 9 |
| `BTN_A` | Pin 4 | **Blue** / **Orange** | 24–26 AWG | Front Panel Push Button Terminal A |
| `PS_ON#` | Pin 5 | **Green** | 22–24 AWG | ATX 24-Pin Connector Pin 16 |
| `PWR_BTN#` | Pin 6 | **Yellow** / **White** | 24–26 AWG | Motherboard Front Panel `POWER SW` (+) |

---

## 3. ESP32-CAM Module & Programming Tools

Flashing and operating the AI-Thinker ESP32-CAM requires the module itself and a serial programming interface (as the ESP32-CAM board does not include an onboard USB-to-UART bridge).

| Item | Component | Qty | Specifications | Recommended Part / Sourcing Keywords | Purpose & Operational Function |
|:----:|:----------|:---:|:---------------|:--------------------------------------|:-------------------------------|
| 1 | AI-Thinker ESP32-CAM Module | 1 | ESP32-D0WD module, 240 MHz dual-core, 4 MB Flash, 2 MB PSRAM, PCB antenna, OV2640 camera connector | AI-Thinker ESP32-CAM development board (ESP-32S) | Core microcontroller module executing the BC Awake firmware, Bluetooth server, and ATX control logic. *(Note: OV2640 camera sensor can be left unmounted to conserve power).* |
| 2 | USB-to-UART Serial Adapter | 1 | 3.3V/5V logic USB serial interface (CP2102, FT232RL, or CH340G) with DTR/RTS or manual buttons | USB to TTL Serial Converter Cable / Module, or dedicated **ESP32-CAM-MB** daughterboard | Connects to PC host via USB to flash firmware binary (`pio run -t upload`) and view boot logs (`pio device monitor`). |
| 3 | Flashing Boot Jumper | 1 | 2.54 mm pitch 2-pin shorting jumper shunt or female-to-female DuPont jumper | Standard 2.54 mm jumper shunt or 10 cm female-to-female DuPont wire | Connects `IO0` (Right header Pin 3) to `GND` (Right header Pin 4 or Left header Pin 2) to force bootloader download mode during flashing. Must be removed for normal boot. |

### Flashing Pin Interconnect (ESP32-CAM to Serial Adapter)

When using a standalone USB-to-UART adapter (without an ESP32-CAM-MB daughterboard):

| USB-UART Adapter Pin | ESP32-CAM Header Pin | Notes |
|:---------------------|:---------------------|:------|
| **5V / VCC** | **5V** (Left header pin 1) | Supply 5V power from USB adapter (ESP32-CAM onboard regulator generates 3.3V). |
| **GND** | **GND** (Left header pin 2 or Right header pin 4) | Common ground reference. **Do NOT connect to Right Pin 2 (`IO16`/PSRAM) or Right Pin 8 (`GND/R` Reset)!** |
| **RXD** | **U0TX** (Right header pin 7) | Serial receive line on adapter connects to ESP32 UART0 TX. |
| **TXD** | **U0RX** (Right header pin 6) | Serial transmit line on adapter connects to ESP32 UART0 RX. |
| **JUMPER** | **IO0** (Right pin 3) to **GND** | Straps ESP32 into ROM bootloader mode upon reset. |

---

## 4. Distributor Sourcing & Search Reference

Components can be acquired from standard catalog electronic distributors (Digi-Key, Mouser, LCSC) or hobby electronics suppliers (AliExpress, Amazon, Adafruit).

| Distributor | Search Term / Part Family | Notes |
|:------------|:--------------------------|:------|
| **LCSC** | `BC547B`, `EEU-FR1C471`, `C315C104K5R5TA`, `MFR-25FRF52-1K` | Fast one-stop order for all passives, connectors, and transistors with exact KiCad footprints. |
| **Digi-Key** | `BC547B-ND`, `P10225-ND` (470uF), `399-4454-1-ND` (100nF) | High reliability, rapid domestic shipping. |
| **Mouser** | `512-BC547B`, `667-EEU-FR1C471`, `80-C315C104K5R` | Complete matching BOM with factory datasheets. |
| **AliExpress** | `"ESP32-CAM with MB programmer"`, `"single side perfboard 2.54mm"`, `"24-pin ATX extension cable"` | Cost-effective bulk procurement for carrier protoboards, DuPont wires, and ATX breakout harness. |

---

## 5. Related References

- [AI-Thinker ESP32-CAM Carrier Board Design & Layout Guide](esp32cam-carrier-pcb.md)
- [Carrier Board Machine-Readable BOM CSV](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv)
- [Complete BC Awake Pinout & Wiring Reference](pinout.md)
- [Firmware Board Pin Configurations](../include/board.h)
