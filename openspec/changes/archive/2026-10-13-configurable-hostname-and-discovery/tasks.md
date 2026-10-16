# Tasks

## 1. Configurable Hostname & NVS Storage

- [x] 1.1 Add `hostname` field to `Config` struct in `include/config.h`, default to `"bc-awake"`, and implement load/save in `src/config.cpp` with NVS key `"hostname"`.
- [x] 1.2 Implement RFC 1123 hostname validation helper (1–32 characters, lowercase alphanumeric and hyphens, no leading/trailing hyphens) and verify acceptance and rejection on test inputs.
- [x] 1.3 Update `src/network_wake.cpp` to configure DHCP client name via `WiFi.setHostname(config.hostname.c_str())` and start mDNS via `MDNS.begin(config.hostname.c_str())`.

## 2. UDP Broadcast Discovery (Protocol v1)

- [x] 2.1 Bind a `WiFiUDP` listener on port `25025` in `src/network_wake.cpp` that remains active across all power states (`STATE_OFF`, `STATE_BOOTING`, `STATE_ON`) whenever Wi-Fi STA is connected.
- [x] 2.2 Implement packet parser in `networkWakeLoop()` matching `"BC250_DISCOVER"` and transmit unicast UDP JSON v1 response to sender's IP and port containing device, hostname, IP, active MAC, boardState, port, and version.
- [x] 2.3 Verify discovery response generation with test datagrams for both `STATE_OFF` (reporting `"OFF"` and dynamic dongle MAC) and `STATE_ON` (reporting `"ON"` and factory default MAC).

## 3. Setup Portal API & Web UI

- [x] 3.1 Update `/api/status` in `src/portal.cpp` to include the active `hostname` in the JSON status payload.
- [x] 3.2 Update `/api/network` handler in `src/portal.cpp` to accept `hostname`, reject invalid syntax with HTTP 400 Bad Request, reject updates with HTTP 403 while host is on, and persist valid hostnames.
- [x] 3.3 Add "Device Hostname / mDNS Name" input field to the Network Settings card in `app/index.html`, run `python scripts/build_html.py` to regenerate `include/portal_html.h`, and verify UI rendering.

## 4. Host Tooling Zero-Config Discovery (`bc250-sync`)

- [x] 4.1 Update `tools/bc250-sync/bc250-sync.py` to support `url = auto` by broadcasting `BC250_DISCOVER` on UDP port `25025` with a 2-second timeout and resolving the target URL from the unicast JSON response.
- [x] 4.2 Update `tools/bc250-sync/bc250-sync.conf` and `docs/host-sync.md` documenting `url = auto` zero-config discovery mode.

## 5. Integration Verification

- [x] 5.1 Run PlatformIO build across targets (`pio run`) to verify clean compilation with zero warnings or flash overflow.
- [x] 5.2 Validate OpenSpec change integrity with `openspec validate --all`.
