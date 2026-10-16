# Design

## Context

See `proposal.md` for motivation. Currently, the ESP32 switch hardcodes `bc-awake.local` for mDNS and only listens for UDP datagrams on ports 7 and 9 in `STATE_OFF` for Wake-on-LAN magic packets. The web server and REST API run continuously over Wi-Fi Station mode, but discovering the device requires either functional mDNS resolution or knowing its DHCP IP address.

## Goals / Non-Goals

**Goals:**
- Provide a configurable device hostname persisted in NVS, registered with both DHCP (`WiFi.setHostname`) and mDNS (`MDNS.begin`).
- Implement an always-available UDP broadcast discovery responder on dedicated port `25025` (Protocol v1) across all power states.
- Respond with a structured unicast JSON payload containing device metadata, network address, and power state.
- Update `tools/bc250-sync` to support zero-configuration automatic discovery when `url = auto`.
- Maintain zero external dependencies for both the ESP32 firmware and the host sync script.

**Non-Goals:**
- Cross-subnet or cloud-based discovery (discovery is strictly broadcast over the local Wi-Fi subnet).
- Dynamic mDNS reconfiguration without Wi-Fi reconnection or restart.
- Implementing DNS-SD service browsing in `bc250-sync.py` (which would require external third-party Python packages like `zeroconf`).

## Decisions

### Decision 1: Dedicated UDP Discovery Port (25025)
- **Choice**: Use dedicated UDP port `25025` for network discovery, independent of WOL ports 7 and 9.
- **Rationale**: Ports 7 and 9 are standard echo/discard ports reserved for Wake-on-LAN frames while in `STATE_OFF`. The discovery listener must run across all power states (`STATE_OFF`, `STATE_BOOTING`, `STATE_ON`) to allow clients and host sync scripts to find the device while the PC is running.
- **Alternatives Considered**:
  - *Piggyback on port 9*: Increases parsing complexity and risks accidental wake logic triggers.
  - *DNS-SD (mDNS browsing)*: Standard in Apple/Avahi ecosystems, but breaks zero-dependency operation on minimal Linux environments (Bazzite, SteamOS) and Windows.

### Decision 2: Unicast JSON Reply (Protocol v1)
- **Choice**: When receiving `"BC250_DISCOVER"` on port 25025, reply directly to the sender's IP and port using unicast UDP with a JSON v1 payload.
- **Rationale**: Unicast avoids unnecessary broadcast traffic on the local Wi-Fi subnet. Structured JSON is easily parsed by Python, scripts, and frontends, and ArduinoJson is already integrated into the firmware.
- **Schema**:
  ```json
  {
    "proto": "v1",
    "device": "bc-awake",
    "hostname": "bc-awake",
    "ip": "192.168.1.142",
    "mac": "24:6f:28:aa:bb:cc",
    "boardState": "OFF",
    "version": "1.2.0",
    "port": 80
  }
  ```

### Decision 3: Hostname Validation and Storage
- **Choice**: Persist `hostname` under key `"hostname"` in NVS namespace `"bc250"`, defaulting to `"bc-awake"` if empty. Validate against RFC 1123 label rules: 1–32 characters, lowercase alphanumeric and hyphens, no consecutive or leading/trailing hyphens.
- **Rationale**: Prevents invalid mDNS / NetBIOS / DNS characters that could break local network resolution or router DHCP tables.
- **API Enforcement**: Hostname is submitted via `/api/network`. If the host is in `STATE_ON` or `STATE_BOOTING`, the request is rejected with `403 Forbidden` (consistent with existing Wi-Fi settings lockouts). If invalid, rejected with `400 Bad Request`.

### Decision 4: Zero-Config Host Sync (`url = auto`)
- **Choice**: Allow `url = auto` in `tools/bc250-sync/bc250-sync.conf`. When set to `auto`, `bc250-sync.py` sends a broadcast datagram to `255.255.255.255:25025` with a 2-second socket timeout, parses the JSON response, and extracts the target HTTP URL.
- **Rationale**: Allows instant out-of-the-box operation on Linux hosts without requiring users to find the IP address or debug mDNS resolution.

## Risks / Trade-offs

- **[Risk] UDP broadcast packet flooding / amplification** → *Mitigation*: The listener only responds when the payload strictly matches `"BC250_DISCOVER"` (trimmed). Unicast replies are capped to a single compact JSON packet (<200 bytes).
- **[Risk] Multiple BC250 devices responding to broadcast** → *Mitigation*: `bc250-sync.py` can match against configured hostname if specified, or pick the first responding device if `url = auto`.
- **[Risk] mDNS collision on custom hostname** → *Mitigation*: mDNS responder returns an error if a name conflict occurs; fallback to discovery protocol ensures device remains accessible even if mDNS registration fails.
