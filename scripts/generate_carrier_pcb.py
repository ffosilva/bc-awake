#!/usr/bin/env python3
"""
Full KiCad Schematic and Single-Layer PCB Generator for
BC Awake AI-Thinker ESP32-CAM Prototype Carrier Board.
"""

import os
import sys
import uuid
import subprocess

OUT_DIR = "hardware/kicad/esp32cam-carrier"
os.makedirs(OUT_DIR, exist_ok=True)

PRO_PATH = os.path.join(OUT_DIR, "esp32cam-carrier.kicad_pro")
SCH_PATH = os.path.join(OUT_DIR, "esp32cam-carrier.kicad_sch")
PCB_PATH = os.path.join(OUT_DIR, "esp32cam-carrier.kicad_pcb")
SVG_CU_PATH = os.path.join(OUT_DIR, "esp32cam-carrier-copper.svg")
SVG_SILK_PATH = os.path.join(OUT_DIR, "esp32cam-carrier-silk.svg")
PDF_PATH = os.path.join(OUT_DIR, "esp32cam-carrier.pdf")

KICAD_CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

def u():
    return str(uuid.uuid4())

root_uuid = u()
project_name = "esp32cam-carrier"

# ===========================================================================
# 1. Project Configuration (.kicad_pro)
# ===========================================================================
pro_content = f"""{{
  "board": {{
    "design_settings": {{
      "defaults": {{
        "board_outline_clearance": 0.5,
        "copper_line_width": 0.8,
        "copper_text_size_h": 1.5,
        "copper_text_size_v": 1.5,
        "copper_text_thickness": 0.3,
        "silk_line_width": 0.12,
        "silk_text_size_h": 0.8,
        "silk_text_size_v": 0.8,
        "silk_text_thickness": 0.12
      }},
      "diff_pair_dimensions": [],
      "drc_exclusions": [],
      "rules": {{
        "max_error": 0.005,
        "min_clearance": 0.25,
        "min_copper_edge_clearance": 0.5,
        "min_hole_clearance": 0.25,
        "min_hole_to_hole": 0.25,
        "min_microvia_diameter": 0.2,
        "min_microvia_drill": 0.1,
        "min_resolved_spoke_count": 2,
        "min_silk_clearance": 0.05,
        "min_text_height": 0.6,
        "min_text_thickness": 0.1,
        "min_through_hole_diameter": 0.3,
        "min_track_width": 0.5,
        "min_via_annular_width": 0.15,
        "min_via_diameter": 0.6,
        "use_height_for_length_calcs": true
      }},
      "track_widths": [
        0.8,
        1.2,
        1.5
      ],
      "via_dimensions": []
    }}
  }},
  "boards": [],
  "libraries": {{
    "pinned_footprint_libs": [],
    "pinned_symbol_libs": []
  }},
  "meta": {{
    "filename": "{project_name}.kicad_pro",
    "version": 1
  }},
  "net_settings": {{
    "classes": [
      {{
        "clearance": 0.25,
        "diff_pair_gap": 0.25,
        "diff_pair_via_gap": 0.25,
        "diff_pair_width": 0.2,
        "microvia_diameter": 0.3,
        "microvia_drill": 0.1,
        "name": "Default",
        "pcb_color": "rgba(0, 0, 0, 0.000)",
        "schematic_color": "rgba(0, 0, 0, 0.000)",
        "track_width": 0.8,
        "via_diameter": 0.8,
        "via_drill": 0.4,
        "wire_width": 0
      }},
      {{
        "clearance": 0.25,
        "diff_pair_gap": 0.25,
        "diff_pair_via_gap": 0.25,
        "diff_pair_width": 0.2,
        "microvia_diameter": 0.3,
        "microvia_drill": 0.1,
        "name": "Power",
        "nets": [
          "+5VSB",
          "GND",
          "+3V3"
        ],
        "pcb_color": "rgba(0, 0, 0, 0.000)",
        "schematic_color": "rgba(0, 0, 0, 0.000)",
        "track_width": 1.2,
        "via_diameter": 1.0,
        "via_drill": 0.5,
        "wire_width": 0
      }}
    ],
    "meta": {{
      "version": 0
    }}
  }},
  "pcbnew": {{
    "page_layout_descr_file": ""
  }},
  "sheets": [],
  "text_variables": {{}}
}}
"""

# ===========================================================================
# 2. Schematic (.kicad_sch) Generation
# ===========================================================================
sym_dir = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols"

def extract_sym(lib_file, sym_name):
    with open(f"{sym_dir}/{lib_file}.kicad_sym") as f:
        content = f.read()
    target = f'(symbol "{sym_name}"'
    idx = content.find(target)
    if idx == -1:
        raise ValueError(f"Symbol {sym_name} not found in {lib_file}")
    depth = 0
    end = idx
    for i in range(idx, len(content)):
        if content[i] == '(':
            depth += 1
        elif content[i] == ')':
            depth -= 1
            if depth == 0:
                end = i + 1
                break
    return content[idx:end]

sym_c08 = extract_sym("Connector_Generic", "Conn_01x08").replace('(symbol "Conn_01x08"', '(symbol "Connector_Generic:Conn_01x08"', 1)
sym_c06 = extract_sym("Connector_Generic", "Conn_01x06").replace('(symbol "Conn_01x06"', '(symbol "Connector_Generic:Conn_01x06"', 1)
sym_r = extract_sym("Device", "R").replace('(symbol "R"', '(symbol "Device:R"', 1)
sym_c = extract_sym("Device", "C").replace('(symbol "C"', '(symbol "Device:C"', 1)
sym_cp = extract_sym("Device", "C_Polarized").replace('(symbol "C_Polarized"', '(symbol "Device:C_Polarized"', 1)
sym_fet = extract_sym("Transistor_FET", "2N7000").replace('(symbol "2N7000"', '(symbol "Transistor_FET:2N7000"', 1)

def pin_pos(sx, sy, px, py, rot):
    if rot == 0:
        return round(sx + px, 2), round(sy - py, 2)
    elif rot == 90:
        return round(sx - py, 2), round(sy - px, 2)
    elif rot == 180:
        return round(sx - px, 2), round(sy + py, 2)
    elif rot == 270:
        return round(sx + py, 2), round(sy + px, 2)
    raise ValueError(f"Unsupported rotation: {rot}")

def mk_symbol_inst(lib_id, ref, val, footprint, at_x, at_y, rot, pins, desc=""):
    pin_xml = "\n".join([f'\t\t(pin "{p}" (uuid "{u()}"))' for p in pins])
    return f"""\t(symbol
\t\t(lib_id "{lib_id}")
\t\t(at {at_x} {at_y} {rot})
\t\t(unit 1)
\t\t(exclude_from_sim no)
\t\t(in_bom yes)
\t\t(on_board yes)
\t\t(dnp no)
\t\t(uuid "{u()}")
\t\t(property "Reference" "{ref}"
\t\t\t(at {at_x} {at_y - 7.62} 0)
\t\t\t(effects (font (size 1.27 1.27)))
\t\t)
\t\t(property "Value" "{val}"
\t\t\t(at {at_x} {at_y - 5.08} 0)
\t\t\t(effects (font (size 1.27 1.27)))
\t\t)
\t\t(property "Footprint" "{footprint}"
\t\t\t(at {at_x} {at_y} 0)
\t\t\t(effects (font (size 1.27 1.27)) (hide yes))
\t\t)
\t\t(property "Datasheet" "~"
\t\t\t(at {at_x} {at_y} 0)
\t\t\t(effects (font (size 1.27 1.27)) (hide yes))
\t\t)
\t\t(property "Description" "{desc}"
\t\t\t(at {at_x} {at_y} 0)
\t\t\t(effects (font (size 1.27 1.27)) (hide yes))
\t\t)
{pin_xml}
\t\t(instances
\t\t\t(project "{project_name}"
\t\t\t\t(path "/{root_uuid}"
\t\t\t\t\t(reference "{ref}")
\t\t\t\t\t(unit 1)
\t\t\t\t)
\t\t\t)
\t\t)
\t)"""

schematic_symbols = [
    {"lib": "Connector_Generic:Conn_01x06", "ref": "J_EXT", "val": "Ext_Terminals", "fp": "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical", "x": 50.8, "y": 63.5, "rot": 0, "pins": ["1","2","3","4","5","6"], "desc": "External Terminals"},
    {"lib": "Connector_Generic:Conn_01x08", "ref": "J_CAM_L", "val": "ESP32-CAM_L", "fp": "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical", "x": 114.3, "y": 63.5, "rot": 0, "pins": [str(i) for i in range(1, 9)], "desc": "ESP32-CAM Left Header"},
    {"lib": "Connector_Generic:Conn_01x08", "ref": "J_CAM_R", "val": "ESP32-CAM_R", "fp": "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical", "x": 177.8, "y": 63.5, "rot": 0, "pins": [str(i) for i in range(1, 9)], "desc": "ESP32-CAM Right Header"},
    {"lib": "Device:C_Polarized", "ref": "C_PWR1", "val": "470uF_16V", "fp": "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm", "x": 88.9, "y": 114.3, "rot": 0, "pins": ["1", "2"], "desc": "5V Bulk Decoupling"},
    {"lib": "Device:C", "ref": "C_PWR2", "val": "100nF", "fp": "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm", "x": 101.6, "y": 114.3, "rot": 0, "pins": ["1", "2"], "desc": "5V Ceramic Decoupling"},
    {"lib": "Device:C_Polarized", "ref": "C_PWR3", "val": "47uF_16V", "fp": "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm", "x": 203.2, "y": 63.5, "rot": 0, "pins": ["1", "2"], "desc": "3.3V Bulk Decoupling"},
    {"lib": "Device:C", "ref": "C_PWR4", "val": "100nF", "fp": "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm", "x": 215.9, "y": 63.5, "rot": 0, "pins": ["1", "2"], "desc": "3.3V Ceramic Decoupling"},
    {"lib": "Device:R", "ref": "R1", "val": "1k", "fp": "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "x": 76.2, "y": 38.1, "rot": 90, "pins": ["1", "2"], "desc": "TPMS1 series resistor"},
    {"lib": "Device:R", "ref": "R2", "val": "4.7k", "fp": "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "x": 88.9, "y": 38.1, "rot": 0, "pins": ["1", "2"], "desc": "TPMS1 pull-down"},
    {"lib": "Device:C", "ref": "C1", "val": "100nF", "fp": "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm", "x": 101.6, "y": 38.1, "rot": 0, "pins": ["1", "2"], "desc": "TPMS1 filter"},
    {"lib": "Transistor_FET:2N7000", "ref": "Q1", "val": "2N7000", "fp": "Package_TO_SOT_THT:TO-92_Inline_Wide", "x": 63.5, "y": 88.9, "rot": 0, "pins": ["1", "2", "3"], "desc": "PS_ON# Buffer"},
    {"lib": "Device:R", "ref": "R3", "val": "1k", "fp": "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "x": 88.9, "y": 88.9, "rot": 90, "pins": ["1", "2"], "desc": "2N7000 gate resistor"},
    {"lib": "Device:R", "ref": "R4", "val": "100k", "fp": "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "x": 76.2, "y": 101.6, "rot": 0, "pins": ["1", "2"], "desc": "2N7000 gate pull-down"},
    {"lib": "Device:C", "ref": "C2", "val": "100nF", "fp": "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm", "x": 63.5, "y": 101.6, "rot": 0, "pins": ["1", "2"], "desc": "2N7000 gate snubber"}
]

pin_net_map = {
    ("J_EXT", "1"): "TPMS1",
    ("J_EXT", "2"): "BTN_A",
    ("J_EXT", "3"): "PWR_BTN#",
    ("J_EXT", "4"): "PS_ON#",
    ("J_EXT", "5"): "GND",
    ("J_EXT", "6"): "+5VSB",

    ("J_CAM_L", "2"): "NET_IO13",
    ("J_CAM_L", "3"): "BTN_A",
    ("J_CAM_L", "4"): "NET_IO14",
    ("J_CAM_L", "5"): "PWR_BTN#",
    ("J_CAM_L", "7"): "GND",
    ("J_CAM_L", "8"): "+5VSB",

    ("J_CAM_R", "1"): "+3V3",
    ("J_CAM_R", "2"): "GND",
    ("J_CAM_R", "7"): "GND",

    ("C_PWR1", "1"): "+5VSB",
    ("C_PWR1", "2"): "GND",

    ("C_PWR2", "1"): "+5VSB",
    ("C_PWR2", "2"): "GND",

    ("C_PWR3", "1"): "+3V3",
    ("C_PWR3", "2"): "GND",

    ("C_PWR4", "1"): "+3V3",
    ("C_PWR4", "2"): "GND",

    ("R1", "1"): "TPMS1",
    ("R1", "2"): "NET_IO13",

    ("R2", "1"): "GND",
    ("R2", "2"): "NET_IO13",

    ("C1", "1"): "GND",
    ("C1", "2"): "NET_IO13",

    ("Q1", "1"): "GND",
    ("Q1", "2"): "NET_GATE",
    ("Q1", "3"): "PS_ON#",

    ("R3", "1"): "NET_IO14",
    ("R3", "2"): "NET_GATE",

    ("R4", "1"): "NET_GATE",
    ("R4", "2"): "GND",

    ("C2", "1"): "NET_GATE",
    ("C2", "2"): "GND",
}

sym_pin_offsets = {
    "Connector_Generic:Conn_01x06": {str(i): (-5.08, 5.08 - (i-1)*2.54) for i in range(1, 7)},
    "Connector_Generic:Conn_01x08": {str(i): (-5.08, 7.62 - (i-1)*2.54) for i in range(1, 9)},
    "Device:R": {"1": (0, 3.81), "2": (0, -3.81)},
    "Device:C": {"1": (0, 3.81), "2": (0, -3.81)},
    "Device:C_Polarized": {"1": (0, 3.81), "2": (0, -3.81)},
    "Transistor_FET:2N7000": {"1": (2.54, -5.08), "2": (-5.08, 0), "3": (2.54, 5.08)}
}

labels_list = []
for s in schematic_symbols:
    ref = s["ref"]
    lib = s["lib"]
    sx = s["x"]
    sy = s["y"]
    rot = s["rot"]
    for pin_num, (lx, ly) in sym_pin_offsets[lib].items():
        if (ref, pin_num) in pin_net_map:
            net = pin_net_map[(ref, pin_num)]
            wx, wy = pin_pos(sx, sy, lx, ly, rot)
            labels_list.append(f'\t(label "{net}" (at {wx} {wy} 0) (effects (font (size 1.27 1.27))) (uuid "{u()}"))')

symbols_inst_txt = "\n".join([
    mk_symbol_inst(s["lib"], s["ref"], s["val"], s["fp"], s["x"], s["y"], s["rot"], s["pins"], s["desc"])
    for s in schematic_symbols
])

sch_full = f"""(kicad_sch
\t(version 20250114)
\t(generator "eeschema")
\t(generator_version "10.0")
\t(uuid "{root_uuid}")
\t(paper "A4")
\t(title_block
\t\t(title "BC Awake ESP32-CAM Single-Layer Prototype Carrier Board")
\t\t(date "2026-10-20")
\t\t(rev "1.0")
\t\t(comment 1 "Single-layer PCB for protoboard replication")
\t\t(comment 2 "Bulk decoupling, TPMS1 RC filter, and PS_ON# MOSFET buffer")
\t)
\t(lib_symbols
{sym_c08}
{sym_c06}
{sym_r}
{sym_c}
{sym_cp}
{sym_fet}
\t)

{symbols_inst_txt}

{"\n".join(labels_list)}

\t(sheet_instances
\t\t(path "/{root_uuid}"
\t\t\t(page "1")
\t\t)
\t)
\t(embedded_fonts no)
)
"""

# ===========================================================================
# 3. PCB Layout (.kicad_pcb) Generation
# ===========================================================================
X0 = 100.0
Y0 = 100.0
G = 2.54

def gx(col):
    return round(X0 + col * G, 2)

def gy(row):
    return round(Y0 + row * G, 2)

BOARD_W = 30 * G
BOARD_H = 12 * G

pcb_header = f"""(kicad_pcb
\t(version 20241229)
\t(generator "pcbnew")
\t(generator_version "10.0")
\t(general
\t\t(thickness 1.6)
\t\t(legacy_teardrops no)
\t)
\t(paper "A4")
\t(title_block
\t\t(title "BC Awake ESP32-CAM Single-Layer Prototype Carrier Board")
\t\t(date "2026-10-20")
\t\t(rev "1.0")
\t\t(comment 1 "Single-layer (B.Cu) track layout for protoboard replication")
\t)
\t(layers
\t\t(0 "F.Cu" signal)
\t\t(2 "B.Cu" signal)
\t\t(5 "F.SilkS" user "F.Silkscreen")
\t\t(7 "B.SilkS" user "B.Silkscreen")
\t\t(1 "F.Mask" user)
\t\t(3 "B.Mask" user)
\t\t(25 "Edge.Cuts" user)
\t\t(31 "F.CrtYd" user "F.Courtyard")
\t\t(35 "F.Fab" user)
\t)
\t(setup
\t\t(pad_to_mask_clearance 0.05)
\t\t(allow_soldermask_bridges_in_footprints no)
\t\t(pcbplotparams
\t\t\t(layerselection 0x00010b0_ffffffff)
\t\t\t(plot_on_all_layers_selection 0x0000000_00000000)
\t\t\t(disableapertmacros no)
\t\t\t(usegerberextensions no)
\t\t\t(usegerberattributes yes)
\t\t\t(usegerberadvancedattributes yes)
\t\t\t(creategerberjobfile yes)
\t\t\t(dashed_line_dash_ratio 12.000000)
\t\t\t(dashed_line_gap_ratio 3.000000)
\t\t\t(svgprecision 4)
\t\t\t(plotframeref no)
\t\t\t(viasonmask no)
\t\t\t(mode 1)
\t\t\t(useauxorigin no)
\t\t\t(hpglpennumber 1)
\t\t\t(hpglpenspeed 20)
\t\t\t(hpglpendiameter 15.000000)
\t\t\t(pdf_front_fp_property_popups yes)
\t\t\t(pdf_back_fp_property_popups yes)
\t\t\t(dxfpolygonmode yes)
\t\t\t(dxfimperialunits yes)
\t\t\t(dxfusepcbnewfont yes)
\t\t\t(psnegative no)
\t\t\t(psa4output no)
\t\t\t(plotreference yes)
\t\t\t(plotvalue yes)
\t\t\t(plotfptext yes)
\t\t\t(plotinvisibletext no)
\t\t\t(sketchpadsonfab no)
\t\t\t(subtractmaskfromsilk no)
\t\t\t(outputformat 1)
\t\t\t(mirror no)
\t\t\t(drillshape 1)
\t\t\t(scaleselection 1)
\t\t\t(outputdirectory "")
\t\t)
\t)

\t(net 0 "")
\t(net 1 "GND")
\t(net 2 "+5VSB")
\t(net 3 "+3V3")
\t(net 4 "TPMS1")
\t(net 5 "NET_IO13")
\t(net 6 "PS_ON#")
\t(net 7 "NET_GATE")
\t(net 8 "BTN_A")
\t(net 9 "PWR_BTN#")
\t(net 10 "NET_IO14")

\t(gr_rect (start {X0 - 2.54} {Y0 - 2.54}) (end {round(X0 + BOARD_W, 2)} {round(Y0 + BOARD_H, 2)})
\t\t(stroke (width 0.2) (type solid))
\t\t(fill none)
\t\t(layer "Edge.Cuts")
\t\t(uuid "{u()}")
\t)

\t(gr_text "BC Awake ESP32-CAM Carrier"
\t\t(at {round(X0 + BOARD_W/2, 2)} {round(Y0 - 1.2, 2)} 0)
\t\t(layer "F.SilkS")
\t\t(uuid "{u()}")
\t\t(effects
\t\t\t(font (size 0.8 0.8) (thickness 0.12))
\t\t)
\t)
"""

def pad_tht(num, px, py, net_id, net_name, is_rect=False, drill=0.9, size=1.6):
    shape = "rect" if is_rect else "circle"
    return f"""\t\t(pad "{num}" thru_hole {shape} (at {px} {py}) (size {size} {size}) (drill {drill}) (layers "*.Cu" "*.Mask") (net {net_id} "{net_name}"))"""

pcb_footprints = []

# 1. J_EXT (1x6 pin header) at Col 2, Rows 4..9
# Pin 1 (Row 4): TPMS1
# Pin 2 (Row 5): BTN_A
# Pin 3 (Row 6): PWR_BTN#
# Pin 4 (Row 7): PS_ON#
# Pin 5 (Row 8): GND
# Pin 6 (Row 9): +5VSB
ext_pads = [
    (1, gx(2), gy(4), 4, "TPMS1", True),
    (2, gx(2), gy(5), 8, "BTN_A", False),
    (3, gx(2), gy(6), 9, "PWR_BTN#", False),
    (4, gx(2), gy(7), 6, "PS_ON#", False),
    (5, gx(2), gy(8), 1, "GND", False),
    (6, gx(2), gy(9), 2, "+5VSB", False)
]
ext_pads_str = "\n".join([pad_tht(p[0], p[1], p[2], p[3], p[4], p[5]) for p in ext_pads])
pcb_footprints.append(f"""\t(footprint "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "J_EXT" (at {gx(2)} {gy(3.1)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "Ext_Terminals" (at {gx(2)} {gy(9.9)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(1.4)} {gy(3.5)}) (end {gx(2.6)} {gy(3.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(2.6)} {gy(3.5)}) (end {gx(2.6)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(2.6)} {gy(9.5)}) (end {gx(1.4)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(1.4)} {gy(9.5)}) (end {gx(1.4)} {gy(3.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{ext_pads_str}
\t)""")

# 2. J_CAM_L (1x8 female pin socket) at Col 13, Rows 2..9
cam_l_pads = [
    (1, gx(13), gy(2), 0, "", True),         # IO12 (NC)
    (2, gx(13), gy(3), 5, "NET_IO13", False),  # IO13
    (3, gx(13), gy(4), 8, "BTN_A", False),     # IO15
    (4, gx(13), gy(5), 10, "NET_IO14", False), # IO14
    (5, gx(13), gy(6), 9, "PWR_BTN#", False),  # IO2
    (6, gx(13), gy(7), 0, "", False),         # IO4 (NC)
    (7, gx(13), gy(8), 1, "GND", False),      # GND
    (8, gx(13), gy(9), 2, "+5VSB", False)     # 5V
]
cam_l_str = "\n".join([pad_tht(p[0], p[1], p[2], p[3], p[4], p[5]) for p in cam_l_pads])
pcb_footprints.append(f"""\t(footprint "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "J_CAM_L" (at {gx(13)} {gy(1.1)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "ESP32-CAM_L" (at {gx(13)} {gy(9.9)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(12.4)} {gy(1.5)}) (end {gx(13.6)} {gy(1.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(13.6)} {gy(1.5)}) (end {gx(13.6)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(13.6)} {gy(9.5)}) (end {gx(12.4)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(12.4)} {gy(9.5)}) (end {gx(12.4)} {gy(1.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{cam_l_str}
\t)""")

# 3. J_CAM_R (1x8 female pin socket) at Col 22, Rows 2..9 (spacing: 9 cols = 22.86 mm = 0.9 in)
cam_r_pads = [
    (1, gx(22), gy(2), 3, "+3V3", True),  # 3V3
    (2, gx(22), gy(3), 1, "GND", False),  # GND
    (3, gx(22), gy(4), 0, "", False),     # IO0 (NC)
    (4, gx(22), gy(5), 0, "", False),     # U0TX (NC)
    (5, gx(22), gy(6), 0, "", False),     # U0RX (NC)
    (6, gx(22), gy(7), 0, "", False),     # IO16 (NC)
    (7, gx(22), gy(8), 1, "GND", False),  # GND
    (8, gx(22), gy(9), 0, "", False)      # VCC (NC)
]
cam_r_str = "\n".join([pad_tht(p[0], p[1], p[2], p[3], p[4], p[5]) for p in cam_r_pads])
pcb_footprints.append(f"""\t(footprint "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "J_CAM_R" (at {gx(22)} {gy(1.1)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "ESP32-CAM_R" (at {gx(22)} {gy(9.9)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(21.4)} {gy(1.5)}) (end {gx(22.6)} {gy(1.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(22.6)} {gy(1.5)}) (end {gx(22.6)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(22.6)} {gy(9.5)}) (end {gx(21.4)} {gy(9.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(21.4)} {gy(9.5)}) (end {gx(21.4)} {gy(1.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{cam_r_str}
\t)""")

# 4. C_PWR1 (470uF radial electrolytic) at Col 10, Pad 1 at Row 9 (+5VSB), Pad 2 at Row 8 (GND)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C_PWR1" (at {gx(10)} {gy(10.4)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "470uF" (at {gx(10)} {gy(6.8)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(9.4)} {gy(9)}) (end {gx(9.0)} {gy(9)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(9.2)} {gy(8.8)}) (end {gx(9.2)} {gy(9.2)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(10), gy(9), 2, "+5VSB", True, size=1.6)}
{pad_tht(2, gx(10), gy(8), 1, "GND", False, size=1.6)}
\t)""")

# 5. C_PWR2 (100nF ceramic disc) at Col 11, Pad 1 at Row 9 (+5VSB), Pad 2 at Row 8 (GND)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C_PWR2" (at {gx(11.5)} {gy(10.4)} 0) (layer "F.SilkS") (effects (font (size 0.6 0.6) (thickness 0.1))))
\t\t(property "Value" "100nF" (at {gx(11.5)} {gy(7.2)} 0) (layer "F.Fab") (effects (font (size 0.6 0.6) (thickness 0.1))))
\t\t(fp_line (start {gx(10.7)} {gy(8.5)}) (end {gx(11.3)} {gy(8.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(11), gy(9), 2, "+5VSB", False)}
{pad_tht(2, gx(11), gy(8), 1, "GND", False)}
\t)""")

# 6. R1 (1k series TPMS1) horizontal on Row 2: Col 4..7
pcb_footprints.append(f"""\t(footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "R1" (at {gx(5.5)} {gy(1.3)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "1k" (at {gx(5.5)} {gy(2.7)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_rect (start {gx(4.7)} {gy(1.7)}) (end {gx(6.3)} {gy(2.3)}) (stroke (width 0.1) (type solid)) (fill none) (layer "F.SilkS"))
{pad_tht(1, gx(4), gy(2), 4, "TPMS1", False)}
{pad_tht(2, gx(7), gy(2), 5, "NET_IO13", False)}
\t)""")

# 7. R2 (4.7k pull-down TPMS1) horizontal on Row 3: Col 5..8
pcb_footprints.append(f"""\t(footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "R2" (at {gx(6.5)} {gy(3.7)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "4.7k" (at {gx(6.5)} {gy(2.3)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_rect (start {gx(5.7)} {gy(2.7)}) (end {gx(7.3)} {gy(3.3)}) (stroke (width 0.1) (type solid)) (fill none) (layer "F.SilkS"))
{pad_tht(1, gx(5), gy(3), 1, "GND", False)}
{pad_tht(2, gx(8), gy(3), 5, "NET_IO13", False)}
\t)""")

# 8. C1 (100nF filter TPMS1) vertical at Col 9: Row 2 (GND) to Row 3 (NET_IO13)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C1" (at {gx(9.6)} {gy(2.5)} 90) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "100nF" (at {gx(8.4)} {gy(2.5)} 90) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(8.8)} {gy(2.5)}) (end {gx(9.2)} {gy(2.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(9), gy(2), 1, "GND", False)}
{pad_tht(2, gx(9), gy(3), 5, "NET_IO13", False)}
\t)""")

# 9. Q1 (2N7000 N-MOSFET TO-92 Wide) horizontal on Row 7: Col 5..7
# Pin 1 (Source) at (Col 7, Row 8) -> GND
# Pin 2 (Gate)   at (Col 6, Row 7) -> NET_GATE
# Pin 3 (Drain)  at (Col 5, Row 7) -> PS_ON#
pcb_footprints.append(f"""\t(footprint "Package_TO_SOT_THT:TO-92_Inline_Wide"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "Q1" (at {gx(6)} {gy(5.5)} 0) (layer "F.SilkS") (effects (font (size 0.6 0.6) (thickness 0.1))))
\t\t(property "Value" "2N7000" (at {gx(6)} {gy(8.9)} 0) (layer "F.Fab") (effects (font (size 0.6 0.6) (thickness 0.1))))
\t\t(fp_line (start {gx(4.2)} {gy(6.3)}) (end {gx(6.8)} {gy(6.3)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(6.8)} {gy(6.3)}) (end {gx(7.8)} {gy(7.3)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(7), gy(8), 1, "GND", False)}
{pad_tht(2, gx(6), gy(7), 7, "NET_GATE", False)}
{pad_tht(3, gx(5), gy(7), 6, "PS_ON#", False)}
\t)""")

# 10. R3 (1k series gate) vertical at Col 11: Row 5 (NET_IO14) to Row 7 (NET_GATE)
pcb_footprints.append(f"""\t(footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "R3" (at {gx(11.6)} {gy(6)} 90) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "1k" (at {gx(10.4)} {gy(6)} 90) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_rect (start {gx(10.7)} {gy(5.4)}) (end {gx(11.3)} {gy(6.6)}) (stroke (width 0.1) (type solid)) (fill none) (layer "F.SilkS"))
{pad_tht(1, gx(11), gy(5), 10, "NET_IO14", False)}
{pad_tht(2, gx(11), gy(7), 7, "NET_GATE", False)}
\t)""")

# 11. R4 (100k pull-down gate) vertical at Col 9: Row 7 (NET_GATE) to Row 8 (GND)
pcb_footprints.append(f"""\t(footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "R4" (at {gx(9.6)} {gy(7.5)} 90) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "100k" (at {gx(8.4)} {gy(7.5)} 90) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(8.8)} {gy(7.5)}) (end {gx(9.2)} {gy(7.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(9), gy(7), 7, "NET_GATE", False)}
{pad_tht(2, gx(9), gy(8), 1, "GND", False)}
\t)""")

# 12. C2 (100nF gate snubber) vertical at Col 8: Row 7 (NET_GATE) to Row 8 (GND)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C2" (at {gx(8.4)} {gy(7.5)} 90) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "100nF" (at {gx(7.6)} {gy(7.5)} 90) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(7.8)} {gy(7.5)}) (end {gx(8.2)} {gy(7.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(8), gy(7), 7, "NET_GATE", False)}
{pad_tht(2, gx(8), gy(8), 1, "GND", False)}
\t)""")

# 13. C_PWR3 (47uF 3.3V radial electrolytic) at Col 24: Row 2 (+3V3) to Row 3 (GND)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C_PWR3" (at {gx(24)} {gy(1.1)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "47uF" (at {gx(24)} {gy(3.9)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(23.4)} {gy(2)}) (end {gx(23.0)} {gy(2)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
\t\t(fp_line (start {gx(23.2)} {gy(1.8)}) (end {gx(23.2)} {gy(2.2)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(24), gy(2), 3, "+3V3", True, size=1.6)}
{pad_tht(2, gx(24), gy(3), 1, "GND", False, size=1.6)}
\t)""")

# 14. C_PWR4 (100nF 3.3V ceramic disc) at Col 26: Row 2 (+3V3) to Row 3 (GND)
pcb_footprints.append(f"""\t(footprint "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm"
\t\t(layer "F.Cu") (uuid "{u()}") (at 0 0)
\t\t(property "Reference" "C_PWR4" (at {gx(26)} {gy(1.1)} 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(property "Value" "100nF" (at {gx(26)} {gy(3.9)} 0) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))))
\t\t(fp_line (start {gx(25.7)} {gy(2.5)}) (end {gx(26.3)} {gy(2.5)}) (stroke (width 0.1) (type solid)) (layer "F.SilkS"))
{pad_tht(1, gx(26), gy(2), 3, "+3V3", False)}
{pad_tht(2, gx(26), gy(3), 1, "GND", False)}
\t)""")

# ===========================================================================
# 4. Single-Layer Routing (B.Cu)
# Strictly planar: zero track crossings!
# ===========================================================================
def trk(x1, y1, x2, y2, net_id, w=0.8):
    return f"""\t(segment (start {x1} {y1}) (end {x2} {y2}) (width {w}) (layer "B.Cu") (net {net_id}) (uuid "{u()}"))"""

tracks = []

# --- Net 2 (+5VSB) along Row 9 ---
# J_EXT pin 6 (2,9) -> C_PWR1 (10,9) -> C_PWR2 (11,9) -> J_CAM_L pin 8 (13,9)
tracks.append(trk(gx(2), gy(9), gx(10), gy(9), 2, 1.2))
tracks.append(trk(gx(10), gy(9), gx(11), gy(9), 2, 1.2))
tracks.append(trk(gx(11), gy(9), gx(13), gy(9), 2, 1.2))

# --- Net 1 (GND) Main Bus along Row 8 ---
# (2,8) -> Q1 S (7,8) -> C2 (8,8) -> R4 (9,8) -> C_PWR1 (10,8) -> C_PWR2 (11,8) -> J_CAM_L pin 7 (13,8) -> J_CAM_R pin 7 (22,8)
tracks.append(trk(gx(2), gy(8), gx(7), gy(8), 1, 1.2))
tracks.append(trk(gx(7), gy(8), gx(8), gy(8), 1, 1.2))
tracks.append(trk(gx(8), gy(8), gx(9), gy(8), 1, 1.2))
tracks.append(trk(gx(9), gy(8), gx(10), gy(8), 1, 1.2))
tracks.append(trk(gx(10), gy(8), gx(11), gy(8), 1, 1.2))
tracks.append(trk(gx(11), gy(8), gx(13), gy(8), 1, 1.2))
tracks.append(trk(gx(13), gy(8), gx(22), gy(8), 1, 1.2))

# --- Net 1 (GND) Top Bus along Row 1 ---
# Col 1 links Row 8 to Row 1
tracks.append(trk(gx(2), gy(8), gx(1), gy(8), 1, 0.8))
tracks.append(trk(gx(1), gy(8), gx(1), gy(1), 1, 0.8))
tracks.append(trk(gx(1), gy(1), gx(5), gy(1), 1, 0.8))
tracks.append(trk(gx(5), gy(1), gx(9), gy(1), 1, 0.8))

# Drops from Row 1 to R2 (5,3) and C1 (9,2)
tracks.append(trk(gx(5), gy(1), gx(5), gy(3), 1, 0.8))
tracks.append(trk(gx(9), gy(1), gx(9), gy(2), 1, 0.8))

# GND connection to 3.3V section via Col 27 around right edge:
# From J_CAM_R pin 7 (22,8) -> (27,8) -> (27,3) -> C_PWR4 (26,3) -> C_PWR3 (24,3) -> J_CAM_R pin 2 (22,3)
tracks.append(trk(gx(22), gy(8), gx(27), gy(8), 1, 0.8))
tracks.append(trk(gx(27), gy(8), gx(27), gy(3), 1, 0.8))
tracks.append(trk(gx(27), gy(3), gx(26), gy(3), 1, 0.8))
tracks.append(trk(gx(26), gy(3), gx(24), gy(3), 1, 0.8))
tracks.append(trk(gx(24), gy(3), gx(22), gy(3), 1, 0.8))

# --- Net 3 (+3V3) along Row 2 ---
# J_CAM_R pin 1 (22,2) -> C_PWR3 (24,2) -> C_PWR4 (26,2)
tracks.append(trk(gx(22), gy(2), gx(24), gy(2), 3, 0.8))
tracks.append(trk(gx(24), gy(2), gx(26), gy(2), 3, 0.8))

# --- Net 4 (TPMS1) ---
# J_EXT pin 1 (2,4) -> (2,2) -> R1 pad 1 (4,2)
tracks.append(trk(gx(2), gy(4), gx(2), gy(2), 4, 0.8))
tracks.append(trk(gx(2), gy(2), gx(4), gy(2), 4, 0.8))

# --- Net 5 (NET_IO13) ---
# R1 pad 2 (7,2) -> (8,2) -> R2 pad 2 (8,3) -> C1 pad 2 (9,3) -> J_CAM_L pin 2 (13,3)
tracks.append(trk(gx(7), gy(2), gx(8), gy(2), 5, 0.8))
tracks.append(trk(gx(8), gy(2), gx(8), gy(3), 5, 0.8))
tracks.append(trk(gx(8), gy(3), gx(9), gy(3), 5, 0.8))
tracks.append(trk(gx(9), gy(3), gx(13), gy(3), 5, 0.8))

# --- Net 8 (BTN_A) along Row 4 ---
# J_EXT pin 2 (2,5) -> (3,5) -> (3,4) -> J_CAM_L pin 3 (13,4)
tracks.append(trk(gx(2), gy(5), gx(3), gy(5), 8, 0.8))
tracks.append(trk(gx(3), gy(5), gx(3), gy(4), 8, 0.8))
tracks.append(trk(gx(3), gy(4), gx(13), gy(4), 8, 0.8))

# --- Net 9 (PWR_BTN#) along Row 6 ---
# J_EXT pin 3 (2,6) -> J_CAM_L pin 5 (13,6)
tracks.append(trk(gx(2), gy(6), gx(13), gy(6), 9, 0.8))

# --- Net 6 (PS_ON#) on Row 7 ---
# J_EXT pin 4 (2,7) -> Q1 Drain (5,7)
tracks.append(trk(gx(2), gy(7), gx(5), gy(7), 6, 0.8))

# --- Net 10 (NET_IO14) on Row 5 ---
# J_CAM_L pin 4 (13,5) -> R3 pad 1 (11,5)
tracks.append(trk(gx(13), gy(5), gx(11), gy(5), 10, 0.8))

# --- Net 7 (NET_GATE) on Row 7 ---
# Q1 Gate (6,7) -> C2 (8,7) -> R4 (9,7) -> R3 pad 2 (11,7)
tracks.append(trk(gx(6), gy(7), gx(8), gy(7), 7, 0.8))
tracks.append(trk(gx(8), gy(7), gx(9), gy(7), 7, 0.8))
tracks.append(trk(gx(9), gy(7), gx(11), gy(7), 7, 0.8))

pcb_full = pcb_header + "\n" + "\n".join(pcb_footprints) + "\n" + "\n".join(tracks) + "\n)\n"

# ===========================================================================
# 5. Execution & Validation
# ===========================================================================
def main():
    print(f"Writing project file to {PRO_PATH}...")
    with open(PRO_PATH, "w") as f:
        f.write(pro_content)

    print(f"Writing schematic file to {SCH_PATH}...")
    with open(SCH_PATH, "w") as f:
        f.write(sch_full)

    print("Verifying schematic netlist export...")
    netlist_out = os.path.join(OUT_DIR, "esp32cam-carrier.net")
    res = subprocess.run([KICAD_CLI, "sch", "export", "netlist", SCH_PATH, "-o", netlist_out], capture_output=True, text=True)
    if res.returncode != 0:
        print("ERROR: Schematic netlist export failed:", res.stderr)
        sys.exit(1)
    print("Schematic verified cleanly!")

    print(f"Writing PCB layout file to {PCB_PATH}...")
    with open(PCB_PATH, "w") as f:
        f.write(pcb_full)

    print("Running Design Rules Check (DRC) on PCB...")
    drc_report = os.path.join(OUT_DIR, "drc_report.rpt")
    res = subprocess.run([KICAD_CLI, "pcb", "drc", PCB_PATH, "-o", drc_report, "--severity-error", "--exit-code-violations"], capture_output=True, text=True)
    with open(drc_report) as f:
        drc_text = f.read()

    print("DRC Summary:")
    for line in drc_text.splitlines()[:25]:
        if "Found" in line or "violations" in line or "error" in line.lower():
            print("  ", line)

    if res.returncode != 0:
        print("ERROR: DRC failed with violations! Check report at", drc_report)
        print(drc_text)
        sys.exit(1)
    print("SUCCESS: 0 DRC violations! Board layout verified.")

    print("Exporting vector graphics and documentation artifacts...")
    subprocess.run([KICAD_CLI, "pcb", "export", "svg", PCB_PATH, "--layers", "B.Cu,Edge.Cuts", "-o", SVG_CU_PATH], check=True)
    subprocess.run([KICAD_CLI, "pcb", "export", "svg", PCB_PATH, "--layers", "F.SilkS,Edge.Cuts", "-o", SVG_SILK_PATH], check=True)
    subprocess.run([KICAD_CLI, "pcb", "export", "pdf", PCB_PATH, "--layers", "B.Cu,F.SilkS,Edge.Cuts", "-o", PDF_PATH], check=True)
    print("Exported:")
    print("  - Copper Layer SVG:", SVG_CU_PATH)
    print("  - Silkscreen SVG:  ", SVG_SILK_PATH)
    print("  - Combined PDF:    ", PDF_PATH)

if __name__ == "__main__":
    main()
