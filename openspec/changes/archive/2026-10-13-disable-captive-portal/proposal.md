# Proposal

## Why

When client devices connect to the setup SoftAP ("BC250 Switch Setup"), operating systems (iOS, Android, macOS, Windows) detect captive portal behavior due to wildcard DNS interception and catch-all HTTP routing. This automatically triggers captive network assistant modal popups or webviews, which provide a degraded user experience, lack full browser capabilities (such as password autofill or file upload reliability), and can prematurely disconnect from the Wi-Fi network if dismissed. Disabling captive network triggers allows clients to associate cleanly without popup prompts, enabling users to access the configuration portal through their full-featured web browser at `http://192.168.4.1`.

## What Changes

- Remove the wildcard DNS server (`DNSServer`) running on port 53 in setup mode that previously resolved all domain queries to `192.168.4.1`.
- Remove the HTTP catch-all route handler that previously served `index.html` on unknown URLs, replacing it with a standard HTTP 404 Not Found response so OS connectivity probes (`captive.apple.com`, `generate_204`, etc.) are not treated as captive portal indications.
- Continue serving the embedded setup web interface and REST API at `http://192.168.4.1` for explicit root requests (`/`).
- Update the `setup-portal` specification to eliminate captive network requirements while retaining SoftAP web interface hosting.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `setup-portal`: Update the SoftAP network and web serving requirement so that connecting to the setup access point does not trigger OS captive portal assistants, eliminating wildcard DNS hijacking and captive catch-all responses while serving the setup interface at `192.168.4.1`.

## Impact

- **Firmware source**: `src/portal.cpp` and `include/portal.h` — remove `DNSServer` instantiation and polling from `portalBegin()` and `portalLoop()`; update `server.onNotFound` handler to return 404 for unknown URLs.
- **Resource usage**: Reduces RAM consumption and CPU loop overhead by eliminating the DNS server daemon on port 53.
- **User workflow**: Users connect to the "BC250 Switch Setup" Wi-Fi network and manually open their web browser to `http://192.168.4.1` rather than being forced into an OS captive webview popup.
