# Proposal: Embed Setup Portal Web UI in Firmware

## Why

Currently, the setup portal web interface (`app/index.html`) resides on a separate SPIFFS partition, requiring two distinct flashing steps during setup (`upload` and `uploadfs`) and producing two separate `.bin` files (`firmware.bin` and `spiffs.bin`). Because Over-The-Air (OTA) updates only flash the firmware partition, any changes to the web UI are not updated during an OTA flash, causing frontend and backend desynchronization. Compressing and embedding `index.html` directly into the firmware binary solves this problem completely, ensuring a single unified `.bin` binary that updates both application code and web interface in a single operation.

## What Changes

- **Embed Compressed Web Assets**: Compress `app/index.html` with gzip into a C/C++ byte array header (`include/portal_html.h`) at build time (consuming only ~8.2 KB of flash).
- **Serve from Flash Memory**: Update `serveIndex` in `src/portal.cpp` to serve the embedded gzipped HTML directly with `Content-Encoding: gzip` rather than opening a SPIFFS file handle.
- **Remove SPIFFS Dependency**: Remove SPIFFS initialization, mounting, and filesystem dependencies from portal code and build configuration.
- **Single-Binary Releases & Updates**: Eliminate `spiffs.bin` artifacts. All future builds, releases, and OTA updates will consist of a single standalone `.bin` file per hardware board.
- **Update Documentation & Build Scripts**: Remove references to `pio run -t uploadfs` and document the unified single-binary flashing workflow.

## Capabilities

### New Capabilities
*(None)*

### Modified Capabilities
- `setup-portal`: Modify requirement for serving the captive portal interface so that `index.html` is served directly from embedded, compressed firmware memory with gzip encoding rather than requiring a mounted SPIFFS filesystem.

## Impact

- **Build Pipeline**: Adds an automated asset generation script or header to gzip `app/index.html` into a C byte array before compilation.
- **Flash / RAM**: Consumes only ~8.2 KB of program flash (PROGMEM). Removes SPIFFS driver RAM overhead and VFS table allocations.
- **User Workflow**: Flashing becomes a single command (`pio run -t upload`). OTA becomes a single file upload in the browser.
