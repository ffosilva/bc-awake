# Tasks: Over-the-Air (OTA) Updates & Versioning

## 1. Partition Table & Firmware Version Metadata

- [x] 1.1 Update `partitions.csv` with dual-bank OTA layout (`app0` and `app1` at 0x1D0000, `spiffs` at 0x40000) and verify by compiling `pio run -e esp32dev`
- [x] 1.2 Create `include/version.h` defining `FW_VERSION "1.1.0"` and board target identification macros (`esp32dev`, `esp32cam`, `esp32-c3`)
- [x] 1.3 Include `version.h` in `src/main.cpp` and print version and target in the startup serial banner

## 2. Setup Portal Backend Implementation

- [x] 2.1 Update `handleStatus` in `src/portal.cpp` to include `version` and `target` in the JSON status response
- [x] 2.2 Implement authenticated `POST /api/update` handler in `src/portal.cpp` using `<Update.h>` with stream chunk writing, Bluetooth discovery suspension, and deferred restart
- [x] 2.3 Compile all three build targets (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`) and verify flash usage remains well within the 1.81 MB bank capacity

## 3. Web Portal UI & Documentation

- [x] 3.1 Update `app/index.html` header to display the running version badge and target identifier
- [x] 3.2 Add a Firmware Update card in `app/index.html` with file input for `.bin` files, progress bar, upload feedback, and rebooting overlay
- [x] 3.3 Update `README.md` to document the OTA update procedure and the one-time wired repartitioning requirement
