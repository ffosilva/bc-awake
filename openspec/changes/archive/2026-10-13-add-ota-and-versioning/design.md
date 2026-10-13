# Design: Over-the-Air (OTA) Updates & Versioning

## Context

The BC250 ESP32 power switch operates with 4 MB flash across three targets (`esp32dev`, `esp32cam`, `esp32-c3-devkitm-1`). The recent addition of Wake-on-LAN (WOL) brings compiled firmware sizes to ~1.24 MB (ESP32 / ESP32-CAM) and ~1.32 MB (ESP32-C3). Setup mode provides a Wi-Fi SoftAP running `ESPAsyncWebServer` at `http://192.168.4.1`, while `partitions.csv` currently allocates only a single application partition (`app0`), preventing wireless updates.

See `proposal.md` for background motivation.

## Goals / Non-Goals

**Goals:**
- Provide safe, atomic dual-bank OTA flashing in setup mode using standard Arduino `Update.h`.
- Protect the update route with the existing session bearer token (`X-Auth-Token`).
- Keep firmware binaries below partition limits with >500 KB headroom for all targets.
- Introduce compile-time semantic versioning and target hardware identity reported via API, web UI, and serial log.
- Provide a clean in-page web UI file picker and real-time progress indicator in `app/index.html`.

**Non-Goals:**
- OTA over BLE or Classic Bluetooth (too slow, incompatible across targets, requires dedicated phone apps).
- Automatic background downloads from the internet (the board operates primarily on an isolated SoftAP or home LAN without cloud dependencies).
- Filesystem (SPIFFS) OTA updates in this initial phase (reserved for future enhancement).

## Decisions

### Decision 1: Dual-Bank 1.81 MB Partition Table
* **Choice**: Set `app0` (ota_0) and `app1` (ota_1) to `0x1D0000` (1,900,544 bytes / ~1.81 MB) each, shrink SPIFFS from 1.25 MB to 256 KB (`0x40000`), keep NVS at 20 KB (`0x5000`), otadata at 8 KB (`0x2000`), and coredump at 64 KB (`0x10000`).
* **Rationale**: The largest target (ESP32-C3) compiles to ~1.32 MB, leaving ~580 KB of headroom (69.5% bank utilization). `index.html` is 28.5 KB, utilizing only ~11% of the 256 KB SPIFFS partition.
* **Alternatives Considered**:
  - *Single bank with SPIFFS buffer*: Flashing directly over running firmware without rollback risks bricking upon interrupted writes. Dual bank with `otadata` provides hardware rollback.

### Decision 2: Stream Handling via `ESPAsyncWebServer` & `Update.h`
* **Choice**: Implement `POST /api/update` using `ESPAsyncWebServer`'s chunked body/upload handler with `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)`, `Update.write()`, and `Update.end(true)`.
* **Rationale**: Reuses the existing async HTTP server; streams chunks directly to the target flash partition without allocating a multi-megabyte RAM buffer.
* **Alternatives Considered**:
  - *AsyncOTA / ElegantOTA libraries*: Introduces extra third-party dependencies; native `Update.h` handles this in <50 lines of clean code.

### Decision 3: Suspend Bluetooth Discovery During Flashing
* **Choice**: Call `wakeSetDiscovery(false)` at the start of the upload stream (`index == 0`) and reboot upon completion.
* **Rationale**: Flash write cycles temporarily disable the SPI cache. Running simultaneous Bluetooth page inquiry/listening creates CPU contention and risks task watchdog timeouts.

### Decision 4: Central Versioning Header (`include/version.h`)
* **Choice**: Create `include/version.h` defining `FW_VERSION "1.1.0"` (baseline post-WOL) and conditional `FW_TARGET` (`esp32dev`, `esp32cam`, `esp32-c3`).
* **Rationale**: Provides a single source of truth across `main.cpp`, `portal.cpp`, and status responses, while allowing future build-system overrides via `-D FW_VERSION`.

## Risks / Trade-offs

- **[Initial Wired Flash Required]** → Altering the partition table shifts flash boundaries. Users must perform one initial wired flash (`pio run -t upload && pio run -t uploadfs`) to flash the new partition table before OTA works. Documented clearly in README and setup instructions.
- **[Power Interruption During Upload]** → If power cuts mid-flash, the active partition remains untouched in `otadata`. The ESP32 reboots safely into the existing functional firmware.
- **[Wrong Architecture Upload]** → Flashing a C3 binary onto a classic ESP32 will fail boot validation. The ESP32 bootloader checks chip ID magic bytes in the image header before swapping banks.

## Migration Plan

1. Flash `partitions.csv` and firmware via USB once.
2. Verify dual banks via serial boot log.
3. Test OTA by uploading a new build via `http://192.168.4.1/api/update`.
