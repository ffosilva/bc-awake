# Tasks

## 1. Portal Firmware Implementation

- [x] 1.1 Remove `DNSServer` include, static instance, `dnsServer.start()` in `portalBegin()`, and `dnsServer.processNextRequest()` in `portalLoop()` in `src/portal.cpp` and update comments in `include/portal.h`, verifying that the codebase contains no remaining `DNSServer` references.
- [x] 1.2 Update `server.onNotFound` handler in `src/portal.cpp` to send HTTP 404 Not Found for all unmapped requests instead of falling back to `serveIndex()`, verifying that captive catch-all routing is eliminated.

## 2. Verification and Build Validation

- [x] 2.1 Build all PlatformIO environments (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) using `pio run`, verifying successful zero-warning builds across all targets.
- [x] 2.2 Verify route definitions to ensure root `/` serves embedded index HTML and `/api/*` endpoints continue operating correctly while unknown paths return HTTP 404.
