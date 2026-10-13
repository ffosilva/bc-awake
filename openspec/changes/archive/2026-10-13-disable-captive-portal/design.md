# Design

## Context

See `proposal.md` for motivation. Currently in `src/portal.cpp`:
- A global `DNSServer dnsServer` is started on port 53 in `portalBegin()` with wildcard redirection (`dnsServer.start(53, "*", WiFi.softAPIP())`), and is serviced in each iteration of `portalLoop()`.
- `server.onNotFound()` catches every non-`/api/` request and calls `serveIndex(req)`.
- Together, this triggers captive network detection mechanisms across all major operating systems (Apple CNA, Android CaptivePortalLogin, Windows NCSI), launching restrictive captive webviews upon connection.

## Goals / Non-Goals

**Goals:**
- Completely disable automatic captive portal / network assistant triggering when client devices connect to the "BC250 Switch Setup" SoftAP.
- Remove `DNSServer` usage from `src/portal.cpp` and `include/portal.h`.
- Update `server.onNotFound()` to respond with HTTP 404 Not Found for any unknown routes rather than serving `index.html`.
- Retain full functionality of the web UI and REST API at `http://192.168.4.1` on root `/` (and in station mode via station IP / mDNS).
- Reduce memory usage and loop processing overhead by eliminating DNS server execution.

**Non-Goals:**
- Modifying SoftAP SSID, IP configuration (`192.168.4.1`), or TX power.
- Adding a runtime toggle or configuration flag to re-enable captive portal behavior.
- Altering the REST API routes or normal Station mode operation.

## Decisions

### Decision 1: Remove `DNSServer` completely rather than maintaining selective resolution
- **Rationale**: The only function of `DNSServer` in this project was wildcard DNS redirection to trigger captive portal detection. Removing `#include <DNSServer.h>`, the `dnsServer` instance, and `dnsServer.processNextRequest()` cleans up the codebase and frees RAM and loop CPU cycles.
- **Alternatives Considered**:
  - *Selective DNS resolution (e.g., only resolving `bc250.setup` or `switch.local`)*: Adds unnecessary DNS handling complexity when users can directly navigate to `192.168.4.1`.
  - *Keeping DNSServer and spoofing probe success*: Answering probes with fake 204/success responses tricks the OS into thinking full internet access is available, causing OS background services to attempt internet queries over the SoftAP and fail. Leaving port 53 inactive is standard for offline configuration APs.

### Decision 2: Standardize `server.onNotFound()` to return HTTP 404
- **Rationale**: Currently, `server.onNotFound()` returns 404 only for paths starting with `/api/`, while falling back to `serveIndex()` for all other paths. Changing this to return HTTP 404 for all unmatched paths ensures that unexpected probe requests (or direct IP requests to non-existent assets) do not return the portal HTML.
- **Alternatives Considered**:
  - *302 Redirect to `/`*: Still triggers captive portal detection heuristics on certain OS probes. Returning 404 avoids captive detection.

## Risks / Trade-offs

- **[Risk] Users cannot type arbitrary URLs (e.g., `setup.com`) to reach the portal** → **Mitigation**: Users navigate directly to `http://192.168.4.1`. Serial logs already print `[PORTAL] connect to AP and open http://192.168.4.1`.
- **[Risk] OS warning banner ("Wi-Fi has no internet connection")** → **Mitigation**: This is expected and standard for all modern devices connected to local IoT access points; users simply remain connected and access the IP in their browser.
