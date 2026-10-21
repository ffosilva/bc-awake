# AI-Thinker ESP32-CAM Prototype Carrier Board

Single-layer PCB and dot-matrix protoboard / perfboard reference layout for the AI-Thinker ESP32-CAM carrier board. This board integrates female socket headers for the ESP32-CAM module, power bulk decoupling, TPMS1 input conditioning, a 2N7000 MOSFET low-side buffer for `PS_ON#`, and an external wiring terminal.

All pads and traces are aligned to a strict **2.54 mm (100 mil)** pitch grid on a single copper layer (`B.Cu`), making it straightforward to build on standard prototype boards with holes and solder pads.

KiCad 8/9/10 project files and exported vector graphics are located in [`hardware/kicad/esp32cam-carrier/`](../hardware/kicad/esp32cam-carrier/).

---

## 1. Physical Specifications & Dimensions

- **Grid Pitch**: 2.54 mm (0.1 in / 100 mil) standard perfboard pitch.
- **Board Grid**: 30 columns × 12 rows (approx. 76.2 mm × 30.5 mm).
- **ESP32-CAM Header Sockets**: Two parallel 1×8 female pin socket headers (`J_CAM_L` and `J_CAM_R`) with **22.86 mm (0.9 in / exactly 9 grid units)** center-to-center row spacing.
- **Copper Layer**: 100% single bottom copper layer (`B.Cu`), zero track crossings (no top jumper wires needed).
- **Trace Widths**:
  - Power rails (`+5VSB`, `GND`, `+3V3`): 1.2 mm (or thick solder bridges / 20–22 AWG bus wire).
  - Signal tracks: 0.8 mm (or 24–30 AWG wire jumper / solder bridge).

---

## 2. Bill of Materials (BOM)

For an exhaustive catalog including ratings, manufacturer part numbers (LCSC, Digi-Key, Mouser), tolerances, wiring harness parts, and programmer tools, see the dedicated [AI-Thinker ESP32-CAM BOM Guide](bom.md) and machine-readable [CSV export](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv).

| Designator | Component | Package | Value / Spec | Purpose |
|------------|-----------|---------|--------------|---------|
| `J_CAM_L` | 1×8 Female Socket Header | 2.54 mm THT | 8-pin 2.54mm pitch | ESP32-CAM Left Header (`5V` to `IO4`) |
| `J_CAM_R` | 1×8 Female Socket Header | 2.54 mm THT | 8-pin 2.54mm pitch | ESP32-CAM Right Header (`3V3`, middle `GND`; `IO16` & `GND/R` floating) |
| `J_EXT` | 1×6 Male Header / Screw Terminal | 2.54 mm THT | 6-pin 2.54mm pitch | External ATX and Motherboard Connections |
| `C_PWR1` | Radial Electrolytic Capacitor | THT (D8.0mm, P3.5mm) | **470 µF**, 16V Low ESR | Bulk 5V rail reservoir against Wi-Fi/BT bursts |
| `C_PWR2` | Ceramic Disc Capacitor | THT (D5.0mm, P2.5mm) | **100 nF** (0.1 µF), 25V X7R | High-frequency 5V decoupling |
| `C_PWR3` | Radial Electrolytic / Tantalum | THT (D6.3mm, P2.5mm) | **47 µF**, 16V (Optional) | Buffers AMS1117 3.3V rail output |
| `C_PWR4` | Ceramic Disc Capacitor | THT (D5.0mm, P2.5mm) | **100 nF** (0.1 µF), 25V X7R | High-frequency 3.3V decoupling |
| `R1` | Metal Film Axial Resistor | THT (DIN0207, P7.62mm) | **1 kΩ**, 1/4W 1% | Series current limit & filter for TPMS1 |
| `R2` | Metal Film Axial Resistor | THT (DIN0207, P10.16mm) | **4.7 kΩ**, 1/4W 1% | Pull-down for TPMS1 (divides 47 kΩ SD pull-up) |
| `C1` | Ceramic Disc Capacitor | THT (D5.0mm, P10.16mm) | **100 nF** (0.1 µF), 25V X7R | TPMS1 low-pass filter ($f_c \approx 1.6\text{ kHz}$) |
| `Q1` | N-Channel MOSFET / NPN BJT | TO-92 Inline (2.54mm formed) | **2N7000 / BC547** | Low-side buffer isolates GPIO 14 from PSU 5V line |
| `R3` | Metal Film Axial Resistor | THT (DIN0207, P7.62mm) | **1 kΩ**, 1/4W 1% | Gate protection resistor for Q1 |
| `R4` | Metal Film Axial Resistor | THT (DIN0207, P5.08mm) | **100 kΩ**, 1/4W 1% | Gate pull-down (keeps PS_ON# off during ESP reset) |
| `C2` | Ceramic Disc Capacitor | THT (D5.0mm, P5.08mm) | **100 nF** (0.1 µF), 25V X7R | Gate snubber (filters reset-time microsecond glitches) |

---

## 3. External Terminal (`J_EXT`) Pinout

The 1×6 external header is located on the left edge (Rows 3 through 8):

| Pin | Row | Label | Connects to | Notes |
|:---:|:---:|:------|:------------|:------|
| **1** | Row 3 | `+5VSB` | ATX 24-pin Pin 9 (Purple Wire) | Permanent standby power (+5V) |
| **2** | Row 4 | `GND` | ATX 24-pin Pins 15/17 + Common Ground | Common ground rail |
| **3** | Row 5 | `TPMS1` | Motherboard TPMS1 Header Pin 9 | Board-on power sense (~3.3V when ON, 0V when OFF) |
| **4** | Row 6 | `BTN_A` | Front-panel momentary push button (Terminal A) | Terminal B connects to `GND` |
| **5** | Row 7 | `PS_ON#` | ATX 24-pin Connector Pin 16 (Green Wire) | Pulled LOW by Q1 to start PSU |
| **6** | Row 8 | `PWR_BTN#` | Motherboard FPANEL Power Button (+) pin | Optional graceful shutdown mirror (idles ~3.4V) |

---

## 4. Protoboard Grid & Pad Placement Map

The layout is arranged on a standard dot-matrix protoboard coordinate grid (Columns 1–28, Rows 1–10).

### Top View (Component Side)

```
Col:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
Row:
 1   [GND-BUS─────────────────────────────────────────────────────────────]  [GND-BUS───────]
 2   [G] .  .  .  .  .  .  .  .  . [5V-IN] . [CAM_L:5V] .  .  .  .  .  .  .  [CAM_R:3V3]-[C3+]-[C4+]
 3   [G][EXT:5V]  .  .  .  .  .  . [C1_+] [C2_+] . [CAM_L:GND] . .  .  .  .  [CAM_R:IO16] .  .  .
 4   [G][EXT:GND] .  .  .  .  .  .  .  .   .  .  . [CAM_L:IO12]. .  .  .  .  [CAM_R:IO0]  .  .  .
 5   [G][EXT:TPMS1]  [R1_1]──────[R1_2][R2_2][C1_2] [CAM_L:IO13]. .  .  .  . [CAM_R:GND]─────────[GND]
 6   [G][EXT:BTN_A]─────────────────────────────────[CAM_L:IO15]. .  .  .  . [CAM_R:VCC]  .  .  .
 7   [G][EXT:PS_ON]  . [Q1:D][Q1:G] . [C2_1][R4_1][R3_1]-[R3_2]-[CAM_L:IO14] [CAM_R:RX]   .  .  .
 8   [G][EXT:PWR_B]─────────────────────────────────[CAM_L:IO2] . .  .  .  . [CAM_R:TX]   .  .  .
 9   [GND-BUS──────────[Q1:S][C2_2][R4_2]] .   .  . [CAM_L:IO4] . .  .  .  . [CAM_R:GND/R] .  .  .
```

> [!IMPORTANT]
> **Pin Safety Note**:
> - `GND/R` (Right Header Pin 8, Column 22, Row 9) is an active-low reset pin (`RST` / `EN`) on many ESP32-CAM boards despite the silkscreen label. Grounding this pin holds the ESP32 permanently in reset mode. It has **zero connections and is left completely floating**.
> - `IO16` (Right Header Pin 2, Column 22, Row 3) is the ESP32-CAM **PSRAM line**. It has **zero connections and is left completely floating**.
> - `IO12`, `IO4`, `IO0`, `VCC`, `U0RX`, and `U0TX` are likewise left completely unconnected.

---

## 5. Single-Layer Bottom Copper Track Routing (`B.Cu`)

All traces run on the bottom copper foil (`B.Cu`). Since every track runs in its own dedicated lane or grid coordinate, there are **zero crossing tracks** and **zero jumper wires required**.

### Bottom Copper Routing Lanes
1. **Row 1, Column 1 & Row 9 (`GND` Ground Bus)**:
   - Top bus (Row 1): Runs from `J_EXT` pin 2 (Col 2, Row 4) left to Col 1, up to Row 1, and across to Col 27, connecting `R2` pin 1 (Col 8), `C1` pin 1 (Col 9), `C_PWR1` (-) (Col 10), `C_PWR2` (-) (Col 11), `J_CAM_L` pin 2 (`GND`, Col 13 Row 3 via Col 14), `C_PWR3` (-) (Col 24), and `C_PWR4` (-) (Col 26).
   - Right link: Runs down Col 27 from Row 1 to Row 5 to connect `J_CAM_R` pin 4 (middle `GND`, Col 22 Row 5).
   - Left link & bottom bus (Col 1 & Row 9): Runs down Col 1 from Row 4 to Row 9 and across Row 9 to connect `Q1` Source/Emitter (Col 7), `C2` (-) (Col 8), and `R4` (-) (Col 9), while leaving `J_CAM_R` pin 8 (`GND/R`, Col 22 Row 9) completely isolated.
2. **Row 3 to Row 2 (`+5VSB` Power Bus)**:
   - Solder bridge / 20 AWG wire from `J_EXT` pin 1 (Col 2, Row 3) → `C_PWR1` (+) (Col 10, Row 3) → `C_PWR2` (+) (Col 11, Row 3) → steps up to Row 2 → `J_CAM_L` pin 1 (Col 13, Row 2).
3. **Row 2 (`+3V3` Power Rail)**:
   - Straight trace from `J_CAM_R` pin 1 (Col 22, Row 2) → `C_PWR3` (+) (Col 24, Row 2) → `C_PWR4` (Col 26, Row 2).
4. **Row 5 (`TPMS1` / `NET_IO13`)**:
   - `TPMS1` enters at `J_EXT` pin 3 (Col 2, Row 5) → `R1` (Col 4..7) → `R2` (Col 8) → `C1` (Col 9) → straight into `J_CAM_L` pin 4 (Col 13, Row 5).
5. **Row 6 (`BTN_A`)**:
   - Straight trace from `J_EXT` pin 4 (Col 2, Row 6) to `J_CAM_L` pin 5 (Col 13, Row 6).
6. **Row 7 (`PS_ON#` & `NET_IO14`)**:
   - `PS_ON#` from `J_EXT` pin 5 (Col 2, Row 7) → `Q1` Drain (Col 5, Row 7).
   - `NET_GATE` connects `Q1` Gate (Col 6) → `C2` (Col 8) → `R4` (Col 9) → `R3` (Col 10..12).
   - `NET_IO14` from `R3` pad 2 (Col 12, Row 7) into `J_CAM_L` pin 6 (Col 13, Row 7).
7. **Row 8 (`PWR_BTN#`)**:
   - Straight trace from `J_EXT` pin 6 (Col 2, Row 8) to `J_CAM_L` pin 7 (Col 13, Row 8).

---

## 6. Step-by-Step Soldering & Assembly Instructions

### Preparation
1. Cut a standard 2.54 mm dot-matrix prototype board to at least **30 columns × 12 rows** (approx. 76 mm × 31 mm).
2. Clean the copper pads with isopropyl alcohol (IPA) to ensure smooth solder flow.

### Step 1: Install Low-Profile Passive Components
1. **Resistors**:
   - Insert `R1` (1 kΩ) horizontally at Col 4..7 on Row 5.
   - Insert `R2` (4.7 kΩ) vertically at Col 8 spanning Row 1 to Row 5.
   - Insert `R3` (1 kΩ) horizontally at Col 10..12 on Row 7.
   - Insert `R4` (100 kΩ) vertically at Col 9 spanning Row 7 to Row 9.
2. **Ceramic Capacitors**:
   - Insert `C1` (100 nF) vertically at Col 9 spanning Row 1 to Row 5.
   - Insert `C2` (100 nF) vertically at Col 8 spanning Row 7 to Row 9.
   - Insert `C_PWR2` (100 nF) vertically at Col 11 spanning Row 1 (`GND`) to Row 3 (`+5VSB`).
   - Insert `C_PWR4` (100 nF) vertically at Col 26 spanning Row 1 (`GND`) to Row 2 (`+3V3`).
3. Bend leads slightly on the bottom side to hold components flush, solder pads, and trim excess lead length.

### Step 2: Install Transistor and Electrolytic Capacitors
1. **MOSFET `Q1` (2N7000)**:
   - Form the leads to 2.54 mm pitch.
   - Insert Pin 3 (Drain) at Col 5 Row 7, Pin 2 (Gate) at Col 6 Row 7, and Pin 1 (Source) at Col 7 Row 9.
   - Ensure the flat face faces toward the top of the board. Solder and trim.
2. **Bulk Capacitors `C_PWR1` and `C_PWR3`**:
   - **Observe polarity**: Pin 1 (+) connects to power (`+5VSB` on Row 3 or `+3V3` on Row 2); Pin 2 (-) connects to `GND` (Row 1).
   - Insert `C_PWR1` (470 µF) at Col 10 (Pad 1 at Row 3, Pad 2 at Row 1).
   - Insert `C_PWR3` (47 µF) at Col 24 (Pad 1 at Row 2, Pad 2 at Row 1).
   - Solder and trim.

### Step 3: Install Socket Headers & External Connector
1. **ESP32-CAM Sockets**:
   - Insert two 1×8 female socket headers:
     - `J_CAM_L` at Column 13, Rows 2 through 9.
     - `J_CAM_R` at Column 22, Rows 2 through 9.
   - Verify that row-to-row spacing is exactly **22.86 mm (9 hole spaces)** so the ESP32-CAM module seats smoothly.
   - Tack opposite corner pins, verify perpendicular alignment to the board, and solder all remaining pins.
2. **External Header `J_EXT`**:
   - Insert 1×6 male pin header or screw terminal at Column 2, Rows 3 through 8. Solder all pins.

### Step 4: Solder Bottom Copper Bus Tracks
1. Form solder bridges or lay stripped 22–24 AWG tinned solid wire along the bottom rows matching the routing lanes described in Section 5.
2. Solder the main `GND` bus (Row 1, Col 1, and Row 9 Cols 1–9) and `+5VSB` bus (Row 3 to Row 2) using 20–22 AWG wire to handle current surges.
3. Connect the right-side GND link (Col 27 from Row 1 to Row 5 into `J_CAM_R` Pin 4). **Do NOT connect `J_CAM_R` Pin 8 (Row 9) to `GND`**, as it is `GND/R` (Reset).

### Step 5: Verification & Testing Before Module Insertion
1. **Continuity Check (Multimeter)**:
   - Probe between `J_EXT` Pin 1 (`+5VSB`) and `J_CAM_L` Pin 1: verify **0 Ω**.
   - Probe between `J_EXT` Pin 2 (`GND`) and `J_CAM_L` Pin 2 and `J_CAM_R` Pin 4: verify **0 Ω**.
   - Probe between `J_CAM_R` Pin 8 (`GND/R`) and `GND`: verify **open circuit** (must NOT be grounded).
   - Probe between `+5VSB` and `GND`: verify **open circuit / high resistance** (no short).
   - Probe between `+3V3` (`J_CAM_R` Pin 1) and `GND`: verify **open circuit**.
2. **Cold Power Test**:
   - Apply 5V DC from bench PSU or ATX +5VSB to `J_EXT` Pin 1 (`+5VSB`) and Pin 2 (`GND`) with ESP32-CAM **unplugged**.
   - Verify 5.0 V on `J_CAM_L` Pin 1. Verify 0 V on all other pins.
   - Power off. Plug in the AI-Thinker ESP32-CAM module (camera module facing up, antenna at top).
3. **Operational Test**:
   - Power on. Confirm the ESP32-CAM boots cleanly without brownout reset (`ESP_RST_POWERON`).
   - Ground `BTN_A` briefly: verify the ESP32 registers button press.
   - Ground `TPMS1`: verify state transitions accurately.

---

## 7. KiCad Project Files & Visual Artifacts

All KiCad design files, schematics, netlists, and vector plots are stored in [`hardware/kicad/esp32cam-carrier/`](../hardware/kicad/esp32cam-carrier/):

- [`esp32cam-carrier.kicad_pro`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier.kicad_pro): KiCad project file.
- [`esp32cam-carrier.kicad_sch`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier.kicad_sch): Schematic.
- [`esp32cam-carrier.kicad_pcb`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier.kicad_pcb): Single-layer PCB layout.
- [`esp32cam-carrier.net`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier.net): Complete netlist.
- [`drc_report.rpt`](../hardware/kicad/esp32cam-carrier/drc_report.rpt): DRC report (0 errors, 0 unconnected items).
- [`esp32cam-carrier-copper.svg`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-copper.svg): Vector plot of bottom copper tracks (`B.Cu`).
- [`esp32cam-carrier-silk.svg`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-silk.svg): Vector plot of component placement silkscreen (`F.SilkS`).
- [`esp32cam-carrier.pdf`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier.pdf): Combined printable vector schematic & PCB document.
- [`esp32cam-carrier-bom.csv`](../hardware/kicad/esp32cam-carrier/esp32cam-carrier-bom.csv): Machine-readable Bill of Materials CSV.

For detailed hardware sourcing, tolerances, and external interconnect harness components, see [docs/bom.md](bom.md).

To regenerate or verify the board programmatically at any time, run:
```bash
python3 scripts/generate_carrier_pcb.py
```
