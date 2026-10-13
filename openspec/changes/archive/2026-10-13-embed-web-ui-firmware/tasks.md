# Tasks: Embed Setup Portal Web UI in Firmware

## 1. Asset Compression & Header Generation

- [x] 1.1 Create `scripts/build_html.py` to gzip `app/index.html` and generate `include/portal_html.h` with `INDEX_HTML_GZ` and length constants
- [x] 1.2 Configure `platformio.ini` with `extra_scripts = pre:scripts/build_html.py` and verify `include/portal_html.h` is generated automatically during build

## 2. Web Portal Server Refactoring

- [x] 2.1 Refactor `serveIndex` in `src/portal.cpp` to serve `INDEX_HTML_GZ` using `req->beginResponse_P` with `Content-Encoding: gzip`
- [x] 2.2 Remove `SPIFFS.h` include, `SPIFFS.begin()` call, and filesystem error handling from `src/portal.cpp`
- [x] 2.3 Remove `board_build.filesystem = spiffs` from `platformio.ini` while keeping `partitions.csv` intact

## 3. Build Verification & Documentation

- [x] 3.1 Compile all three hardware targets (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) and verify clean compilation and bank capacity
- [x] 3.2 Update `README.md` to reflect single-binary flashing and remove references to `uploadfs`
