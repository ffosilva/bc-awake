# Design

## Context

See `proposal.md - Why` for motivation.

Currently, the ESP32 switches between two mutually exclusive modes:
1. **Setup Mode (`portalBegin`)**: Starts a SoftAP network (`192.168.4.1`) and runs the `AsyncWebServer`. The ATX power supply is held off.
2. **Normal Mode (`normalBegin`)**: Manages ATX power via `PS_ON#`, senses motherboard power on `TPMS1`, handles Bluetooth controller wake, and optionally connects to Wi-Fi STA solely to listen for Wake-on-LAN UDP packets on ports 7 and 9. The web portal is completely offline.

When paired gamepads or host Bluetooth adapters change, the user is forced to enter setup mode manually via an 8-second button hold. Furthermore, USB Wi-Fi dongles on the BC250 motherboard lose power when the ATX PSU is off, preventing standard Wake-on-Wireless-LAN (WoWLAN).

## Goals / Non-Goals

**Goals:**
- Keep the HTTP web server and REST API permanently online in normal mode over Wi-Fi Station connectivity.
- Dynamically toggle the Wi-Fi Station MAC address: spoof the Wi-Fi dongle's MAC while `STATE_OFF` (to capture WOL packets and maintain router presence), and restore the factory ESP32 MAC while `STATE_ON` (to avoid network collisions with the active host dongle).
- Advertise `bc250-switch.local` via mDNS on Wi-Fi STA to enable stable addressing across MAC/IP changes.
- Provide a standalone, zero-dependency Python 3 Linux systemd service for Bazzite, SteamOS, Ubuntu, and Arch Linux to synchronize local paired gamepads and the host adapter MAC to the ESP32 REST API.
- Prevent Bluetooth sensing and Wi-Fi reconfiguration while the host machine is in `STATE_ON`.

**Non-Goals:**
- Native Windows background service (Linux is the target host OS for this change).
- Modifying ATX power control states or timing thresholds (debouncing, power-on/power-off sequences remain unchanged).
- Modifying Bluetooth Classic HCI page-scan emulation logic during `STATE_OFF`.

## Decisions

### 1. Dynamic Wi-Fi Station MAC Switching on State Transitions
- **Approach**: When entering `STATE_OFF`, the ESP32 calls `WiFi.disconnect(true)`, invokes `esp_wifi_set_mac(WIFI_IF_STA, dongleMac)`, and reconnects. When transitioning to `STATE_BOOTING` or `STATE_ON`, the ESP32 calls `WiFi.disconnect(true)`, restores `esp_wifi_set_mac(WIFI_IF_STA, defaultMac)`, and reconnects.
- **Rationale**: If the ESP32 retained the dongle's MAC while the host PC is powered on, both the ESP32 and the host Wi-Fi dongle would transmit with the same MAC address on the same AP, triggering severe frame collisions, ARP confusion, and network drops. If the ESP32 never spoofed the dongle's MAC, routers bound to the host's MAC would drop directed wake packets.
- **Alternatives Considered**:
  - *Keep ESP32 MAC permanently*: Breaks WoWLAN when routers require packets sent to the host's registered MAC or IP lease.
  - *Keep dongle MAC permanently*: Causes network collapse as soon as the host boots.

### 2. Persistent Web Server & mDNS in Normal Mode
- **Approach**: Initialize `server.begin()` and `MDNS.begin("bc250-switch")` inside normal mode whenever Wi-Fi STA connects. SoftAP remains an onboarding mechanism when Wi-Fi is unconfigured or recovery is requested via button hold.
- **Rationale**: Enables remote monitoring, configuration, OTA updates, and automated device synchronization without taking down the PC. mDNS ensures clients and background services can reach `http://bc250-switch.local` even if the DHCP-assigned IP changes between `STATE_OFF` and `STATE_ON`.
- **Alternatives Considered**:
  - *Static IP configuration*: Requires manual router or portal configuration and is brittle across different subnets.

### 3. Power-State Guardrails in Web UI and REST API
- **Approach**:
  - Extend `handleStatus` (`GET /api/status`) to output `boardState: "OFF" | "BOOTING" | "ON"`.
  - In `src/portal.cpp`: return `403 Forbidden` on `/api/network` and `409 Conflict` on `/api/listener/start` when `state != STATE_OFF`.
  - In `app/index.html`:
    - Display a visual status light badge in the header: green dot for `POWERED ON` (`STATE_ON`), red dot for `POWERED OFF` (`STATE_OFF`), and amber/yellow dot for `BOOTING` (`STATE_BOOTING`).
    - When `boardState === "ON"`, disable Wi-Fi configuration input fields, hide the network save button, disable the interactive pairing listener button, and display a status banner explaining that sensing is paused to prevent adapter conflicts. Manual controller enrollment and bound controller list management remain active.
- **Rationale**: While the host is ON, its real Bluetooth adapter is active. Running page scan or inquiry with the spoofed MAC on the ESP32 creates radio contention and desyncs controllers from the host. Clear visual cues inform the user why configuration options are temporarily locked.

### 4. Zero-Dependency Linux Host Sync Service (`bc250-sync`)
- **Approach**: A Python 3 script using standard library modules (`urllib.request`, `json`, `subprocess`) paired with systemd service and timer units.
  - Inspects local Bluetooth state via `bluetoothctl list` and `bluetoothctl devices`.
  - Authenticates with `POST /api/login` at `http://bc250-switch.local`.
  - Compares local state against `GET /api/status`.
  - Synchronizes adapter MAC via `POST /api/host` and adds/removes controllers via `POST /api/controllers/add` and `POST /api/controllers/remove`.
- **Rationale**: Bazzite and SteamOS are immutable distributions with read-only root filesystems where `pip install` is unavailable by default. A zero-dependency script ensures seamless operation out-of-the-box on all Linux distributions.
- **Alternatives Considered**:
  - *Compiled Go/Rust binary*: Requires cross-compilation toolchains and larger binaries.
  - *Direct BlueZ D-Bus Python bindings*: Requires `python3-dbus` or `python3-gi`, which may not be packaged in all minimal OS images. `bluetoothctl` is universally present with BlueZ.

## Risks / Trade-offs

- **[Risk] Brief Wi-Fi disconnection during power-state transition**
  - *Mitigation*: Dropping and reconnecting Wi-Fi to swap MAC addresses takes 1.5–3 seconds. Because the BC250 takes 8–15 seconds to complete BIOS POST and boot the Linux kernel, the ESP32 completes its reconnection with its default MAC well before the host OS initializes its Wi-Fi adapter.
- **[Risk] Router ARP cache lag**
  - *Mitigation*: Re-associating with the access point triggers 802.11 association frames and gratuitous ARP broadcasts, causing APs and switches to update their port/MAC forwarding tables immediately.
- **[Risk] Controller limit exceeded (max 8)**
  - *Mitigation*: The sync tool sorts controllers and synchronizes at most the first 8 paired controllers, logging an informative notice if more than 8 controllers are paired locally.
