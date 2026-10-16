# Proposal

## Why

Today, the mDNS hostname is hardcoded to `bc-awake.local`. When multiple BC250 controllers operate on the same local network, their mDNS names collide, causing name resolution failures and making individual devices inaccessible. Additionally, mDNS resolution is frequently unsupported, blocked by network equipment, or absent on clients without Zeroconf responders.

To solve this, users need to configure a custom hostname for both DHCP and mDNS, and the firmware must provide an always-on, zero-dependency network discovery mechanism over UDP broadcast so clients and host tooling (like `bc250-sync`) can discover any switch on the LAN reliably without manual IP configuration.

## What Changes

- **Configurable Hostname & mDNS Name**:
  - Store a custom `hostname` in NVS configuration (defaulting to `"bc-awake"` if empty).
  - Enforce RFC 1123 hostname validation (1–32 lowercase alphanumeric characters and hyphens, no leading/trailing hyphens).
  - Register the custom hostname with DHCP via `WiFi.setHostname()` and mDNS via `MDNS.begin()`.
  - Expose `hostname` in `/api/status` and allow updating it via `/api/network` (protected: rejected with `403 Forbidden` while host is powered on).
  - Add a "Device Hostname / mDNS Name" input field to the setup portal web interface under Network Settings.
- **Magic Phrase UDP Broadcast Discovery (Protocol v1)**:
  - Run an always-available UDP listener on dedicated port `25025` across all power states (`STATE_OFF`, `STATE_BOOTING`, `STATE_ON`) whenever Wi-Fi Station is connected.
  - Listen for broadcast discovery datagrams with the magic phrase payload `"BC250_DISCOVER"`.
  - Respond via unicast UDP datagram to the requester's IP and port with a structured JSON v1 payload containing device identity, custom hostname, local IP, active MAC address, board power state, web port, and firmware version.
- **Host Sync Zero-Config Discovery**:
  - Update `tools/bc250-sync/bc250-sync.py` and `bc250-sync.conf` to support `url = auto`.
  - When `url = auto`, broadcast `BC250_DISCOVER` on UDP port 25025 to locate the ESP32 switch IP dynamically.

## Capabilities

### New Capabilities
<!-- None: enhancements naturally extend existing network-wake, setup-portal, and host-device-sync capabilities -->

### Modified Capabilities
- `network-wake`: Add always-on UDP broadcast discovery responder (port 25025, Protocol v1) across all power states.
- `setup-portal`: Support configurable hostname and mDNS name in NVS, REST API (`/api/status`, `/api/network`), and Web UI.
- `host-device-sync`: Support `url = auto` automatic discovery fallback in `bc250-sync` via UDP broadcast.

## Impact

- **Firmware (`include/config.h`, `src/config.cpp`)**: Add `hostname` field to `Config`, default to `"bc-awake"`, with NVS persistence and validation.
- **Firmware (`src/network_wake.cpp`, `include/network_wake.h`)**: Add UDP listener on port 25025 to process `"BC250_DISCOVER"` and send unicast JSON v1 response across power states; use configured hostname in `WiFi.setHostname()` and `MDNS.begin()`.
- **Firmware (`src/portal.cpp`)**: Include `hostname` in `/api/status`, accept `hostname` in `/api/network`, and enforce validation.
- **Web Interface (`app/index.html`, `include/portal_html.h`)**: Add hostname field in Network settings card and rebuild embedded HTML asset.
- **Host Tooling (`tools/bc250-sync/`)**: Implement broadcast discovery when `url = auto`.
