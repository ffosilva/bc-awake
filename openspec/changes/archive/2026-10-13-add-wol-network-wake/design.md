# Design

## Context

See `proposal.md` for background motivation. Currently, the ESP32 firmware manages the SFX PSU `PS_ON#` line and TPMS1 power-sensing line, and provides Bluetooth wake (Classic BT page scanning or BLE advertisement tracking) while the BC250 board is off (`STATE_OFF`). Wi-Fi is currently activated only when entering the setup portal via SoftAP.

In normal operation, the BC250 board is powered entirely from the SFX PSU's 12V rails. When the PSU is off, the BC250's built-in Ethernet NIC receives no standby power and cannot detect Wake-on-LAN packets. The ESP32, however, is continuously powered from the PSU's 5VSB rail.

## Goals / Non-Goals

**Goals:**
- Provide a non-blocking Wake-on-LAN listener in normal operation when enabled.
- Parse standard UDP Magic Packets on port 9 (and port 7) and match against a configured BC250 Ethernet MAC address.
- Support both password-protected and open (passwordless) Wi-Fi networks.
- Provide an explicit toggle (`wolEnabled`) in the Setup Portal to allow running fully offline without turning on the Wi-Fi radio.
- Maintain full compatibility with physical button controls, power-state debouncing, and Bluetooth controller wake.

**Non-Goals:**
- Running an HTTP web dashboard or REST API in normal operation mode (keeps normal mode lightweight).
- Sending outgoing WOL magic packets from the ESP32 (the ESP32 is exclusively a receiver/proxy).
- Supporting static IP configuration (DHCP is standard for home LANs).

## Decisions

### Decision 1: Dedicated UDP Socket via `WiFiUDP` in Normal Mode
- **Choice**: Use the Arduino `WiFiUDP` class listening on ports 9 and 7 in `normalLoop()`.
- **Rationale**: Minimal memory consumption (~2 KB RAM) and minimal CPU overhead. Avoids keeping `ESPAsyncWebServer` or SPIFFS active in normal mode, which reserves RAM for Bluetooth buffers and FreeRTOS tasks.
- **Alternatives Considered**:
  - Running a full web server in normal mode: Rejected to keep memory overhead minimal and reduce RF traffic.
  - Raw LwIP sockets: More complex with negligible performance gain over `WiFiUDP` for occasional packet checks.

### Decision 2: Strict Target MAC Matching Against Configured `bc250Mac`
- **Choice**: Parse the 102-byte Magic Packet sync pattern (`FF FF FF FF FF FF` + 16x 6-byte MAC) and only trigger wake if the MAC matches `config.bc250Mac`.
- **Rationale**: In home networks with multiple PCs or devices receiving WOL broadcasts, waking on any broadcast packet would cause unwanted power-on events. Binding to the BC250's Ethernet MAC aligns with standard WOL clients (Moonlight, Steam Link, Wake-on-LAN apps).
- **Alternatives Considered**:
  - Matching the ESP32's Wi-Fi MAC: Would require configuring streaming clients with the ESP32 MAC instead of the PC's actual MAC address.
  - Waking on any valid magic packet: High risk of false triggers.

### Decision 3: Non-Blocking Wi-Fi Station Management
- **Choice**: Initialize Wi-Fi in STA mode only if `config.wolEnabled` is true and `config.wifiSsid` is not empty. Wi-Fi connection attempts and background reconnects rely on non-blocking SDK state handling (`WiFi.setAutoReconnect(true)` and periodic status checks).
- **Rationale**: The switch loop must remain completely responsive to button presses and power sense ADC readings. Blocking calls like `WiFi.waitForConnectResult()` must never run in `normalLoop()`.
- **Passwordless Networks**: If `wifiPassword` is empty, connect using open authentication (`WiFi.begin(ssid.c_str())`).

### Decision 4: Setup Portal UI and API Structure
- **Choice**: Add an authenticated `/api/network` POST endpoint accepting `{ "wolEnabled": bool, "wifiSsid": string, "wifiPassword": string, "bc250Mac": string }`, and include network configuration status in the existing `/api/status` endpoint.
- **UI Integration**: In `app/index.html`, add a dedicated "Wake-on-LAN (Wi-Fi)" card containing the enable checkbox, SSID text input, password input (marked optional), and BC250 Ethernet MAC input.

### Decision 5: Coexistence and Power Gating
- **Choice**: Gated strictly to `STATE_OFF` with cooldown enforcement:
  - If received during `STATE_BOOTING` or `STATE_ON`, ignore silently.
  - If received within `WAKE_WAKE_COOLDOWN_MS` after power-off, ignore to prevent immediate re-trigger from streaming client reconnect bursts.
- **Radio Coexistence**: ESP-IDF software coexistence arbitrates radio time between Bluetooth scanning/page scan and Wi-Fi STA reception.

## Risks / Trade-offs

- **[Risk] Wi-Fi reconnection attempts blocking or delaying the normal loop**
  → *Mitigation*: Strictly non-blocking Wi-Fi management; check packet availability only when `WiFi.status() == WL_CONNECTED`.
- **[Risk] Radio coexistence impact on Classic Bluetooth page scan**
  → *Mitigation*: Wi-Fi traffic is idle listening for UDP broadcasts; page scan window remains spec-compliant (11.25 ms every 50 ms), leaving abundant airtime for Wi-Fi beacon and packet reception.
- **[Risk] Malformed or partial UDP packets causing crashes**
  → *Mitigation*: Validate packet length (`len >= 102`) before buffer parsing; inspect sync prefix and MAC repetitions with bounded pointer arithmetic.
