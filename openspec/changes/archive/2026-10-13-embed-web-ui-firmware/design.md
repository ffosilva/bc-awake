# Design: Embed Setup Portal Web UI in Firmware

## Context

The setup portal UI in `app/index.html` is ~30 KB uncompressed and compresses to ~8.2 KB with gzip. The portal currently mounts a SPIFFS partition and loads the file from disk on request. This requires two upload steps during initial setup and breaks web UI synchronization during OTA firmware updates.

See `proposal.md` for problem motivation and scope.

## Goals / Non-Goals

**Goals:**
- Automatically gzip and convert `app/index.html` into a C byte array header (`include/portal_html.h`) during the build.
- Serve the gzipped asset directly from flash (PROGMEM) with `Content-Encoding: gzip` using `ESPAsyncWebServer::beginResponse_P`.
- Remove SPIFFS filesystem initialization and dependencies from `src/portal.cpp`.
- Keep the partition table compatible with v1.1.0 so devices can upgrade over OTA without a wired reflash.

**Non-Goals:**
- Serving dynamic files from SPIFFS (the portal is a single-page app and has no other assets).

## Decisions

### Decision 1: Automated Pre-Build Asset Compression
* **Choice**: Use a PlatformIO pre-build script (`scripts/build_html.py`) referenced in `platformio.ini` via `extra_scripts = pre:scripts/build_html.py`.
* **Rationale**: Automatically gzips `app/index.html` and writes `include/portal_html.h` whenever `index.html` is modified, preventing manual sync steps.
* **Alternatives Considered**:
  - *Manual header maintenance*: Error-prone; developers could edit HTML without regenerating the C array.
  - *Raw uncompressed C string literal (`R"rawliteral(...)rawliteral"`)*: Would consume ~30 KB instead of ~8.2 KB of flash. Gzip saves ~22 KB of flash and speeds up transfer over the air.

### Decision 2: Native Gzip Serving via `beginResponse_P`
* **Choice**: Serve `INDEX_HTML_GZ` directly from flash using:
  ```cpp
  AsyncWebServerResponse *response = req->beginResponse_P(200, "text/html", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
  response->addHeader("Content-Encoding", "gzip");
  req->send(response);
  ```
* **Rationale**: All modern browsers decompress gzip instantly; serving directly from PROGMEM avoids copying the HTML payload into RAM.

### Decision 3: Preserve Dual-Bank Partition Table
* **Choice**: Retain the existing `partitions.csv` layout established in v1.1.0.
* **Rationale**: The ~8.2 KB addition fits comfortably in the >550 KB headroom of the 1.81 MB application banks. Keeping the exact same partition layout means boards already running v1.1.0 can update to this single-binary release completely over the air.

## Risks / Trade-offs

- **[Flash Footprint]** → Adds ~8.2 KB to the firmware binary. Mitigated by ~550 KB of free space in the application partition.
- **[Build Dependency on Python]** → PlatformIO already requires Python, so a standard Python build script adds zero external tool dependencies.

## Migration Plan

1. Build new single-binary firmware.
2. Upload via OTA or USB.
3. Once booted, the ESP32 serves the new web interface from flash. SPIFFS is no longer accessed.
